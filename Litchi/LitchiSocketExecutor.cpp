#include "LitchiSocketExecutor.h"

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