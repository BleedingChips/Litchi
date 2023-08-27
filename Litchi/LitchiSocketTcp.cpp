module;

#include <cassert>

module LitchiSocketTcp;


namespace Litchi::TCP
{
	auto Socket::Create(Context::Ptr Owner, std::pmr::memory_resource* IMemoryResource)
		-> Ptr
	{
		if (Owner && IMemoryResource != nullptr)
		{
			auto Layout = AsioWrapper::TCPSocket::GetLayout();
			assert(Layout.Align == alignof(Socket));
			auto MPtr = IMemoryResource->allocate(
				Layout.Size + sizeof(Socket),
				alignof(Socket)
			);
			if (MPtr != nullptr)
			{
				auto P = reinterpret_cast<std::byte*>(MPtr) + sizeof(Socket);
				auto ResoAdress = new Socket{ std::move(Owner), P, IMemoryResource };
				return Ptr{ ResoAdress };
			}
		}
		return {};
	}

	Socket::Socket(Context::Ptr Owner, void* Adress, std::pmr::memory_resource* IMResource)
		: IMResource(IMResource), Owner(Owner), SocketPtr(AsioWrapper::TCPSocket::Construct(Adress, Owner->GetContextImp(), IMResource))
	{

	}

	void Socket::Release()
	{
		auto OResource = IMResource;
		this->~Socket();
		auto Layout = AsioWrapper::TCPSocket::GetLayout();
		OResource->deallocate(this, Layout.Size + sizeof(Socket), alignof(Socket));
	}

	Socket::~Socket()
	{
		assert(SocketPtr != nullptr);
		SocketPtr->~TCPSocket();
	}
}