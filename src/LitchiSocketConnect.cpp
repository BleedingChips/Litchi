#include "Litchi/LitchiSocketConnect.h"

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
				this->AsyncConnect(std::move(EP), [&](std::error_code& EC, EndPointT, Agency&){
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
				this->AsyncConnect(Host, Service, [&](std::error_code& EC, EndPointT EP, Agency&) {
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