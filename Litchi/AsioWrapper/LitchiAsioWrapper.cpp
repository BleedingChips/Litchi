#include <cassert>
#include <memory_resource>
#include <span>
#include "asio.hpp"
#include "LitchiAsioWrapper.h"


namespace Litchi::AsioWrapper
{


	struct ContextImp : public Context
	{
		virtual bool PollOne() override { return Context.poll_one() != 0; }
		virtual void Cancel() override { Context.stop(); }
		asio::io_context Context;
	};

	Layout Context::GetLayout()
	{
		return Layout{
			sizeof(ContextImp),
			alignof(ContextImp)
		};
	}

	Context* Context::Construct(void* Adress)
	{
		if(Adress != nullptr)
		{
			return new(Adress) ContextImp{};
		}
		return nullptr;
	}

	struct TCPSocketImp : public TCPSocket
	{
		TCPSocketImp(ContextImp& Context)
			: Resolver(Context.Context), Socket(Context.Context)
		{}

		virtual void Connect(
			char8_t const* Host, unsigned long HostSize, char8_t const* Server, unsigned long ServerSize,
			void (*Executer)(void* AppendData, std::error_code const&), void* AppendBuffer, std::pmr::memory_resource* Resource
		) override
		{
			Resolver.async_resolve(
				std::string_view{ reinterpret_cast<char const*>(Host), HostSize },
				std::string_view{ reinterpret_cast<char const*>(Server), ServerSize },
				[Executer, AppendBuffer, Resource, this](std::error_code const& EC, asio::ip::tcp::resolver::results_type EndPoints)
				{
					auto Array = Resource->allocate(sizeof(asio::ip::tcp::socket::endpoint_type) * EndPoints.size(), alignof(asio::ip::tcp::socket::endpoint_type));
					if(Array != nullptr)
					{
						auto EArray = new (Array) asio::ip::tcp::socket::endpoint_type[EndPoints.size()];
						std::size_t Count = 0;
						for (auto Ite : EndPoints)
						{
							assert(Count < EndPoints.size());
							new (EArray + Count) asio::ip::tcp::socket::endpoint_type{Ite.endpoint()};
							++Count;
						}
						CallConnect(
							EArray,
							0, EndPoints.size(), Executer, AppendBuffer, Resource
						);
					}else
					{
						std::error_code ErrorCode{ ENOMEM, std::system_category() };
						Executer(AppendBuffer, ErrorCode);
					}
				}
			);
		}

		void CallConnect(
			asio::ip::tcp::socket::endpoint_type* Array,
			std::size_t CurIndex,
			std::size_t TotalIndex,
			void (*Executer)(void* AppendData, std::error_code const&), void* AppendBuffer, std::pmr::memory_resource* Resource
		)
		{
			if(CurIndex < TotalIndex)
			{
				Socket.async_connect(
					Array[CurIndex],
					[CurIndex, Array, TotalIndex, Executer, AppendBuffer, Resource, this](std::error_code const& EC) mutable
					{
						CurIndex += 1;
						if(!EC || CurIndex >= TotalIndex)
						{
							for(std::size_t I = 0; I < TotalIndex; ++I)
							{
								Array[I].~basic_endpoint();
							}
							Resource->deallocate(Array, sizeof(asio::ip::tcp::socket::endpoint_type) * TotalIndex, alignof(asio::ip::tcp::socket::endpoint_type));
						}

						if(!EC)
						{
							Executer(AppendBuffer, EC);
						}else
						{
							if(CurIndex <= TotalIndex)
							{
								CallConnect(Array, CurIndex, TotalIndex, Executer, AppendBuffer, Resource);
							}else
							{
								Executer(AppendBuffer, EC);
							}
						}
					}
				);
			}
		}

		virtual void Send(
			void const* Data, unsigned long long DataCount,
			void (*Executer)(void* AppendData, std::error_code const&, unsigned long long), void* AppendBuffer
		) override
		{
			SendImp(0, Data, DataCount, Executer, AppendBuffer);
		}

		virtual void ReceiveSome(
			void* Data, unsigned long long DataCount,
			void (*Executer)(void* AppendData, std::error_code const&, unsigned long long), void* AppendBuffer
		) override
		{
			Socket.async_read_some(
				asio::mutable_buffer{
					Data,
					DataCount
				},
				[Executer, AppendBuffer](std::error_code const& EC, std::size_t Readed)
				{
					Executer(AppendBuffer, EC, Readed);
				}
			);
		}

		virtual void SendImp(
			std::size_t SendedSize,
			void const* Data, unsigned long long DataCount,
			void (*Executer)(void* AppendData, std::error_code const&, unsigned long long), void* AppendBuffer
			)
		{
			Socket.async_send(
				asio::const_buffer{
					Data,
					DataCount
				},
				[Executer, AppendBuffer, Data, DataCount, this, SendedSize](std::error_code const& EC, std::size_t Sended)
				{
					if(EC || DataCount >= Sended)
					{
						Executer(AppendBuffer, EC, SendedSize + Sended);
					}else
					{
						SendImp(
							SendedSize + Sended,
							reinterpret_cast<std::byte const*>(Data) + Sended,
							DataCount - Sended,
							Executer, AppendBuffer
						);
					}
				}
			);
		}

		virtual ~TCPSocketImp()
		{
			Resolver.cancel();
			if(Socket.is_open())
				Socket.close();
		}

		asio::ip::tcp::resolver Resolver;
		asio::ip::tcp::socket Socket;
	};

	Layout TCPSocket::GetLayout()
	{
		return Layout{
			sizeof(TCPSocketImp),
			alignof(TCPSocketImp)
		};
	}

	TCPSocket* TCPSocket::Construct(void* Adress, Context& Context, std::pmr::memory_resource* Resource)
	{
		if (Adress != nullptr)
		{
			return new(Adress) TCPSocketImp{static_cast<ContextImp&>(Context)};
		}
		return nullptr;
	}
}

namespace Litchi
{
}
