module;

#include "asio.hpp"

module Litchi.Context;

namespace Litchi
{
	Context::~Context() {}


	struct ContextBackEnds : public Context
	{

		ContextBackEnds(InstanceAllocator<ContextBackEnds> InputAllocator, std::size_t ThreadCount) :
			Allocator(InputAllocator), Threads(InputAllocator)
		{
			ThreadCount = std::max(1, ThreadCount);
			Threads.reserve(ThreadCount);
			Work.emplace(IOContext.get_executor());
			for (std::size_t I = 0; I < ThreadCount; ++I)
			{
				Threads.emplace_back(
					[this]() { IOContext.run(); }
				);
			}
		}

		~ContextBackEnds()
		{
			Work.reset();
			IOContext.stop();
			for (auto& Ite : Threads)
				Ite.join();
			Threads.clear();
		}

	protected:

		InstanceAllocator<ContextBackEnds> Allocator;
		mutable Potato::Misc::AtomicRefCount Ref;
		asio::io_context IOContext;
		std::vector<std::thread, InstanceAllocator<std::thread>> Threads;
		std::optional<asio::executor_work_guard<decltype(IOContext.get_executor())>> Work;

		virtual void AddRef() const { Ref.AddRef(); }
		virtual void SubRef() const { Ref.SubRef(); }
	};

	auto Context::CreateBackEnd(std::size_t ThreadCount, InstanceAllocator<Context> Allocator) -> PtrT
	{
		
		InstanceAllocator<ContextBackEnds> TempA(Allocator);

		auto cur = TempA.allocate(1);
		PtrT Ptr = new ContextBackEnds(std::move(TempA), ThreadCount);
		return Ptr;
	}

}

