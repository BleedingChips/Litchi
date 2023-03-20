module;

export module Litchi.Context;

export import Potato.STD;
export import Potato.Allocator;
export import Litchi.Socket;
export import Litchi.Http;

export namespace Litchi
{

	struct Context
	{
		template<typename T>
		using AllocatorT = Potato::Misc::AllocatorT<T>;

		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;
		virtual ~Context();

		using PtrT = Potato::Misc::IntrusivePtr<Context>;

		static auto CreateBackEnd(std::size_t ThreadCount = 1, AllocatorT<Context> Allocator = {}) -> PtrT;
		
		virtual Socket CreateIpTcpSocket() = 0;
		virtual Http11 CreateHttp11(AllocatorT<std::byte> Allocator = {}) = 0;
	};
}