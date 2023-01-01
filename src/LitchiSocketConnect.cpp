#include "Litchi/LitchiSocketConnect.h"

namespace Litchi
{
	void TcpSocket::Agency::Cancle()
	{
		auto lg = std::lock_guard(SocketMutex);
		Socket.cancel();
	}

	void TcpSocket::Agency::Close()
	{
		auto lg = std::lock_guard(SocketMutex);
		Socket.cancel();
		Socket.close();
	}
}