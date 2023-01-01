#pragma once
#include "LitchiContext.h"
#include "asio/ip/tcp.hpp"
#include "asio/io_context_strand.hpp"
#include "asio/error_code.hpp"
#include <string_view>
#include <array>
#include <future>
#include <deque>
#include <mutex>
#include <system_error>

namespace Litchi
{
	
	struct TcpResolver
	{
		using ResultT = asio::ip::tcp::resolver::results_type;

		struct Agency : public IntrusiveObjInterface
		{

			using PtrT = IntrusivePtr<Agency>;

			template<typename RespondFunction>
			auto Resolve(std::u8string_view Host, std::u8string_view Service, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, ResultT>)
			{
				auto lg = std::lock_guard(ResolverMutex);
				if (!IsResolving)
				{
					IsResolving = true;
					Resolver.async_resolve(
						std::string_view{reinterpret_cast<char const*>(Host.data()), Host.size()},
						std::string_view{ reinterpret_cast<char const*>(Service.data()), Service.size()},
						[This = PtrT{this}, Func = std::move(Func)](std::error_code EC, ResultT RR) mutable {
							assert(This);
							{
								auto lg = std::lock_guard(This->ResolverMutex);
								This->IsResolving = false;
							}
							Func(EC, RR);
						}
					);
				}
				return false;
			}

			auto Cancle() -> void {	auto lg = std::lock_guard(ResolverMutex); IsResolving = false; Resolver.cancel(); }
			~Agency() { Cancle(); }

			static auto Create(asio::io_context& Context) -> PtrT { return new Agency{Context}; }
			bool InResolving() const { 
				auto lg = std::lock_guard(ResolverMutex);
				return IsResolving;
			}

		protected:

			Agency(asio::io_context& Context) : Resolver(Context) {}

			mutable std::mutex ResolverMutex;
			bool IsResolving = false;
			asio::ip::tcp::resolver Resolver;

			friend struct IntrusiceObjWrapper;
		};
		
		using PtrT = Agency::PtrT;

		static auto Create(asio::io_context& Context) -> PtrT { return Agency::Create(Context); }

	};



	struct TcpSocket
	{
		using EndPointT = asio::ip::tcp::endpoint;

		struct SendRequestT
		{
			std::span<std::byte const> SendBuffer;
			std::function<void(std::error_code, std::size_t)> RespondFunction;
		};

		struct ReceiveRequestT
		{
			std::span<std::byte> ReceiveBuffer;
			std::function<std::optional<std::span<std::byte>>(std::size_t)> ReallocateFunction;
			std::function<void(std::error_code, std::size_t)> RespondFunction;
		};


		struct Agency : IntrusiveObjInterface
		{
			using PtrT = IntrusivePtr<Agency>;

			static auto Create(asio::io_context& Context) -> PtrT { return new Agency{ Context }; }

			template<typename RespondFunction>
			auto Connect(EndPointT EP, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, EndPointT>)
			{
				auto lg = std::lock_guard(SocketMutex);
				if (!IsConnecting && !IsSending && !IsReceiving)
				{
					IsConnecting = true;
					Agency::ConnectExecute(PtrT{ this }, EP, {}, std::move(Func));
					return true;
				}
				auto Temp = [Func = std::move(Func)]() {
					Func(asio::error::in_progress, {});
				};
				Strand.post(std::move(Temp), std::allocator<decltype(Temp)>{});
				return false;
			}

