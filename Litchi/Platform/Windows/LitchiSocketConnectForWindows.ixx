module;

#include <winsock2.h>

export module LitchiSocketForWindows;

import LitchiSocket;
import Potato;

export namespace Litchi
{
	struct TCPSocketForWindows : public TCPSocket, public Potato::IR::MemoryResourceRecordIntrusiveInterface
	{
		TCPSocketForWindows(Potato::IR::MemoryResourceRecord record) : MemoryResourceRecordIntrusiveInterface(record) {}
		virtual void AddTCPSocketRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubTCPSocketRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
		~TCPSocketForWindows();
		SOCKET self_socket = INVALID_SOCKET;
	};
}


//#include "LitchiSocketExecutor.h"

/*
namespace Litchi
{
	void TcpSocketExecuter::Close()
	{
		Resolver.cancel();
		Socket.close();
	}

	void TcpSocketExecuter::Cancel()
	{
		Resolver.cancel();
		Socket.cancel();
	}
}
*/