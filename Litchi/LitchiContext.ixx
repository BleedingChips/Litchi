module;

export module LitchiContext;

import std;
import LitchiSocket;
import LitchiHttp;
import PotatoPointer;

export namespace Litchi
{

	struct Context
	{
		struct Wrapper
		{
			void AddRef(Context const* ptr) const { ptr->AddContextRef(); };
			void SubRef(Context const* ptr) const { ptr->SubContextRef(); };
		};
		
		virtual ~Context() = default;

		using Ptr = Potato::Pointer::IntrusivePtr<Context>;

		static Ptr CreateBackEnd(std::size_t ThreadCount = 1, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		
		virtual Socket CreateIpTcpSocket() = 0;
		virtual Http11 CreateHttp11(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) = 0;
	protected:
		virtual void AddContextRef() const = 0;
		virtual void SubContextRef() const = 0;
	};
}