module;

#include <cassert>
#include <cerrno>

module LitchiContext;

namespace Litchi::ErrorCode
{
	std::error_code const& BadAllocateErrorCode()
	{
		static std::error_code EC{ ENOMEM, std::system_category() };
		return EC;
	}
}

namespace Litchi
{

	Context::Context(Potato::Task::TaskContext::Ptr LinkedTaskContext, std::size_t TaskCount, void* Adress, std::size_t Priority, std::u8string_view TaskName, std::pmr::memory_resource* IMResource)
		: IMResource(IMResource), ContextPtr(AsioWrapper::Context::Construct(Adress)),
	LinkedTaskContext(std::move(LinkedTaskContext)), RequestTaskCount(std::max(TaskCount, std::size_t{1})),
	Priority(Priority), TaskName(TaskName)
	{

	}

	auto Context::Create(Potato::Task::TaskContext::Ptr LinkedTaskContext, std::size_t TaskCount, std::size_t Priority, std::u8string_view TaskName, std::pmr::memory_resource* IMemoryResource)
	-> Ptr
	{
		if (LinkedTaskContext && IMemoryResource != nullptr)
		{
			auto Layout = AsioWrapper::Context::GetLayout();
			assert(Layout.Align == alignof(Context));
			auto MPtr = IMemoryResource->allocate(
				Layout.Size + sizeof(Context),
				alignof(Context)
			);
			if (MPtr != nullptr)
			{
				auto P = reinterpret_cast<std::byte*>(MPtr) + sizeof(Context);
				Ptr ConAdress {new (MPtr) Context{std::move(LinkedTaskContext), TaskCount, P, Priority, TaskName, IMemoryResource}};
				return ConAdress;
			}
		}
		return {};
	}

	void Context::Release()
	{
		auto OResource = IMResource;
		this->~Context();
		auto Layout = AsioWrapper::Context::GetLayout();
		OResource->deallocate(this, Layout.Size + sizeof(Context), alignof(Context));
	}

	Context::~Context()
	{
		assert(ContextPtr != nullptr);
		ContextPtr->~Context();
	}

	void Context::operator()(Potato::Task::ExecuteStatus& Status)
	{
		while(GetContextImp().PollOne())
		{
			
		}

		if(ContextMutex.try_lock())
		{
			std::lock_guard lg(ContextMutex, std::adopt_lock);
			if(CurrentRequest >= RunningTaskCount)
			{
				Status.Context.CommitDelayTask(this, std::chrono::system_clock::now() + std::chrono::milliseconds{ 1 }, {GetPriority(), GetTaskName()});
			}else
			{
				RunningTaskCount -= 1;
			}
		}
	}

	void Context::AddRequest()
	{
		std::lock_guard lg(ContextMutex);
		CurrentRequest += 1;
		if(RunningTaskCount < RequestTaskCount)
		{
			assert(LinkedTaskContext);
			++RunningTaskCount;
			LinkedTaskContext->CommitTask(this, {GetPriority(), GetTaskName()});
		}
	}

	void Context::SubRequest()
	{
		std::lock_guard lg(ContextMutex);
		assert(CurrentRequest >= 1);
		CurrentRequest -= 1;
	}


