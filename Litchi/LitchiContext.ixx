module;

export module LitchiContext;

import std;
import PotatoIR;
import PotatoPointer;
import LitchiSocket;
import LitchiHttp;


export namespace Litchi
{

	struct Context
	{
		struct Wrapper
		{
			void AddRef(Context const* ptr) const { ptr->AddContextRef(); };
			void SubRef(Context const* ptr) const { ptr->SubContextRef(); };
		};
		
		~Context() = default;

		using Ptr = Potato::Pointer::IntrusivePtr<Context, Wrapper>;

		static Ptr Create(std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		virtual TCPSocket::Ptr CreateTCPSocket(std::u8string_view host, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) = 0;
		
		//virtual Socket CreateIpTcpSocket() = 0;
		//virtual Http11 CreateHttp11(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) = 0;

	protected:

		virtual void AddContextRef() const = 0;
		virtual void SubContextRef() const = 0;

		//asio::io_context context;
	};
}
