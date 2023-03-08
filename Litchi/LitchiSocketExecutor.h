#pragma once
#include <asio/ip/tcp.hpp>
#include <span>
#include <functional>
#include <system_error>

import Litchi.Socket;

namespace Litchi
{
	struct TcpSocketExecuter
	{
		TcpSocketExecuter(asio::io_context const& Con) : Socket(Con), Resolver(Con) {}
		asio::ip::tcp::socket Socket;
		asio::ip::tcp::resolver Resolver;
		void ConnectExe(std::u8string_view Host, std::u8string_view Service, std::function<void(std::error_code const& EC)>);
		void CallConnect(asio::ip::tcp::endpoint CurEndPoint, std::vector<asio::ip::tcp::endpoint> LastEndpoint, std::function<void(std::error_code const&)>);
		void SendExe(std::span<std::byte const> SendBuffer, std::size_t LastSend, std::function<void(std::error_code const&, std::size_t)>);
		void ReceiveSomeExe(std::span<std::byte> PersistenceBuffer, std::function<void(std::error_code const&, std::size_t)>);
		void CloseExe();
		void CancleExe();
	};
}