	/*
	template<typename T>
	using AllocatorT = Potato::Misc::AllocatorT<T>;

	Context::~Context() {}

	ErrorT Translate(std::error_code const& EC)
	{
		if (!EC)
			return ErrorT::None;
		else if (EC == asio::error::eof)
			return ErrorT::Eof;
		else if (EC == asio::error::host_not_found)
			return ErrorT::HostNotFound;
		else {
			auto K = EC.message();
			return ErrorT::Unknow;
		}
	}

	template<typename Interface, typename Core>
	struct SocketHolder : public AllocatorT<SocketHolder<Interface, Core>>, public Interface, public Core
	{
		template<typename ...AppendPars>
		SocketHolder(AllocatorT<SocketHolder> Allo, Context::PtrT ContextPtr, asio::io_context& Context, AppendPars&& ...AP)
			: AllocatorT<SocketHolder>(std::move(Allo)), Interface(std::forward<AppendPars>(AP)...), ContextPtr(std::move(ContextPtr)), Core(Context) {}
		Context::PtrT ContextPtr;

		virtual void Release() const override
		{
			AllocatorT<SocketHolder<Interface, Core>> Allo = std::move(*this);
			Context::PtrT TempCon = std::move(ContextPtr);
			this->~SocketHolder();
			Allo.deallocate(const_cast<SocketHolder*>(this), 1);
		}

		virtual void CloseExe() override { Core::Close(); }
		virtual void CancelExe() override { Core::Cancel(); }
		virtual void ConnectExe(std::u8string_view Host, std::u8string_view Service, std::function<void(ErrorT)> Func) override
		{
			Core::Connect(Host, Service, [Func = std::move(Func)](std::error_code const& EC) {
				return Func(Translate(EC));
			});
		}

		virtual void SendExe(std::span<std::byte const> SendBuffer, std::function<void(ErrorT, std::size_t)> Func) override
		{
			Core::Send(SendBuffer, 0, [Func = std::move(Func)](std::error_code const& EC, std::size_t Count) {
				return Func(Translate(EC), Count);
			});
		}

		virtual void ReceiveSomeExe(std::span<std::byte> PersistenceBuffer, std::function<void(ErrorT, std::size_t)> Func) override
		{
			Core::ReceiveSome(PersistenceBuffer, [Func = std::move(Func)](std::error_code const& EC, std::size_t Count){
				return Func(Translate(EC), Count);
			});
		}

	};

	struct ContextBackEnds : public AllocatorT<ContextBackEnds>, public Context
	{

		ContextBackEnds(AllocatorT<ContextBackEnds> InputAllocator, std::size_t ThreadCount) :
			AllocatorT<ContextBackEnds>(InputAllocator), Threads(InputAllocator)
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

		virtual Socket CreateIpTcpSocket() override
		{
			AllocatorT<SocketHolder<SocketAgency, TcpSocketExecuter>> Allo = *this;
			auto P = Allo.allocate(1);
			auto K = new (P) SocketHolder<SocketAgency, TcpSocketExecuter>{std::move(Allo), this, IOContext};
			return Socket{ SocketAgency ::PtrT{K}};
		}

		virtual Http11 CreateHttp11(AllocatorT<std::byte> Allocator) override
		{
			AllocatorT<SocketHolder<Http11Agency, TcpSocketExecuter>> Allo = *this;
			auto P = Allo.allocate(1);
			auto K = new (P) SocketHolder<Http11Agency, TcpSocketExecuter>{std::move(Allo), this, IOContext, std::move(Allocator)};
			return Http11{ Http11Agency::PtrT{K} };
		}

	protected:

		mutable Potato::Misc::AtomicRefCount Ref;
		asio::io_context IOContext;
		std::vector<std::thread, AllocatorT<std::thread>> Threads;
		std::optional<asio::executor_work_guard<decltype(IOContext.get_executor())>> Work;

		virtual void AddRef() const override { Ref.AddRef(); }
		virtual void SubRef() const override { 
			if(Ref.SubRef()){
				AllocatorT<ContextBackEnds> TemAllo = std::move(*this);
				this->~ContextBackEnds();
				TemAllo.deallocate(const_cast<ContextBackEnds*>(this), 1);
			}
		}
	};

	auto Context::CreateBackEnd(std::size_t ThreadCount, AllocatorT<Context> InputAllocator) -> PtrT
	{
		AllocatorT<ContextBackEnds> TempA{ std::move(InputAllocator) };
		auto cur = TempA.allocate(1);
		PtrT Ptr = new (cur) ContextBackEnds(std::move(TempA), ThreadCount);
		return Ptr;
	}
	*/

}

