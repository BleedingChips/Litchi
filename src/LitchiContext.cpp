#include "Litchi/LitchiContext.h"

namespace Litchi
{
	Context::Context(std::size_t InThreadCount)
		: ThreadCount(std::max(InThreadCount, std::size_t{1})), IOContext(static_cast<int>(InThreadCount))
	{
		Work.emplace(IOContext.get_executor());
		for (std::size_t I = 0; I < ThreadCount; ++I)
		{
			Threads.emplace_back(
				[this](){ IOContext.run(); }
			);
		}
	}

	Context::~Context()
	{
		Work.reset();
		IOContext.stop();
		for (auto& Ite : Threads)
			Ite.join();
		Threads.clear();
	}


}