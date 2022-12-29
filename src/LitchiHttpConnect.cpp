#include "Litchi/LitchiHttpConnect.h"
#include "Potato/PotatoStrFormat.h"

namespace Potato::StrFormat
{

	constexpr auto Trans(Litchi::Http11::RequestMethodT Method) -> std::u8string_view
	{
		switch (Method)
		{
		case Litchi::Http11::RequestMethodT::GET:
			return u8"GET";
		}
		return {};
	}

	template<>
	struct Formatter<Litchi::Http11::RequestMethodT, char8_t>
	{
		bool Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::Http11::RequestMethodT Input) {
			auto Tar = Trans(Input);
			std::memcpy(Output.data(), Tar.data(), Tar.size() * sizeof(char8_t));
			return true;
		}
		std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Parameter, Litchi::Http11::RequestMethodT Input) {
			auto Tar = Trans(Input);
			return Tar.size();
		}
	};
}

namespace Litchi
{
	auto Http11::ProtocolLength(std::u8string_view Input) -> std::optional<std::size_t>
	{
		constexpr std::u8string_view Sperator = u8"\r\n\r\n";
		auto Index = Input.find(Sperator);
		if (Index < Input.size())
		{
			Input = Input.substr(0, Index);
			constexpr std::u8string_view LengthPar = u8"Content-Length: ";
			auto Index2 = Input.find(LengthPar);
			if (Index2 < Input.size())
			{
				Input = Input.substr(Index2 + LengthPar.size());
				Index2 = Input.find(u8"\r\n");
				Input = Input.substr(0, Index2);
				std::size_t Count = 0;
				Potato::StrFormat::DirectScan(Input, Count);
				return Count + (Index + Sperator.size()) * sizeof(char8_t);
			}
		}
		return {};
	}

	auto Http11::TranslateRequest(RequestT const& Re, std::u8string_view Host) -> std::u8string
	{
		static Potato::StrFormat::FormatPattern<char8_t> Pattern = *Potato::StrFormat::CreateFormatPattern(u8"{} /{} HTTP/1.1\r\nHost: {}\r\nContent-Length: {}\r\n\r\n{}");
		auto Reference = Potato::StrFormat::CreateFormatReference(
			Pattern, Re.Method, Re.Target, Host, Re.Context.size(), Re.Context
		);
		auto Size = Reference->GetRequireSize();
		std::u8string Buffer;
		Buffer.resize(Size);
		Reference->FormatTo(Buffer);
		return Buffer;
	}
}