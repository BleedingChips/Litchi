#include "Litchi/LitchiSocketConnect.h"

namespace Litchi
{
	namespace Tcp
	{
		SocketAngency::SocketAngency(Context& Context)
			: Socket(Context.IOContext), Strand(Context.IOContext)
		{

		}

		SocketAngency::~SocketAngency()
		{
			Close();
		}

		auto SocketAngency::Create(Context& Context) -> Ptr
		{
			Ptr RePtr = new SocketAngency{ Context };
			return RePtr;
		}

		auto SocketAngency::Connect(std::u8string_view Host, std::u8string_view Service) -> std::future<std::error_code>
		{
			ResolverAgency::Ptr Resolver = new ResolverAgency{Strand.context()};
			assert(Resolver);
			std::promise<std::error_code> Promise;
			auto Future = Promise.get_future();
			auto TResolver = Resolver.Data();
			assert(TResolver != nullptr);
			TResolver->Resolver.async_resolve(
				std::string_view{reinterpret_cast<char const*>(Host.data()), Host.size()},
				std::string_view{ reinterpret_cast<char const*>(Service.data()), Service.size() },
				[This = SocketAngency::Ptr{this}, Resolver = std::move(Resolver), Promise = std::move(Promise)](std::error_code EC, decltype(Resolver->Resolver)::results_type Result) mutable {
					if (!EC)
					{
						assert(Resolver);
						std::size_t Count = 0;
						for (auto& Ite : Result)
						{
							if (Count < Resolver->EndPoints.size())
							{
								Resolver->EndPoints[Count++] = Ite;
							}else
								break;
						}
						Resolver->AvailableEndPointCount = Count;
						if (Count != 0)
						{
							SocketAngency::TryConnect(0, std::move(This), std::move(Resolver), std::move(Promise));
						}
						else {
							Promise.set_value(EC);
						}
					}else
						Promise.set_value(EC);
				}
			);
			return Future;
		}

		void SocketAngency::TryConnect(std::size_t EndPointeIndex, SocketAngency::Ptr This, ResolverAgency::Ptr Resolver, std::promise<EC> Promise)
		{
			assert(This && Resolver);

			auto TThis = This.Data();

			assert(TThis);

			auto TempFunc = [EndPointeIndex, This = std::move(This), Resolver = std::move(Resolver), Promise = std::move(Promise)]() mutable {
				assert(This && Resolver);
				auto Top = Resolver->EndPoints[EndPointeIndex++];
				auto TThis = This.Data();
				TThis->Socket.async_connect(Top, [EndPointeIndex, This = std::move(This), Resolver = std::move(Resolver), Promise = std::move(Promise)](std::error_code EC) mutable{
					if (!EC)
					{
						Promise.set_value(EC);
					}
					else {
						assert(Resolver);
						if (EndPointeIndex < Resolver->AvailableEndPointCount)
						{
							TryConnect(EndPointeIndex, std::move(This), std::move(Resolver), std::move(Promise));
						}
						else {
							Promise.set_value(EC);
						}
					}
				});
			};

			TThis->Strand.post(std::move(TempFunc), std::allocator<decltype(TempFunc)>{});
		}

		auto SocketAngency::Close() -> std::future<bool>
		{
			std::promise<bool> Promise;
			auto Future = Promise.get_future();
			auto TempFunc = [This = SocketAngency::Ptr{this}, Promise = std::move(Promise)]() mutable {
				assert(This);
				if (This->Socket.is_open())
				{
					This->Socket.close();
					Promise.set_value(true);
				}
				else
					Promise.set_value(false);
			};
			Strand.post(std::move(TempFunc), std::allocator<decltype(TempFunc)>{});
			return Future;
		}
	}
	

	/*
	bool TcpSocketConnection::Connect(std::u8string_view Host, std::u8string_view Service)
	{
		asio::ip::tcp::resolver Resolver(Context);
		auto Re = Resolver.resolve(std::string_view{ reinterpret_cast<char const*>(Host.data()), Host.size()}, std::string_view{ reinterpret_cast<char const*>(Service.data()), Service.size() });
		for (auto& Ite : Re)
		{
			std::error_code EC;
			Socket.connect(Ite, EC);
			if(!EC)
				return true;
		}
		return false;
	}

	bool TcpSocketConnection::Close()
	{
		if (Socket.is_open())
		{
			Socket.close();
			return true;
		}
		return false;
	}

	TcpSocketConnection::~TcpSocketConnection()
	{
		Close();
	}
	*/
}