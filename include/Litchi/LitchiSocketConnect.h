#pragma once
#include "LitchiContext.h"
#include "asio/ip/tcp.hpp"
#include "asio/io_context_strand.hpp"
#include "Potato/PotatoIntrusivePointer.h"
#include <string_view>
#include <array>
#include <future>
namespace Litchi
{
	
	namespace Tcp
	{

		struct ResolverAgency : Potato::Misc::DefaultIntrusiveObjectInterface<ResolverAgency>
		{
			using Ptr = Potato::Misc::IntrusivePtr<ResolverAgency>;

			static auto Create(asio::io_context& context) -> Ptr { return new ResolverAgency{ context }; }

			using ResultsT = asio::ip::tcp::resolver::results_type;

			template<typename RespondFunc>
			auto Resolver(std::u8string_view Host, std::u8string_view Service, RespondFunc Func) -> void
				requires(std::is_invocable_v<RespondFunc, std::error_code, ResultsT>)
			{
				IOResolver.async_resolve(
					std::string_view{reinterpret_cast<char const*>(Host.data()), Host.size()},
					std::string_view{ reinterpret_cast<char const*>(Service.data()), Service.size() },
					[This = ResolverAgency::Ptr{this}, Func = std::move(Func)](std::error_code EC, ResultsT Results) mutable{
						Func(EC, std::move(Results));
					}
				);
			}

			~ResolverAgency() {}

		protected:

			asio::ip::tcp::resolver IOResolver;

			ResolverAgency(asio::io_context& Context) : IOResolver(Context) {}
			
		};

		template<template<typename...> class AllocatorT = std::allocator>
		struct SocketAngency : Potato::Misc::DefaultIntrusiveObjectInterface<SocketAngency<AllocatorT>>
		{

			using Ptr = Potato::Misc::IntrusivePtr<SocketAngency>;
			using EndPointT = asio::ip::tcp::endpoint;

			static auto Create(asio::io_context& Context) -> Ptr
			{
				return new SocketAngency{Context};
			}

			template<typename RespondFunc>
			auto Connect(std::u8string_view Host, std::u8string_view Service, RespondFunc Func) -> void 
				requires(std::is_invocable_v<RespondFunc, std::error_code, EndPointT>)
			{
				auto Resolver = ResolverAgency::Create(Strand.context());
				Resolver->Resolver(Host, Service, [This = SocketAngency::Ptr{this}, Func = std::move(Func)](std::error_code EC, ResolverAgency::ResultsT Results) mutable {
					assert(This);
					if (!EC)
					{
						std::vector<EndPointT, AllocatorT<EndPointT>> EndPoints;
						EndPoints.reserve(Results.size());
						for (auto& Ite : Results)
							EndPoints.emplace_back(Ite);
						std::reverse(EndPoints.begin(), EndPoints.end());
						assert(!EndPoints.empty());
						auto Top = std::move(*EndPoints.rbegin());
						EndPoints.pop_back();
						SocketAngency::ConnectExecute(Top, std::move(EndPoints), std::move(This), std::move(Func));
					}
					else {
						Func(EC, asio::ip::tcp::endpoint{});
					}
				});
			}

			template<typename RespondFunc>
			auto Connect(EndPointT EndPoint, RespondFunc Func) -> void
				requires(std::is_invocable_v<RespondFunc, std::error_code, EndPointT>)
			{
				SocketAngency::ConnectExecute(EndPoint, {}, SocketAngency::Ptr{this}, std::move(Func));
			}

			template<typename RespondFunc>
			auto Close(RespondFunc Func)
			{
				auto TempFunc = [This = SocketAngency::Ptr{ this }, Func = std::move(Func)]() mutable {
					assert(This);
					if (This->Socket.is_open())
					{
						This->Socket.close();
						Func(true);
					}
					else
						Func(false);
				};
				Strand.post(std::move(TempFunc), AllocatorT<decltype(TempFunc)>{});
			}

			~SocketAngency() {}

