#pragma once
#include "asio/io_context.hpp"
#include <thread>
#include <vector>
#include <optional>
namespace Litchi
{
	template<template<typename...> class AllocatorT = std::allocator>
	struct Context
	{
		Context(std::size_t InThreadCount = 1)
			: ThreadCount(std::max(InThreadCount, std::size_t{ 1 })), IOContext(static_cast<int>(InThreadCount))
		{
			Work.emplace(IOContext.get_executor());
			for (std::size_t I = 0; I < ThreadCount; ++I)
			{
				Threads.emplace_back(
					[this]() { IOContext.run(); }
				);
			}
		}

		~Context()
		{
			Work.reset();
			IOContext.stop();
			for (auto& Ite : Threads)
				Ite.join();
			Threads.clear();
		}

		asio::io_context& GetIOContext(){ return IOContext; }
		std::size_t GetThreadCount() const { return ThreadCount; }
	protected:
		const std::size_t ThreadCount;
		asio::io_context IOContext;
		std::optional<asio::executor_work_guard<decltype(IOContext.get_executor())>> Work;
		std::vector<std::thread, AllocatorT<std::thread>> Threads;
	};
}