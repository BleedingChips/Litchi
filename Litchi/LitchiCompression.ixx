module;

export module LitchiCompression;

import std;

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