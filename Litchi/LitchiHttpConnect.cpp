module;

module Litchi.Http;
import Potato.Format;
import Litchi.Compression;
import Potato.Document;

namespace Litchi
{
	struct HexChunkedContextCount
	{
		std::size_t Value;
	};
}

namespace Potato::Format
{

	auto Trans(Litchi::HttpMethodT Method) -> std::u8string_view
	{
		switch (Method)
		{
		case Litchi::HttpMethodT::Get:
			return u8"GET";
		}
		return {};
	}

	auto Trans(Litchi::HttpConnectionT Connect) -> std::u8string_view
	{
		switch (Connect)
		{
		case Litchi::HttpConnectionT::None:
			return {};
		case Litchi::HttpConnectionT::Close:
			return u8"Connection: close\r\n";
		}
		return {};
	}

	template<>
	struct Formatter<Litchi::HttpMethodT, char8_t>
	{
		static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpMethodT Input);
		static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Parameter, Litchi::HttpMethodT Input);
	};

	template<>
	struct Formatter<Litchi::HttpConnectionT, char8_t>
	{
		static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpConnectionT Input) {
			auto Tar = Trans(Input);
			std::copy_n(Tar.data(), Tar.size() * sizeof(char8_t), Output.data());
			return Tar.size();
		}
		static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Parameter, Litchi::HttpConnectionT Input) {
			auto Tar = Trans(Input);
			return Tar.size();
		}
	};

	template<>
	struct Formatter<Litchi::HttpOptionT, char8_t>
	{
		static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input);
		static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input);
	};

	template<>
	struct Formatter<Litchi::HttpContextT, char8_t>
	{
		static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpContextT const& Input);
		static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpContextT const& Input);
	};

	std::optional<std::size_t> Formatter<Litchi::HttpMethodT, char8_t>::Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpMethodT Input) {
		auto Tar = Trans(Input);
		std::copy_n(Tar.data(), Tar.size() * sizeof(char8_t), Output.data());
		return Tar.size();
	}
	std::optional<std::size_t> Formatter<Litchi::HttpMethodT, char8_t>::FormatSize(std::basic_string_view<char8_t> Parameter, Litchi::HttpMethodT Input) {
		auto Tar = Trans(Input);
		return Tar.size();
	}

	static constexpr std::u8string_view OptionF2 = u8"Accept-Encoding: {}\r\n";
	static constexpr std::u8string_view OptionF3 = u8"Accept-Charset: {}\r\n";
	static constexpr std::u8string_view OptionF4 = u8"Accept: {}\r\n";


	std::optional<std::size_t> Formatter<Litchi::HttpOptionT, char8_t>::Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input)
	{
		auto Count = *Format::DirectFormatToUnSafe(Output, {}, Input.Connection);
		Output = Output.subspan(Count);
		if (!Input.AcceptEncoding.empty())
		{
			auto CCount = *Format::FormatToUnSafe(Output, OptionF2, Input.AcceptEncoding);
			Output = Output.subspan(CCount);
			Count += CCount;
		}
		if (!Input.AcceptCharset.empty())
		{
			auto CCount = *Format::FormatToUnSafe(Output, OptionF3, Input.AcceptCharset);
			Output = Output.subspan(CCount);
			Count += CCount;
		}
		if (!Input.Accept.empty())
		{
			auto CCount = *Format::FormatToUnSafe(Output, OptionF4, Input.Accept);
			Output = Output.subspan(CCount);
			Count += CCount;
		}
		return Count;
	}

	std::optional<std::size_t> Formatter<Litchi::HttpOptionT, char8_t>::FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input)
	{
		auto Count = *Format::DirectFormatSize<char8_t>({}, Input.Connection);
		if (!Input.AcceptEncoding.empty())
		{
			auto CCount = *Format::FormatSize(OptionF2, Input.AcceptEncoding);
			Count += CCount;
		}
		if (!Input.AcceptCharset.empty())
		{
			auto CCount = *Format::FormatSize(OptionF3, Input.AcceptCharset);
			Count += CCount;
		}
		if (!Input.Accept.empty())
		{
			auto CCount = *Format::FormatSize(OptionF4, Input.Accept);
			Count += CCount;
		}
		return Count;
	}

	static constexpr std::u8string_view ContextFor1 = u8"Cookie: {}\r\n";

	std::optional<std::size_t> Formatter<Litchi::HttpContextT, char8_t>::Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpContextT const& Input)
	{
		if (!Input.Cookie.empty())
		{
			return Format::FormatToUnSafe(Output, ContextFor1, Input.Cookie);
		}
		return 0;
	}

	std::optional<std::size_t> Formatter<Litchi::HttpContextT, char8_t>::FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpContextT const& Input)
	{
		if (!Input.Cookie.empty())
		{
			return Format::FormatSize(ContextFor1, Input.Cookie);
		}
		return 0;
	}


	template<>
	struct Scanner<Litchi::HexChunkedContextCount, char8_t>
	{
		bool Scan(std::u8string_view Par, Litchi::HexChunkedContextCount& Input)
		{
			Input.Value = 0;
			for (auto Ite : Par)
			{
				Input.Value *= 16;
				if (Ite >= u8'0' && Ite <= u8'9')
				{
					Input.Value += (Ite - u8'0');
				}
				else if (Ite >= u8'a' && Ite <= u8'f')
				{
					Input.Value += 10 + (Ite - u8'a');
				}
				else if (Ite >= u8'A' && Ite <= u8'F')
				{
					Input.Value += 10 + (Ite - u8'A');
				};
			}
			return true;
		}
	};
}

