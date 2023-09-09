#pragma once
#include <stdint.h>

namespace std
{
	class error_code;
}

namespace std::pmr
{
	class memory_resource;
}


namespace Litchi::AsioWrapper
{

	struct Layout
	{
		unsigned long long Size = 0;
		unsigned long long Align = 0;
	};

	struct Context
	{
		static Layout GetLayout();
		static Context* Construct(void* Adress);
		virtual ~Context() = default;
		virtual bool PollOne() = 0;
		virtual void Cancel() = 0;
	};

	struct TCPSocket
	{
		static Layout GetLayout();
		static TCPSocket* Construct(void* Adress, Context& Content, std::pmr::memory_resource* Resource);
		virtual ~TCPSocket() = default;
		virtual void Connect(
			char8_t const* Host, unsigned long HostSize, char8_t const* Server, unsigned long ServerSize,
			void (*Executer)(void* AppendData, std::error_code const&), void* AppendBuffer, std::pmr::memory_resource* Resource
		) = 0;

		virtual void Send(
			void const* Data, unsigned long long DataCount,
			void (*Executer)(void* AppendData, std::error_code const&, unsigned long long), void* AppendBuffer
		) = 0;

		virtual void ReadSome(
			void* Data, unsigned long long DataCount,
			void (*Executer)(void* AppendData, std::error_code const&, unsigned long long), void* AppendBuffer
		) = 0;

		struct Require
		{
			void* OutputBuffer = nullptr;
			unsigned long long BufferSize = 0;
			bool ClearnessSize = false;
		};

		struct Respond
		{
			Require LastRequire;
			unsigned long long ReadedSize = 0;
		};

		virtual void ReadProtocol(
			Require(*Executer)(
			void* Object,
				unsigned long long Step,
				std::error_code const& EC,
				Respond Respond
				), 
				void* Object
		) = 0;
	};

	/*


	struct TcpSocketExecuter
	{
		TcpSocketExecuter(asio::io_context& Con) : Socket(Con), Resolver(Con) {}	
		
		template<typename ResFunction>
		void Connect(std::u8string_view Host, std::u8string_view Service, ResFunction Func)
			requires(std::is_invocable_v<ResFunction, std::error_code const&>)
		{
			Resolver.async_resolve(
				{ reinterpret_cast<char const*>(Host.data()), Host.size() },
				{ reinterpret_cast<char const*>(Service.data()), Service.size() },
				[this, Func = std::move(Func)](std::error_code const& EC, asio::ip::tcp::resolver::results_type RR)
				{
					if (!EC && RR.size() >= 1)
					{
						std::vector<asio::ip::tcp::endpoint> EndPoints;
						EndPoints.resize(RR.size() - 1);
						auto Ite = RR.begin();
						auto Top = *Ite;
						for (++Ite; Ite != RR.end(); ++Ite)
							EndPoints.push_back(*Ite);
						std::reverse(EndPoints.begin(), EndPoints.end());
						CallConnect(Top, std::move(EndPoints), std::move(Func));
					}
					else {
						Func(EC);
					}
				}
			);
		}

		template<typename ResFunction>
		void CallConnect(asio::ip::tcp::endpoint CurEndPoint, std::vector<asio::ip::tcp::endpoint> LastEndpoint, ResFunction Func)
		{
			Socket.async_connect(CurEndPoint, [LastEndpoint = std::move(LastEndpoint), Func = std::move(Func), CurEndPoint, this](std::error_code const& EC) mutable {
				if (!EC)
				{
					Func(EC);
				}
				else if (EC != asio::error::operation_aborted && !LastEndpoint.empty()) {
					CurEndPoint = std::move(*LastEndpoint.rbegin());
					LastEndpoint.pop_back();
					CallConnect(std::move(CurEndPoint), std::move(LastEndpoint), std::move(Func));
				}
				else {
					Func(EC);
				}
			});
		}
		template<typename ResFunction>
		void Send(std::span<std::byte const> SendBuffer, std::size_t LastSend, ResFunction Func)
			requires(std::is_invocable_v<ResFunction, std::error_code const&, std::size_t>)
		{
			Socket.async_write_some(
				asio::const_buffer{
					static_cast<void const*>(SendBuffer.data()),
					SendBuffer.size()
				},
				[this, Func = std::move(Func), SendBuffer, LastSend](std::error_code const& EC, std::size_t Sended)
				{
					if (!EC && Sended < SendBuffer.size())
					{
						Send(SendBuffer.subspan(Sended), LastSend + Sended, std::move(Func));
					}
					else {
						Func(EC, LastSend + Sended);
					}
				}
			);
		}
		
		template<typename ResFunction>
		void ReceiveSome(std::span<std::byte> PersistenceBuffer, ResFunction Func)
			requires(std::is_invocable_v<ResFunction, std::error_code const&, std::size_t>)
		{
			Socket.async_read_some(
				asio::mutable_buffer{
					PersistenceBuffer.data(),
					PersistenceBuffer.size()
				},
				Func
			);
		}

		void Close();
		void Cancel();

	protected:
		
		asio::ip::tcp::socket Socket;
		asio::ip::tcp::resolver Resolver;
	};
	*/
}