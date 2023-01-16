#include "Litchi/LitchiCompression.h"
#include "zlib/zlib.h"
namespace Litchi
{
	namespace Implement
	{
		std::optional<std::size_t> GZipDecompressImp(
			std::span<std::byte const> Input,
			std::span<std::byte> TempOutput,
			void(*OutputFunction)(void*, std::span<std::byte const>),
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
			while (true)
			{
				Stream.next_out = reinterpret_cast<Bytef*>(TempOutput.data());
				Stream.avail_out = TempOutput.size() * sizeof(std::byte) / sizeof(Bytef);
				int Re = inflate(&Stream, Z_SYNC_FLUSH);
				switch (Re)
				{
				case Z_OK:
					OutputFunction(Data, TempOutput.subspan(0, TempOutput.size() - Stream.avail_out));
					break;
				case Z_STREAM_END:
				{
					OutputFunction(Data, TempOutput.subspan(0, TempOutput.size() - Stream.avail_out));
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