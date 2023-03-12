module;

export module Litchi.Context;

export import Potato.STD;
export import Litchi.Memory;
export import Litchi.Socket;
export import Litchi.Http;

export namespace Litchi
{

	struct Context
	{
		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;
		virtual ~Context();

		using PtrT = Potato::Misc::IntrusivePtr<Context>;

		static auto CreateBackEnd(std::size_t ThreadCount = 1, AllocatorT<Context> Allocator = {}) -> PtrT;
		
		virtual Socket CreateIpTcpSocket() = 0;
		virtual Http11 CreateHttp11(AllocatorT<std::byte> Allocator = {}) = 0;
	};


	/*
	struct Context
	{
		template<typename AllocatorT = std::allocator<std::thread>>
		struct MulityThreadAgency
		{
			MulityThreadAgency(std::size_t InThreadCount = 1, AllocatorT const& Allocator = AllocatorT{})
				: ThreadCount(std::max(InThreadCount, std::size_t{ 1 })), IOContext(static_cast<int>(InThreadCount)) Threads(Allocator)
			{
				Work.emplace(IOContext.get_executor());
				for (std::size_t I = 0; I < ThreadCount; ++I)
				{
					Threads.emplace_back(
						[this]() { IOContext.run(); }
					);
				}
			}

			~MulityThreadAgency()
			{
				//Work.reset();
				//IOContext.stop();
				for (auto& Ite : Threads)
					Ite.join();
				Threads.clear();
			}

			//asio::io_context& GetIOContext() { return IOContext; }
			std::size_t GetThreadCount() const { return ThreadCount; }
		protected:
			const std::size_t ThreadCount;
			//asio::io_context IOContext;
			//std::optional<asio::executor_work_guard<decltype(IOContext.get_executor())>> Work;
			std::vector<std::thread, AllocatorT> Threads;
		};
	};

	struct IntrusiveObjInterface
	{
	protected:
		mutable Potato::Misc::AtomicRefCount Ref;
		void AddRef() const { Ref.AddRef(); }
		void SubRef() const { if(Ref.SubRef()) delete this; }
		virtual ~IntrusiveObjInterface() = default;
		friend struct IntrusiceObjWrapper;
	};

	struct IntrusiceObjWrapper
	{
		template<typename ObjecT>
		void AddRef(ObjecT* Ptr) { static_cast<IntrusiveObjInterface const*>(Ptr)->AddRef(); }
		template<typename ObjecT>
		void SubRef(ObjecT* Ptr) { static_cast<IntrusiveObjInterface const*>(Ptr)->SubRef(); }
	};

	template<typename ObjectT>
	using IntrusivePtr = Potato::Misc::IntrusivePtr<ObjectT, IntrusiceObjWrapper>;
	*/
}