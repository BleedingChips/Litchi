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
		virtual void Deallocate(MemoryType Type, std::byte* MemoryBegin, std::size_t Align, std::size_t Size, std::size_t Count) = 0;
		virtual std::tuple<std::byte*, std::size_t> AllocateAtLast(MemoryType Type, std::size_t Align, std::size_t Size, std::size_t Count) = 0;
	};

	template<AllocatorInterface::MemoryType MType, typename Type>
	struct BasicAllocator : std::allocator<Type>
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

		constexpr void deallocate(void* Adress, std::size_t Num) {
			if (!AllocatorPtr)
			{
				return std::allocator<Type>::deallocate(Adress, Num);
			}
			else
				return reinterpret_cast<Type*>(AllocatorPtr->Deallocate(
					MType,
					Adress,
					alignof(Type),
					sizeof(Type),
					Num
				));
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
		BasicAllocator() = default;
		BasicAllocator(AllocatorInterface::Ptr InputPtr) : AllocatorPtr(std::move(InputPtr)) {}
		BasicAllocator(BasicAllocator const&) = default;
		BasicAllocator(BasicAllocator&&) = default;
		template<AllocatorInterface::MemoryType MT2, typename OT>
		BasicAllocator(BasicAllocator<MT2, OT> const Allocator) requires(MType == MT2 && MT2 == AllocatorInterface::MemoryType::INSTANCE)
			: BasicAllocator(Allocator.AllocatorPtr) {}

	protected:

		AllocatorInterface::Ptr AllocatorPtr;

		template<AllocatorInterface::MemoryType MT2, typename T>
		friend struct BasicAllocator;
	};

	template<typename Type>
	using InstanceAllocator = BasicAllocator<AllocatorInterface::MemoryType::INSTANCE, Type>;

	template<typename Type>
	using TempAllocator = BasicAllocator<AllocatorInterface::MemoryType::TEMP, Type>;
}