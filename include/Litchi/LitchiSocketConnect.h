#pragma once
#include "asio.hpp"
#include <string_view>
namespace Litchi
{
	
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


}