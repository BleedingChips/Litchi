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
			asio::ip::tcp::resolver Resolver;
			std::array<asio::ip::tcp::endpoint, 20> EndPoints;
			std::size_t AvailableEndPointCount = 0;
			ResolverAgency(Context& Context) : ResolverAgency(Context.IOContext) {}
			ResolverAgency(asio::io_context& Context) : Resolver(Context) {}
			~ResolverAgency() {}
		};

		struct SocketAngency : Potato::Misc::DefaultIntrusiveObjectInterface<SocketAngency>
		{

			using Ptr = Potato::Misc::IntrusivePtr<SocketAngency>;

			static auto Create(Context& Context) -> Ptr;

			auto Connect(std::u8string_view Host, std::u8string_view Service) -> std::future<std::error_code>;
			auto Connect(asio::ip::tcp::endpoint EndPoint) -> std::future<std::error_code>;
			auto Close() -> std::future<bool>;
			~SocketAngency();

			template<typename RespondFunction>
			auto Send(std::span<std::byte const> PersistenceBuffer, RespondFunction FinishFunc) -> void 
				requires(std::is_invocable_v<RespondFunction, std::error_code>);

			template<typename RespondFunction>
			auto Recive(std::span<std::byte> PersistenceBuffer, RespondFunction FinishOnceRespond) -> void
				requires(std::is_invocable_v<RespondFunction, std::error_code, std::size_t>);

		protected:

			template<typename RespondFunction>
			void TryRecive(std::size_t ReadCount, std::span<std::byte> PersistenceBuffer, RespondFunction FinishOnceRespond);
			static void TryConnect(std::size_t EndPointeIndex, SocketAngency::Ptr This, ResolverAgency::Ptr Resolver, std::promise<std::error_code> Promise);
			
			SocketAngency(Context& Context);

			asio::ip::tcp::socket Socket;
			asio::io_context::strand Strand;

		};

		template<typename RespondFunction>
		auto SocketAngency::Send(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> void 
			requires(std::is_invocable_v<RespondFunction, std::error_code>)
		{
			
			auto TempFunc = [This = SocketAngency::Ptr{this}, PersistenceBuffer, Func = std::move(Func)]() mutable {
				assert(This);
				auto TThis = This.Data();
				TThis->Socket.async_write_some(asio::const_buffer{ PersistenceBuffer.data(), PersistenceBuffer.size() }, [This = std::move(This), PersistenceBuffer, Func = std::move(Func)](std::error_code EC, std::size_t WritedCount) mutable {
					if (!EC)
					{
						if (WritedCount >= PersistenceBuffer.size())
							Func({});
						else
							This->Send(PersistenceBuffer.subspan(WritedCount), std::move(Func));
					}
					else {
						Func(EC);
					}
				});
			};
			Strand.post(std::move(TempFunc), std::allocator<decltype(TempFunc)>{});
		}

		template<typename RespondFunction>
		auto SocketAngency::Recive(std::span<std::byte> PersistenceBuffer, RespondFunction Func) -> void
			requires(std::is_invocable_v<RespondFunction, std::error_code, std::size_t>)
		{
			this->TryRecive(0, PersistenceBuffer, std::move(Func));
		}

		template<typename RespondFunction>
		void SocketAngency::TryRecive(std::size_t LastReadCount, std::span<std::byte> PersistenceBuffer, RespondFunction Func)
		{
			auto TempFunc = [This = SocketAngency::Ptr{ this }, PersistenceBuffer, LastReadCount, Func = std::move(Func)]() mutable {
				assert(This);
				auto TThis = This.Data();
				TThis->Socket.async_read_some(asio::mutable_buffer{ PersistenceBuffer.data(), PersistenceBuffer.size() }, [This = std::move(This), PersistenceBuffer, LastReadCount, Func = std::move(Func)](std::error_code EC, std::size_t ReadedCount) mutable {
					if (!EC)
					{
						if(ReadedCount >= PersistenceBuffer.size() && LastReadCount != 0)
							Func({}, ReadedCount + LastReadCount);
						else
							This->TryRecive(ReadedCount + LastReadCount, PersistenceBuffer.subspan(ReadedCount), std::move(Func));
					}
					else {
						Func(EC, ReadedCount + LastReadCount);
					}
					});
			};
			Strand.post(std::move(TempFunc), std::allocator<decltype(TempFunc)>{});
		}
	}

	/*
	struct TcpSocketConnection
	{
		bool Connect(std::u8string_view Host, std::u8string_view Service);
		bool Close();

		TcpSocketConnection() : Context(), Socket(Context) {}
		~TcpSocketConnection();

	private:
		
		asio::io_context Context;
		asio::ip::tcp::socket Socket;
	};
	*/


}