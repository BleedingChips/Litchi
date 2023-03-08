#include "LitchiSocketExecutor.h"

namespace Litchi
{
	void TcpSocketExecuter::ConnectExe(std::u8string_view Host, std::u8string_view Service, std::function<void(std::error_code const&)> Func)
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

	void TcpSocketExecuter::CallConnect(asio::ip::tcp::endpoint CurEndPoint, std::vector<asio::ip::tcp::endpoint> LastEndpoint, std::function<void(std::error_code const&)> Func)
	{
		Socket.async_connect(CurEndPoint, [LastEndpoint = std::move(LastEndpoint), Func = std::move(Func), CurEndPoint, this](std::error_code const& EC) mutable {
			if (!EC)
			{
				Func(EC);
			}
			else if(EC != asio::error::operation_aborted && !LastEndpoint.empty()) {
				CurEndPoint = std::move(*LastEndpoint.rbegin());
				LastEndpoint.pop_back();
				CallConnect(std::move(CurEndPoint), std::move(LastEndpoint), std::move(Func));
			}
			else {
				Func(EC);
			}
		});
	}

	void TcpSocketExecuter::SendExe(std::span<std::byte const> SendBuffer, std::size_t LastSend, std::function<void(std::error_code const&, std::size_t)> Func)
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
					SendExe(SendBuffer.subspan(Sended), LastSend + Sended, std::move(Func));
				}
				else {
					Func(EC, LastSend + Sended);
				}
			}
		);
	}

	void TcpSocketExecuter::ReceiveSomeExe(std::span<std::byte> PersistenceBuffer, std::function<void(std::error_code const&, std::size_t)> Func)
	{
		Socket.async_read_some(
			asio::mutable_buffer{
				PersistenceBuffer.data(),
				PersistenceBuffer.size()
			},
			Func
		);
	}

}

//#include "Litchi/LitchiSocketConnect.h"

/*
namespace Litchi
{
	auto TcpSocket::SyncConnect(EndPointT EP) -> std::optional<std::error_code>
	{
		std::promise<std::error_code> Promise;
		auto Fur = Promise.get_future();
		{
			auto lg = std::lock_guard(SocketMutex);
			if (AbleToConnect())
			{
				this->AsyncConnect(std::move(EP), [&](std::error_code const& EC, EndPointT, Agency&){
					Promise.set_value(EC);
				});
				return Fur.get();
			}
			else
			{
				return {};
			}
		}
	}

	auto TcpSocket::SyncConnect(std::u8string_view Host, std::u8string_view Service) -> std::optional<std::tuple<std::error_code, EndPointT>>
	{
		std::promise<std::tuple<std::error_code, EndPointT>> Promise;
		auto Fur = Promise.get_future();
		{
			auto lg = std::lock_guard(SocketMutex);
			if (AbleToConnect())
			{
				this->AsyncConnect(Host, Service, [&](std::error_code const& EC, EndPointT EP, Agency&) {
					Promise.set_value({EC, std::move(EP)});
				});
			}
			else
			{
				return {};
			}
		}
		return Fur.get();
	}

	auto TcpSocket::CloseExecute() -> void
	{
		Resolver.cancel();
		Socket.close();
		Connecting = false;
		Sending = false;
		Receiving = false;
	}

	auto TcpSocket::CancelExecute() -> void
	{
		Resolver.cancel();
		Socket.cancel();
		Connecting = false;
		Sending = false;
		Receiving = false;
	}

	auto TcpSocket::SyncCancel() -> void
	{
		auto lg = std::lock_guard(SocketMutex);
		CancelExecute();
	}
	auto TcpSocket::SyncClose() -> void
	{
		auto lg = std::lock_guard(SocketMutex);
		CloseExecute();
	}
}
*/