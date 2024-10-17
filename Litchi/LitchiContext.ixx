module;

#include "asio.hpp"

export module LitchiContext;

import std;
import PotatoIR;
import PotatoPointer;
import LitchiSocket;
import LitchiHttp;

export namespace Litchi
{

	struct Context : public Potato::IR::MemoryResourceRecordIntrusiveInterface
	{
		struct Wrapper
		{
			void AddRef(Context const* ptr) const { ptr->AddContextRef(); };
			void SubRef(Context const* ptr) const { ptr->SubContextRef(); };
		};
		
		~Context();

		using Ptr = Potato::Pointer::IntrusivePtr<Context>;

		static Ptr Create(std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		
		//virtual Socket CreateIpTcpSocket() = 0;
		//virtual Http11 CreateHttp11(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) = 0;

	protected:

		Context(Potato::IR::MemoryResourceRecord record) : MemoryResourceRecordIntrusiveInterface(record) {}
		virtual void AddContextRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubContextRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }

		asio::io_context context;
	};
}