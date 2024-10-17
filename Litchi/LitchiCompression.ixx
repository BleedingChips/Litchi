module;
export module LitchiCompression;

import std;

export namespace Litchi
{
	struct Zip
	{
		struct DecompressProperty
		{
			std::size_t UnDecompressSize = 0;
			std::size_t LastReceiveOutputSize = 0;
			std::size_t LastDecompressOutputSize = 0;
		};

		template<typename RespondFun>
		std::optional<std::size_t> Decompress(std::span<std::byte const> Input, RespondFun Func)
			requires(std::is_invocable_r_v<std::span<std::byte>, RespondFun, void*, DecompressProperty>)
		{
			return Decompress(Input, [](void* Data, DecompressProperty const& Pro) -> std::span<std::byte> {
				return (*reinterpret_cast<RespondFun*>(Data))(Pro);
			}, &Func);
		}

		static std::optional<std::size_t> Decompress(
			std::span<std::byte const> Input,
			std::span<std::byte>(*OutputFunction)(void*, DecompressProperty const&),
			void* Data
		);

	};
}