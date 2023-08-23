#include "asio.hpp"
#include "LitchiAsioWrapper.h"

namespace Litchi::AsioWrapper
{

	struct AsioContextImp : public AsioContext
	{
		AsioContextImp(MemoryResourceInfo Resource) : Resource(Resource) {}
		virtual void Release() override
		{
			auto OldResource = Resource;
			assert(OldResource.DeallocatorFunction != nullptr);
			this->~AsioContextImp();
			OldResource.DeallocatorFunction(OldResource.Object, this, sizeof(AsioContextImp), alignof(AsioContextImp));
		}

		virtual bool CallOnce() override
		{
			return Context.run_one() != 0;
		}

		MemoryResourceInfo Resource;
		asio::io_context Context;
	};

	AsioContext* AsioContext::CreateInstance(MemoryResourceInfo AllocatorInfo)
	{
		assert(AllocatorInfo.AllocatorFunction != nullptr);
		auto Adress = AllocatorInfo.AllocatorFunction(AllocatorInfo.Object, sizeof(AsioContextImp), alignof(AsioContextImp));
		if(Adress != nullptr)
		{
			auto Ptr = new (Adress) AsioContextImp{ std::move(AllocatorInfo) };
		}
		return nullptr;
	}

	struct AsioResolverImp : public TcpAsioResolver
	{
		AsioResolverImp(MemoryResourceInfo Info, AsioContextImp& Context)
			: Resource(Info), Resolver(Context.Context) {}
		static AsioContext* CreateInstance(MemoryResourceInfo AllocatorInfo);

		virtual void Release() override
		{
			auto OldResource = Resource;
			assert(OldResource.DeallocatorFunction != nullptr);
			this->~AsioResolverImp();
			OldResource.DeallocatorFunction(OldResource.Object, this, sizeof(AsioResolverImp), alignof(AsioResolverImp));
		}

		MemoryResourceInfo Resource;
		asio::ip::tcp::resolver Resolver;

	};
}

namespace Litchi
{
}