			template<typename RespondFunction>
			auto Connect(std::u8string_view Host, std::u8string_view Service, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, EndPointT>)
			{
				auto lg = std::lock_guard(SocketMutex);
				if (!IsConnecting && !IsSending && !IsReceiving)
				{
					IsConnecting = true;
					Resolver = TcpResolver::Create(Strand.context());
					Resolver->Resolve(Host, Service, [This = PtrT{ this }, Func = std::move(Func)](std::error_code EC, TcpResolver::ResultT RR) mutable {
						assert(This);
						Agency* TThis = This.GetPointer();
						{
							auto lg = std::lock_guard(TThis->SocketMutex);
							TThis->Resolver.Reset();
							if (!EC && RR.size() >= 1)
							{
								std::vector<EndPointT> EndPoints;
								EndPoints.resize(RR.size() - 1);
								auto Ite = RR.begin();
								auto Top = *Ite;
								for (++Ite; Ite != RR.end(); ++Ite)
									EndPoints.push_back(*Ite);
								std::reverse(EndPoints.begin(), EndPoints.end());
								Agency::ConnectExecute(std::move(This), std::move(Top), std::move(EndPoints), std::move(Func));
								return;
							}
						}
						Func(EC, {});
					});
					return true;
				}
				auto Temp = [Func = std::move(Func)]() mutable {
					Func(asio::error::in_progress, {});
				};
				Strand.post(std::move(Temp), std::allocator<decltype(Temp)>{});
				return false;
			}
				
			template<typename RespondFunction>
			auto Send(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, std::size_t>)
			{
				auto lg = std::lock_guard(SocketMutex);
				if (!IsConnecting && !IsSending)
				{
					IsSending = true;
					Agency::SendExecute(0, PtrT{ this }, PersistenceBuffer, std::move(Func));
					return true;
				}
				auto Temp = [Func = std::move(Func)]() mutable {
					Func(asio::error::operation_aborted, 0);
				};
				Strand.post(std::move(Temp), std::allocator<decltype(Temp)>{});
				return false;
			}

			template<typename ProtocolFunction, typename RespondFunction>
			auto FlexibleReceive(ProtocolFunction PF, RespondFunction Func) -> bool
				requires(
					std::is_invocable_r_v<std::optional<std::size_t>, ProtocolFunction, std::span<std::byte const>>
					&& std::is_invocable_v<RespondFunction, std::error_code, std::span<std::byte>>
					)
			{
				auto lg = std::lock_guard(SocketMutex);
				if (!IsConnecting && !IsReceiving)
				{
					IsSending = true;
					Agency::FlexibleReceiveProtocolExecute(1024, 0, PtrT{ this }, std::move(PF), std::move(Func));
					return true;
				}
				auto Temp = [Func = std::move(Func)]() mutable {
					Func(asio::error::operation_aborted, {});
				};
				Strand.post(std::move(Temp), std::allocator<decltype(Temp)>{});
				return false;
			}

			void Cancle();
			void Close();


		protected:

			template<typename RespondFunction>
			static auto ConnectExecute(PtrT This, EndPointT CurEndPoint, std::vector<EndPointT> EndPoints, RespondFunction Func) -> void
			{
				assert(This);
				Agency* TThis = This.GetPointer();
				TThis->Socket.async_connect(CurEndPoint, [CurEndPoint, This = std::move(This), EndPoints = std::move(EndPoints), Func = std::move(Func)](std::error_code EC) mutable {
					if (EC && EC != asio::error::operation_aborted && !EndPoints.empty())
					{
						auto Top = std::move(*EndPoints.rbegin());
						EndPoints.pop_back();
						auto lg = std::lock_guard(This->SocketMutex);
						Agency::ConnectExecute(std::move(This), std::move(Top), std::move(EndPoints), std::move(Func));
					}
					else {
						{
							auto lg = std::lock_guard(This->SocketMutex);
							This->IsConnecting = false;
						}
						Func(EC, CurEndPoint);
					}
				});
			}


			void CancelSendReceiveExecute();

