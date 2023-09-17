module;

#include "AsioWrapper/LitchiAsioWrapper.h"

export module LitchiContext;

import std;
import PotatoPointer;
import PotatoTaskSystem;

export namespace Litchi::ErrorCode
{
	std::error_code const& BadAllocateErrorCode();
}


export namespace Litchi
{

	struct Context : public Potato::Task::Task
	{

		using Ptr = Potato::Task::ControlPtr<Context>;

		static Ptr Create(Potato::Task::TaskContext::Ptr LinkedTaskContext, std::size_t TaskCount = 1, std::size_t Priority = *Potato::Task::TaskPriority::Normal, std::u8string_view TaskName = u8"AsioContext", std::pmr::memory_resource* IMemoryResource = std::pmr::get_default_resource());

		AsioWrapper::Context& GetContextImp() { return *ContextPtr; }

		void AddRequest();
		void SubRequest();

		std::u8string_view GetTaskName() const{ return TaskName; }
		std::size_t GetPriority() const { return Priority; }

	protected:

		virtual void Release() override;
		virtual void operator()(Potato::Task::ExecuteStatus Status, Potato::Task::TaskContext& Context) override;

		Context(Potato::Task::TaskContext::Ptr LinkedTaskContext, std::size_t TaskCount, void* Adress, std::size_t Priority, std::u8string_view TaskName, std::pmr::memory_resource* IMResource);
		~Context();

		std::size_t const Priority;
		std::u8string_view const TaskName;
		
		std::pmr::memory_resource* IMResource = nullptr;
		AsioWrapper::Context* ContextPtr = nullptr;
		Potato::Task::TaskContext::Ptr LinkedTaskContext;

		std::mutex ContextMutex;
		std::size_t CurrentRequest = 0;
		std::size_t RequestTaskCount = 1;
		std::size_t RunningTaskCount = 0;
	};

	template<typename FunT, typename PtrT>
	struct TemporaryBlockKeeper
	{
		FunT Func;
		PtrT Ptr;
		std::pmr::memory_resource* Resource;
	};

	template<typename TFunc, typename PtrT>
	auto CreateTemporaryBlockKeeper(TFunc&& Func, PtrT&& Ptr, std::pmr::memory_resource* Resource)
		-> TemporaryBlockKeeper<std::remove_cvref_t<TFunc>, std::remove_cvref_t<PtrT>>*
	{
		using Type = TemporaryBlockKeeper<std::remove_cvref_t<TFunc>, std::remove_cvref_t<PtrT>>;
		if(Ptr && Resource != nullptr)
		{
			auto Adress = Resource->allocate(sizeof(Type), alignof(Type));
			if(Adress != nullptr)
			{
				return new (Adress) Type{ std::forward<TFunc>(Func), std::forward<PtrT>(Ptr), Resource};
			}
		}
		return nullptr;
	}

	template<typename TFunc, typename PtrT>
	bool DestroyTemporaryBlockKeeper(TemporaryBlockKeeper<TFunc, PtrT>* Block)
	{
		if(Block != nullptr)
		{
			using Type = TemporaryBlockKeeper<TFunc, PtrT>;
			auto OldResource = Block->Resource;
			Block->~TemporaryBlockKeeper();
			OldResource->deallocate(Block, sizeof(Type), alignof(Type));
			return true;
		}
		return false;
	}

	/*
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
	*/
}

export namespace Litchi::ErrorCode
{
	struct LitchiErrorCode
		//: public std::error_category
	{
		
	};
}