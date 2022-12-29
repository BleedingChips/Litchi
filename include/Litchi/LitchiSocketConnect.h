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

		struct Agency : IntrusiveObjInterface
		{
			using PtrT = IntrusivePtr<Agency>;

			static auto Create(asio::io_context& Context) -> PtrT { return new Agency{ Context }; }

			template<typename RespondFunction>
			auto Connect(EndPointT EP, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code>)
			{
				{
					auto lg = std::lock_guard(SocketMutex);
					if (!IsConnecting)
					{
						IsConnecting = true;
						Socket.async_connect(EP, [This = PtrT{ this }, Func = std::move(Func)](std::error_code EC) mutable {
							{
								assert(This);
								auto lg = std::lock_guard(This->SocketMutex);
								This->IsConnecting = false;
							}
							Func(EC);
						});
						return true;
					}
				}
				Func(asio::error::in_progress);
				return false;
			}

			template<typename RespondFunction>
			auto Send(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, std::size_t>)
			{
				{
					auto lg = std::lock_guard(SocketMutex);
					if (!IsSending)
					{
						IsSending = true;
						Agency::SendExecute(0, PtrT{ this }, PersistenceBuffer, std::move(Func));
						return true;
					}
				}
				Func(asio::error::in_progress, 0);
				return false;
			}

			struct ReceiveT
			{
				std::size_t CurrentLength = 0;
				std::size_t TotalLength = 0;
			};

			template<typename DetectFunction, typename RespondFunction>
			auto Receive(std::span<std::byte> PersistenceBuffer, DetectFunction DF, RespondFunction RF) -> bool
				requires(
					std::is_invocable_r_v<std::optional<std::span<std::byte>>, DetectFunction, ReceiveT> &&
					std::is_invocable_v<RespondFunction, std::error_code, ReceiveT>
				)
			{
				{
					auto lg = std::lock_guard(SocketMutex);
					if (!IsReceiving)
					{
						IsReceiving = true;
						Agency::ReceiveExecute(0, PtrT{ this }, PersistenceBuffer, std::move(DF), std::move(RF));
						return true;
					}
				}
				RF(asio::error::in_progress, {0, 0});
				return false;
			}

		protected:

			template<typename RespondFunction>
			static auto SendExecute(std::size_t LastWrited, PtrT This, std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> void
			{
				assert(This);
				auto TThis = This.GetPointer();
				TThis->Socket.async_write_some(
					asio::const_buffer{ PersistenceBuffer.data(), PersistenceBuffer.size()}, 
					[This = std::move(This), PersistenceBuffer, Func = std::move(Func), LastWrited](std::error_code EC, std::size_t WritedBuffer) mutable {
						assert(This);
						if (!EC && PersistenceBuffer.size() < WritedBuffer)
						{
							auto lg = std::lock_guard(This->SocketMutex);
							Agency::SendExecute(LastWrited + WritedBuffer, std::move(This), PersistenceBuffer.subspan(WritedBuffer), std::move(Func));
						}
						else {
							{
								auto lg = std::lock_guard(This->SocketMutex);
								assert(This->IsSending);
								This->IsSending = false;
							}
							Func(EC, LastWrited + WritedBuffer);
						}
					});
			}

			template<typename DetectFunction, typename RespondFunction>
			static auto ReceiveExecute(std::size_t LastReceive, PtrT This, std::span<std::byte> PersistenceBuffer, DetectFunction DFunc, RespondFunction Func)
			{
				assert(This);
				auto TThis = This.GetPointer();
				TThis->Socket.async_read_some(asio::mutable_buffer{ PersistenceBuffer.data(), PersistenceBuffer.size()}, 
					[This = std::move(This), LastReceive, DFunc = std::move(DFunc), Func = std::move(Func)](std::error_code EC, std::size_t Received) mutable {
						if (!EC)
						{
							auto Ter = DFunc({ Received, LastReceive + Received });
							if (Ter.has_value())
							{
								auto lg = std::lock_guard(This->SocketMutex);
								Agency::ReceiveExecute(LastReceive, std::move(This), *Ter, std::move(DFunc), std::move(Func));
								return;
							}
						}
						{
							auto lg = std::lock_guard(This->SocketMutex);
							assert(This->IsReceiving);
							This->IsReceiving = false;
						}
						Func(EC, { Received, LastReceive + Received });
					}
				);
			}

			Agency(asio::io_context& Context) : Socket(Context) {}

			std::mutex SocketMutex;
			bool IsConnecting = false;
			bool IsSending = false;
			bool IsReceiving = false;
			asio::ip::tcp::socket Socket;
			
			friend class TcpSocket;
		};

		using PtrT = Agency::PtrT;

		static auto Create(asio::io_context& Context) -> PtrT { return Agency::Create(Context); }

	};
}