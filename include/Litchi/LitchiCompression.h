#pragma once
#include <span>
#include <optional>
namespace Litchi
{
	namespace Implement
	{
		std::optional<std::size_t> GZipDecompressImp(
			std::span<std::byte const> Input,
			std::span<std::byte> TempOutput,
			void(*OutputFunction)(void*, std::span<std::byte const>),
			void* Data
		);
	}

	template<std::size_t BufferCache = 2048, typename RespondFun>
	std::optional<std::size_t> GZipDecompress(std::span<std::byte const> Input, RespondFun Func)
		requires(std::is_invocable_v<RespondFun, std::span<std::byte const>>)
	{
		std::array<std::byte, BufferCache> TemporaryBuffer;
		return Implement::GZipDecompressImp(Input, std::span(TemporaryBuffer), [](void* Data, std::span<std::byte const> InputTemp){
			(*reinterpret_cast<RespondFun*>(Data))(InputTemp);
		}, &Func);
	}
}