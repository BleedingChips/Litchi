module;

#include "asio.hpp"
#include "LitchiSocketExecutor.h"

module Litchi.Context;


namespace Litchi
{
	Context::~Context() {}

	struct IpTcpSocket : public SocketAgency, public TcpSocketExecuter
	{

	};

	struct ContextHolder : public Context
	{
		asio::io_context IOContext;

		Socket CreateIpTcpSocket() = 0;
	};


	struct ContextBackEnds : public ContextHolder
	{

		ContextBackEnds(Allocator<ContextBackEnds> InputAllocator, std::size_t ThreadCount) :
			InsideAllocator(InputAllocator), Threads(InputAllocator)
		{
			ThreadCount = std::max(std::size_t{ 1 }, ThreadCount);
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

		Allocator<ContextBackEnds> InsideAllocator;
		mutable Potato::Misc::AtomicRefCount Ref;
		asio::io_context IOContext;
		std::vector<std::thread, Allocator<std::thread>> Threads;
		std::optional<asio::executor_work_guard<decltype(IOContext.get_executor())>> Work;

		virtual void AddRef() const override { Ref.AddRef(); }
		virtual void SubRef() const override { Ref.SubRef(); }
	};

	auto Context::CreateBackEnd(std::size_t ThreadCount, Allocator<Context> InputAllocator) -> PtrT
	{
		Allocator<ContextBackEnds> TempA{ std::move(InputAllocator) };
		auto cur = TempA.allocate(1);
		PtrT Ptr = new ContextBackEnds(std::move(TempA), ThreadCount);
		return Ptr;
	}

}

