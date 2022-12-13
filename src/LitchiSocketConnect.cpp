#include "Litchi/LitchiSocketConnect.h"

namespace Litchi
{
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
}