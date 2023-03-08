module;


export module Litchi.Memory;

export import Potato.STD;
export import Potato.Misc;
export import Potato.SmartPtr;

export namespace Litchi
{
	struct AllocatorInterface
	{
		enum class MemoryType
		{
			TEMP,
			INSTANCE,
		};

		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;
		virtual ~AllocatorInterface();

		using Ptr = Potato::Misc::IntrusivePtr<AllocatorInterface>;

	protected:

		virtual std::byte* Allocate(MemoryType Type, std::size_t Align, std::size_t Size, std::size_t Count) = 0;
		virtual void Deallocate(MemoryType Type, std::byte const* MemoryBegin, std::size_t Align, std::size_t Size, std::size_t Count) = 0;
		virtual std::tuple<std::byte*, std::size_t> AllocateAtLast(MemoryType Type, std::size_t Align, std::size_t Size, std::size_t Count) = 0;

		template<typename Type>
		friend struct Allocator;
	};

	template<typename Type>
	struct Allocator : public std::allocator<Type>
	{
		using value_type = Type;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;

		constexpr Type* allocate(std::size_t Num) {
			if (!AllocatorPtr)
			{
				return std::allocator<Type>::allocate(Num);
			}
			else
				return reinterpret_cast<Type*>(AllocatorPtr->Allocate(
					MType,
					alignof(Type),
					sizeof(Type),
					Num
				));
		}

		constexpr void deallocate(Type* const Adress, std::size_t Num) {
			if (!AllocatorPtr)
			{
				return std::allocator<Type>::deallocate(Adress, Num);
			}
			else
				return AllocatorPtr->Deallocate(
					MType,
					reinterpret_cast<std::byte const*>(Adress),
					alignof(Type),
					sizeof(Type),
					Num
				);
		}

		constexpr std::allocation_result<Type*> allocate_at_least(std::size_t Num) {
			if (!AllocatorPtr)
			{
				return std::allocator<Type>::allocate_at_least(Num);
			}
			else
			{
				auto [Adress, Count] = AllocatorPtr->AllocateAtLast(
					MType,
					alignof(Type),
					sizeof(Type),
					Num
				);

				return { reinterpret_cast<Type*>(Adress), Count };
			}
		}
		Allocator() = default;
		Allocator(AllocatorInterface::Ptr InputPtr, AllocatorInterface::MemoryType InputType = AllocatorInterface::MemoryType::INSTANCE) : AllocatorPtr(std::move(InputPtr)), MType(InputType) {}
		Allocator(Allocator const&) = default;
		Allocator(Allocator&&) = default;
		template<typename OT>
		Allocator(Allocator<OT> Allocator)
			: Allocator(std::move(Allocator.AllocatorPtr), Allocator.MType) {}
		
	protected:
		
		const AllocatorInterface::MemoryType MType = AllocatorInterface::MemoryType::INSTANCE;
		AllocatorInterface::Ptr AllocatorPtr;

		template<typename T>
		friend struct Allocator;
	};
}