namespace Litchi
{

	auto FindHeadOptionalValue(std::u8string_view Key, std::u8string_view Head) -> std::optional<std::u8string_view>
	{
		constexpr std::u8string_view S1 = u8": ";
		constexpr std::u8string_view S2 = u8"\r\n";
		while (!Head.empty())
		{
			auto Index = Head.find(Key);
			if (Index < Head.size())
			{
				Head = Head.substr(Index + Key.size());
				Index = Head.find(S1);
				if (Index < Head.size())
				{
					Head = Head.substr(Index + S1.size());
					Index = Head.find(S2);
					if (Index < Head.size())
						return Head.substr(0, Index);
				}
			}
			else
				return {};
		}
		return {};
	}

	static constexpr std::u8string_view HeadOnlyRequestFor = u8"{} {} HTTP/1.1\r\nHost: {}\r\n{}{}Content-Length: 0\r\n\r\n";

	std::size_t Http11Agency::FormatSizeHeadOnlyRequest(HttpMethodT Method, std::u8string_view Target, HttpOptionT const& Optional, HttpContextT const& ContextT)
	{
		return *Potato::Format::FormatSize(HeadOnlyRequestFor, Method, Target, Host, Optional, ContextT);
	}

	std::size_t Http11Agency::FormatToHeadOnlyRequest(std::span<std::byte> Output, HttpMethodT Method, std::u8string_view Target, HttpOptionT const& Optional, HttpContextT const& ContextT)
	{
		return *Potato::Format::FormatToUnSafe({ reinterpret_cast<char8_t*>(Output.data()), Output.size() }, HeadOnlyRequestFor, Method, Target, Host, Optional, ContextT);
	}

