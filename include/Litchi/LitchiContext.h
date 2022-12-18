#pragma once
#include "asio/io_context.hpp"
#include <thread>
#include <vector>
#include <optional>
namespace Litchi
{
	struct Context
	{
		Context(std::size_t InThreadCount = 1);
		~Context();
		const std::size_t ThreadCount;
		asio::io_context IOContext;
		std::optional<asio::executor_work_guard<decltype(IOContext.get_executor())>> Work;
		std::vector<std::thread> Threads;
	};
}