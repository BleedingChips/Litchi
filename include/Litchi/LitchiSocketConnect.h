#pragma once
#include "LitchiContext.h"
#include "asio/ip/tcp.hpp"
#include "asio/error_code.hpp"
#include <string_view>
#include <array>
#include <future>
#include <deque>
#include <mutex>
#include <system_error>

namespace Litchi
{

	struct TcpSocket : protected IntrusiveObjInterface
	{
		using EndPointT = asio::ip::tcp::endpoint;
		using ResolverT = asio::ip::tcp::resolver::results_type;
		using PtrT = IntrusivePtr<TcpSocket>;

		static auto Create(asio::io_context& Context) -> PtrT { return new TcpSocket(Context); }

		struct Agency
		{

			template<typename RespondFunction>
			auto Connect(EndPointT EP, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code&, EndPointT, Agency&>)
			{
				return Socket->AsyncConnect(EP, std::move(Func));
			}

			template<typename RespondFunction>
			auto Connect(std::u8string_view Host, std::u8string_view Service, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code&, EndPointT, Agency&>)
			{
				return Socket->AsyncConnect(Host, Service, std::move(Func));
			}
				
			template<typename RespondFunction>
			auto Send(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code&, std::size_t, Agency&>)
			{
				return Socket->AsyncSend(PersistenceBuffer, std::move(Func));
			}

			template<typename RespondFunction>
			auto ReceiveSome(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code&, std::size_t, Agency&>)
			{
				return Socket->AsyncReceiveSome(PersistenceBuffer, std::move(Func));
			}

			template<typename RespondFunction>
			auto Receive(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code&, std::size_t, Agency&>)
			{
				return Socket->AsyncReceive(PersistenceBuffer, std::move(Func));
			}

			void Cancel() { Socket -> CancelExecute(); }
			void Close() { Socket->CloseExecute(); }

			bool AbleToConnect() const { return Socket->AbleToConnect(); }
			bool AbleToSend() const { return Socket->AbleToSend(); }
			bool AbleToReceive() const { return Socket->AbleToReceive(); }

		protected:

			Agency(Agency const&) = default;
			Agency(TcpSocket* Socket) : Socket(Socket) {}
			TcpSocket* Socket = nullptr;


			friend class TcpSocket;
		};

		template<typename AgencyFunc>
		void Lock(AgencyFunc&& Func) requires(std::is_invocable_v<AgencyFunc, Agency&>)
		{
			auto lg = std::lock_guard(SocketMutex);
			Func(Agency(this));
		}

		auto SyncConnect(EndPointT) -> std::optional<std::error_code>;
		auto SyncConnect(std::u8string_view Host, std::u8string_view Service) -> std::optional<std::tuple<std::error_code, EndPointT>>;
		
		auto SyncCancel() -> void;
		auto SyncClose() -> void;
		

	protected:

		template<typename RespondFunc>
		auto AsyncConnect(EndPointT EP, RespondFunc Fun) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code&, EndPointT, Agency&>)
		{
			if (AbleToConnect())
			{
				Connecting = true;
				TcpSocket::ConnectExecute(PtrT{this}, EP, {}, std::move(Fun));
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncConnect(std::u8string_view Host, std::u8string_view Service, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code&, EndPointT, Agency&>)
		{
			if (AbleToConnect())
			{
				Connecting = true;
				Resolver.async_resolve(
					std::string_view{ reinterpret_cast<char const*>(Host.data()), Host.size() },
					std::string_view{ reinterpret_cast<char const*>(Service.data()), Service.size() },
					[This = PtrT{ this }, Func = std::move(Func)](std::error_code EC, ResolverT RR) mutable {
						assert(This);
						if (!EC && RR.size() >= 1)
						{
							std::vector<EndPointT> EndPoints;
							EndPoints.resize(RR.size() - 1);
							auto Ite = RR.begin();
							auto Top = *Ite;
							for (++Ite; Ite != RR.end(); ++Ite)
								EndPoints.push_back(*Ite);
							std::reverse(EndPoints.begin(), EndPoints.end());
							{
								auto lg = std::lock_guard(This->SocketMutex);
								TcpSocket::ConnectExecute(std::move(This), std::move(Top), std::move(EndPoints), std::move(Func));
							}
						}
						else {
							auto lg = std::lock_guard(This->SocketMutex);
							This->Connecting = false;
							Agency Age{ This.GetPointer() };
							Func(EC, {}, Age);
						}
					}
				);
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncSend(std::span<std::byte const> PersistenceBuffer, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code&, std::size_t, Agency&>)
		{
			if (AbleToSend())
			{
				Sending = true;
				TcpSocket::SendExecute(0, PtrT{this}, PersistenceBuffer, std::move(Func));
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncReceiveSome(std::span<std::byte> PersistenceBuffer, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code&, std::size_t, Agency&>)
		{
			if (AbleToReceive())
			{
				Receiving = true;
				Socket.async_read_some(
					{ PersistenceBuffer.data(), PersistenceBuffer .size()},
					[Func = std::move(Func), This = PtrT{this}](std::error_code& EC, std::size_t Readed) {
						auto lg = std::lock_guard(This->SocketMutex);
						This->Receiving = false;
						Func(EC, Readed, Agency{ This .GetPointer()});
					}
				);
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncReceive(std::span<std::byte> PersistenceBuffer, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code&, std::size_t, Agency&>)
		{
			if (AbleToReceive())
			{
				TcpSocket::ReceiveExecute(0, PersistenceBuffer, PtrT{this}, std::move(Func));
				return true;
			}
			return false;
		}

		auto CloseExecute() -> void;
		auto CancelExecute() -> void;

		bool IsConnecting() const { return Connecting; }
		bool IsSending() const { return Sending; }
		bool IsReceiving() const { return Receiving; }

		bool AbleToConnect() const { return !IsConnecting() && !IsSending() && !IsReceiving(); }
		bool AbleToSend() const { return !IsConnecting() && !IsSending(); }
		bool AbleToReceive() const { return !IsConnecting() && !IsReceiving(); }

		TcpSocket(asio::io_context& Context) : Socket(Context), Resolver(Context) {}

		std::mutex SocketMutex;
		asio::ip::tcp::socket Socket;
		asio::ip::tcp::resolver Resolver;

		bool Connecting = false;
		bool Sending = false;
		bool Receiving = false;

	private:

		template<typename RespondFunction>
		static auto ConnectExecute(PtrT This, EndPointT CurEndPoint, std::vector<EndPointT> EndPoints, RespondFunction Func) -> void
		{
			assert(This);
			TcpSocket* TThis = This.GetPointer();
			TThis->Socket.async_connect(CurEndPoint, [CurEndPoint, This = std::move(This), EndPoints = std::move(EndPoints), Func = std::move(Func)](std::error_code EC) mutable {
				if (EC && EC != asio::error::operation_aborted && !EndPoints.empty())
				{
					auto Top = std::move(*EndPoints.rbegin());
					EndPoints.pop_back();
					auto lg = std::lock_guard(This->SocketMutex);
					TcpSocket::ConnectExecute(std::move(This), std::move(Top), std::move(EndPoints), std::move(Func));
				}
				else {
					auto lg = std::lock_guard(This->SocketMutex);
					This->Connecting = false;
					Agency Age{ This.GetPointer() };
					Func(EC, CurEndPoint, Age);
				}
			});
		}

		template<typename RespondFunction>
		static void SendExecute(std::size_t TotalWrite, PtrT This, std::span<std::byte const> Buffer, RespondFunction Func)
		{
			assert(This);
			TcpSocket* TThis = This.GetPointer();
			TThis->Socket.async_write_some(asio::const_buffer{ Buffer.data(), Buffer.size() },
				[This = std::move(This), Buffer, TotalWrite, Func = std::move(Func)](std::error_code& EC, std::size_t Writed) mutable {
					if (!EC && Writed < Buffer.size())
					{
						auto lg = std::lock_guard(This->SocketMutex);
						TcpSocket::SendExecute(TotalWrite + Writed, std::move(This), Buffer.subspan(Writed), std::move(Func));
					}
					else {
						auto lg = std::lock_guard(This->SocketMutex);
						This->Sending = false;
						Func(EC, TotalWrite + Writed, Agency{This.GetPointer()});
					}
				}
			);
		}

		template<typename RespondFunction>
		static void ReceiveExecute(std::size_t TotalSize, std::span<std::byte> IteBuffer, PtrT This, RespondFunction Func)
		{
			assert(This);
			TcpSocket* TThis = This.GetPointer();
			TThis->Socket.async_read_some(asio::mutable_buffer{ IteBuffer.data(), IteBuffer.size() },
				[TotalSize, IteBuffer, This = std::move(This), Func = std::move(Func)](std::error_code& EC, std::size_t Readed) mutable {
					if (!EC && IteBuffer.size() > Readed)
					{
						auto lg = std::lock_guard(This->SocketMutex);
						TcpSocket::ReceiveExecute(TotalSize + Readed, IteBuffer.subspan(Readed), std::move(This), std::move(Func));
					}
					else {
						auto lg = std::lock_guard(This->SocketMutex);
						This->Receiving = false;
						Func(EC, TotalSize + Readed, Agency{This.GetPointer()});
					}
				}
			);
		}

		friend struct IntrusiceObjWrapper;
	};
}