			template<typename RespondFunction>
			auto Send(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> void 
				requires(std::is_invocable_v<RespondFunction, std::error_code>)
			{
				SocketAngency::SendExecute(SocketAngency::Ptr{ this }, PersistenceBuffer, std::move(Func));
			}

			struct ReciveResult
			{
				std::size_t CurrentStepReaded;
				std::size_t TotalRead;
			};

			template<typename RespondFunction>
			auto Recive(std::span<std::byte> PersistenceBuffer, RespondFunction FinishOnceRespond) -> void
				requires(std::is_invocable_r_v<bool, RespondFunction, std::error_code, ReciveResult>)
			{
				ReciveExecute(0, SocketAngency::Ptr{this}, PersistenceBuffer, std::move(FinishOnceRespond));
			}

		protected:

			template<typename RespondFunction>
			static auto ConnectExecute(
				EndPointT EndPoint,
				std::vector<EndPointT, AllocatorT<EndPointT>> EndPoints,
				SocketAngency::Ptr This,
				RespondFunction Func
			) -> void
			{
				assert(This);
				auto TThis = This.GetPointer();
				auto TempFunc = [EndPoint = std::move(EndPoint), This = std::move(This), EndPoints = std::move(EndPoints), Func = std::move(Func)]() mutable {
					assert(This);
					auto TThis = This.GetPointer();
					TThis->Socket.async_connect(EndPoint, [EndPoint, This = std::move(This), EndPoints = std::move(EndPoints), Func = std::move(Func)](std::error_code EC) mutable {
						if (!EC)
						{
							Func(EC, EndPoint);
						}
						else {
							if (!EndPoints.empty())
							{
								auto Top = std::move(*EndPoints.rbegin());
								EndPoints.pop_back();
								SocketAngency::ConnectExecute(Top, std::move(EndPoints), std::move(This), std::move(Func));
							}
							else {
								Func(EC, asio::ip::tcp::endpoint{});
							}
						}
					});
				};
				TThis->Strand.post(std::move(TempFunc), AllocatorT<decltype(TempFunc)> {});
			}

			template<typename RespondFunction>
			static void SendExecute(SocketAngency::Ptr This, std::span<std::byte const> PersistenceBuffer, RespondFunction Func)
			{
				assert(This);
				auto TThis = This.GetPointer();
				auto TempFunc = [This = std::move(This), PersistenceBuffer, Func = std::move(Func)]() mutable {
					assert(This);
					auto TThis = This.GetPointer();
					TThis->Socket.async_write_some(asio::const_buffer{ PersistenceBuffer.data(), PersistenceBuffer.size() }, [This = std::move(This), PersistenceBuffer, Func = std::move(Func)](std::error_code EC, std::size_t WritedCount) mutable {
						if (!EC)
						{
							if (WritedCount >= PersistenceBuffer.size())
								Func({});
							else
								SocketAngency::SendExecute(std::move(This), PersistenceBuffer.subspan(WritedCount), std::move(Func));
						}
						else {
							Func(EC);
						}
						});
				};
				TThis->Strand.post(std::move(TempFunc), std::allocator<decltype(TempFunc)>{});
			}

			template<typename RespondFunction>
			static void ReciveExecute(std::size_t LastReadCount, SocketAngency::Ptr This, std::span<std::byte> PersistenceBuffer, RespondFunction Func)
			{
				assert(This);
				auto TThis = This.GetPointer();
				auto TempFunc = [This = std::move(This), PersistenceBuffer, LastReadCount, Func = std::move(Func)]() mutable {
					assert(This);
					auto TThis = This.GetPointer();
					TThis->Socket.async_read_some(asio::mutable_buffer{ PersistenceBuffer.data(), PersistenceBuffer.size() }, [This = std::move(This), PersistenceBuffer, LastReadCount, Func = std::move(Func)](std::error_code EC, std::size_t ReadedCount) mutable {
						if (!EC)
						{
							if (Func(EC, ReciveResult{ ReadedCount, ReadedCount + LastReadCount }))
							{
								SocketAngency::ReciveExecute(ReadedCount + LastReadCount, std::move(This), PersistenceBuffer.subspan(ReadedCount), std::move(Func));
							}
						}
						else {
							Func(EC, ReciveResult{ ReadedCount, ReadedCount + LastReadCount });
						}
						});
				};
				TThis->Strand.post(std::move(TempFunc), std::allocator<decltype(TempFunc)>{});
			}
			
			SocketAngency(asio::io_context& Context) : Socket(Context), Strand(Context) {}

			asio::ip::tcp::socket Socket;
			asio::io_context::strand Strand;

		};
	}
}