	bool Http11Agency::TryGenerateRespond()
	{
		constexpr std::u8string_view Spe = u8"\r\n\r\n";
		constexpr std::u8string_view SubSpe = u8"\r\n";
		constexpr std::u8string_view RespondHead = u8"HTTP/1.1 ";

		while (true)
		{
			auto TotalBuffer = ReceiveBufferIndex.Slice(ReceiveBuffer);

			if (!TotalBuffer.empty())
			{
				switch (Status)
				{
				case StatusT::WaitHead:
				{
					auto Str = std::u8string_view{ reinterpret_cast<char8_t const*>(TotalBuffer.data()), TotalBuffer.size() };

					{
						auto Index = Str.find(RespondHead);
						if (Index < Str.size())
							Str = Str.substr(Index);
						else
							return false;
					}

					auto Index = Str.find(Spe);
					if (Index < Str.size())
					{
						Index += Spe.size();
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.begin() + Index);
						RespondHeadLength = Index;
						Str = Str.substr(0, Index);
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(Index);
						auto P = FindHeadOptionalValue(u8"Transfer-Encoding", Str);
						if (P.has_value() && *P == u8"chunked")
						{
							Status = StatusT::WaitChunkedContextHead;
						}
						else {
							auto P = FindHeadOptionalValue(u8"Content-Length", Str);
							if (P.has_value())
							{
								Potato::Format::DirectScan(*P, ContextLength);
								Status = StatusT::WaitHeadContext;
							}
							else {
								return true;
							}
						}
					}
					else {
						return false;
					}
				}
				break;
				case StatusT::WaitHeadContext:
				{
					if (TotalBuffer.size() >= ContextLength)
					{
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.begin() + ContextLength);
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(ContextLength);
						return true;
					}
					else {
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.end());
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(ReceiveBufferIndex.Count());
						ContextLength -= TotalBuffer.size();
						return false;
					}
				}
				break;
				case StatusT::WaitChunkedContextHead:
				{
					auto Str = std::u8string_view{ reinterpret_cast<char8_t const*>(TotalBuffer.data()), TotalBuffer.size() };
					auto Index = Str.find(SubSpe);
					if (Index < Str.size())
					{
						Str = Str.substr(0, Index);
						HexChunkedContextCount Count;
						Potato::Format::DirectScan(Str, Count);
						ContextLength = Count.Value;
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(Index + SubSpe.size());
						if(ContextLength != 0)
							Status = StatusT::WaitChunkedContext;
						else {
							ReachEnd = true;
							Status = StatusT::WaitChunkedEnd;
						}
					}
					else {
						return false;
					}
				}
				break;
				case StatusT::WaitChunkedContext:
				{
					if (TotalBuffer.size() >= ContextLength)
					{
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.begin() + ContextLength);
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(ContextLength);
						Status = StatusT::WaitChunkedEnd;
					}
					else {
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.end());
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(ReceiveBufferIndex.Count());
						ContextLength -= TotalBuffer.size();
						return false;
					}
				}
				break;
				case StatusT::WaitChunkedEnd:
				{
					if (TotalBuffer.size() >= SubSpe.size())
					{
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(SubSpe.size());
						if (!ReachEnd)
						{
							Status = StatusT::WaitChunkedContextHead;
						}
						else {
							ReachEnd = false;
							return true;
						}
					}
					else {
						return false;
					}
				}
				break;
				}
			}
			else {
				return false;
			}
		}
	}

	std::optional<std::vector<std::byte>> Http11Agency::DecompressContent(std::u8string_view Head, std::span<std::byte const> Res)
	{
		auto Value = FindHeadOptionalValue(u8"Content-Encoding", Head);
		if (Value.has_value())
		{
			if (*Value == u8"gzip")
			{
				std::vector<std::byte> Temp;
				auto Output = GZipDecompress(Res, [&](GZipDecProperty const& Pro) -> std::span<std::byte> {
					Temp.resize(Temp.size() - Pro.LastReceiveOutputSize + Pro.LastDecompressOutputSize);
					auto Comm = std::max(Pro.UnDecompressSize * 3, std::size_t{ 256 });
					Temp.resize(Temp.size() + Comm);
					return std::span(Temp).subspan(Temp.size() - Comm);
					});
				if (Output.has_value())
				{
					Temp.resize(*Output);
					return Temp;
				}
			}
		}
		return {};
	}
}