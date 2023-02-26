module;


#include "zlib.h"

export module Litchi.Compression;

export import Potato.STD;

export namespace Litchi
{
	struct GZipDecProperty
	{
		std::size_t UnDecompressSize = 0;
		std::size_t LastReceiveOutputSize = 0;
		std::size_t LastDecompressOutputSize = 0;
	};

	namespace Implement
	{
		std::optional<std::size_t> GZipDecompressImp(
			std::span<std::byte const> Input,
			std::span<std::byte>(*OutputFunction)(void*, GZipDecProperty const&),
			void* Data
		);
	}

	template<typename RespondFun>
	std::optional<std::size_t> GZipDecompress(std::span<std::byte const> Input, RespondFun Func)
		requires(std::is_invocable_r_v<std::span<std::byte>, RespondFun, GZipDecProperty const&>)
	{
		return Implement::GZipDecompressImp(Input, [](void* Data, GZipDecProperty const& Pro) -> std::span<std::byte> {
			return (*reinterpret_cast<RespondFun*>(Data))(Pro);
			}, &Func);
	}
}

module : private;

namespace Litchi
{
	namespace Implement
	{
		std::optional<std::size_t> GZipDecompressImp(
			std::span<std::byte const> Input,
			std::span<std::byte>(*OutputFunction)(void*, GZipDecProperty const&),
			void* Data
		)
		{
			z_stream Stream;
			Stream.zalloc = Z_NULL;
			Stream.zfree = Z_NULL;
			Stream.opaque = Z_NULL;
			Stream.next_in = reinterpret_cast<Bytef const*>(Input.data());
			Stream.avail_in = Input.size() * sizeof(std::byte) / sizeof(Bytef);
			if(inflateInit2(&Stream, MAX_WBITS + 16) != Z_OK)
				return {};
			std::size_t LastDecompressSize = 0;
			std::size_t LastOutputSize = 0;
			while (true)
			{
				GZipDecProperty Pro{ Stream.avail_in, LastOutputSize, LastDecompressSize };
				auto Out = OutputFunction(Data, Pro);
				Stream.next_out = reinterpret_cast<Bytef*>(Out.data());
				Stream.avail_out = Out.size() * sizeof(std::byte) / sizeof(Bytef);
				LastOutputSize = Out.size();
				int Re = inflate(&Stream, Z_SYNC_FLUSH);
				switch (Re)
				{
				case Z_OK:
					LastDecompressSize = Out.size() - Stream.avail_out;
					break;
				case Z_STREAM_END:
				{
					LastDecompressSize = Out.size() - Stream.avail_out;
					GZipDecProperty Pro{ Stream.avail_in, LastOutputSize, LastDecompressSize };
					OutputFunction(Data, Pro);
					std::size_t TotalSize = Stream.total_out;
					inflateEnd(&Stream);
					return TotalSize;
				}	
				default:
					inflateEnd(&Stream);
					return {};
				}
			}
			inflateEnd(&Stream);
			return {};
		}
	}
}