			template<typename RespondFunction>
			static void SendExecute(std::size_t TotalWrite, PtrT This, std::span<std::byte const> Buffer, RespondFunction Func)
			{
				assert(This);
				Agency* TThis = This.GetPointer();
				TThis->Socket.async_write_some(asio::const_buffer{ Buffer.data(), Buffer.size()}, 
					[This = std::move(This), Buffer, TotalWrite, Func = std::move(Func)](std::error_code EC, std::size_t Writed) mutable {
						if (!EC && Writed < Buffer.size())
						{
							auto lg = std::lock_guard(This->SocketMutex);
							Agency::SendExecute(TotalWrite + Writed, std::move(This), Buffer.subspan(Writed), std::move(Func));
						}
						else {
							{
								auto lg = std::lock_guard(This->SocketMutex);
								This->IsSending = false;
							}
							Func(EC, TotalWrite + Writed);
						}
				});
			}

			template<typename ProtocolFunction, typename RespondFunction>
			static void FlexibleReceiveProtocolExecute(std::size_t CacheSize, std::size_t LastReadSize, PtrT This, ProtocolFunction PF, RespondFunction Func)
			{
				assert(This);
				Agency* TThis = This.GetPointer();
				TThis->ReceiveBuffer.clear();
				TThis->ReceiveBuffer.resize(CacheSize);
				TThis->Socket.async_receive(
					asio::mutable_buffer{ TThis->ReceiveBuffer.data(), CacheSize },
					asio::ip::tcp::socket::message_peek,
					[CacheSize, LastReadSize, This = std::move(This), PF = std::move(PF), Func = std::move(Func)](std::error_code EC, std::size_t ReadSize) mutable {
						if (!EC && ReadSize != LastReadSize)
						{
							auto lg = std::lock_guard(This->SocketMutex);
							This->ReceiveBuffer.resize(ReadSize);
							auto PFSize = PF(std::span(This->ReceiveBuffer));
							This->ReceiveBuffer.clear();
							if (PFSize.has_value())
							{
								This->ReceiveBuffer.resize(*PFSize);
								auto TThis = This.GetPointer();
								Agency::ReceiveExecute(std::span(TThis->ReceiveBuffer), std::span(TThis->ReceiveBuffer), std::move(This), std::move(Func));
							}
							else {
								Agency::FlexibleReceiveProtocolExecute(CacheSize * 2, ReadSize, std::move(This), std::move(PF), std::move(Func));
							}
						}
						else {
							{
								auto lg = std::lock_guard(This->SocketMutex);
								This->IsReceiving = false;
							}
							if(!EC && ReadSize == LastReadSize)
								Func(asio::error::no_protocol_option, {});
							else
								Func(EC, {});
						}
					}
				);
			}

			template<typename RespondFunction>
			static void ReceiveExecute(std::span<std::byte> TotalBuffer, std::span<std::byte> IteBuffer, PtrT This, RespondFunction Func)
			{
				assert(This);
				Agency* TThis = This.GetPointer();
				TThis->Socket.async_read_some(asio::mutable_buffer{ IteBuffer.data(), IteBuffer.size()},
					[TotalBuffer, IteBuffer, This = std::move(This), Func = std::move(Func)](std::error_code EC, std::size_t Readed) mutable {
						if (!EC && IteBuffer.size() > Readed)
						{
							auto lg = std::lock_guard(This->SocketMutex);
							Agency::ReceiveExecute(TotalBuffer, IteBuffer.subspan(Readed), std::move(This), std::move(Func));
						}
						else {
							{
								auto lg = std::lock_guard(This->SocketMutex);
								This->IsReceiving = false;
							}
							Func(EC, TotalBuffer.subspan(0, TotalBuffer.size() + Readed - IteBuffer.size() ));
						}
					}
				);
			}

			Agency(asio::io_context& Context) : Socket(Context), Strand(Context) {}

			std::mutex SocketMutex;
			asio::ip::tcp::socket Socket;
			asio::io_context::strand Strand;

			bool IsConnecting = false;
			TcpResolver::PtrT Resolver;
			bool IsSending = false;
			bool IsReceiving = false;

			std::vector<std::byte> ReceiveBuffer;
			
			friend class TcpSocket;
		};

		using PtrT = Agency::PtrT;

		static auto Create(asio::io_context& Context) -> PtrT { return Agency::Create(Context); }

	};
}