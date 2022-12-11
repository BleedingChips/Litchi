#include "Litchi/LitchiHttpConnect.h"

namespace Litchi
{
	auto HttpConnection::Create(std::u8string_view Host, uint16_t Port) -> Ptr
	{
		Ptr P{new HttpConnection{} };
		if (P)
		{
			asio::ip::tcp::resolver Resolver(P->Context);
			auto Re = Resolver.resolve(std::string_view{ reinterpret_cast<char const*>(Host.data()), Host.size()}, "http");
			for (auto& Ite : Re)
			{
				std::error_code EC;
				P->Socket.connect(Ite, EC);
				if(!EC)
					return P;
			}
		}
		return {};
	}

	HttpConnection::~HttpConnection()
	{
		if (Socket.is_open())
		{
			Socket.close();
		}
	}
}