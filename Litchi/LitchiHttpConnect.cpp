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

	template<>
	struct Formatter<Litchi::HttpMethodT, char8_t>
	{
		static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpMethodT Input);
		static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Parameter, Litchi::HttpMethodT Input);
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

	static constexpr std::u8string_view OptionFor1 = u8"AcceptEncoding: {}\r\nAcceptCharset: {}\r\n";
	static constexpr std::u8string_view OptionFor2 = u8"Connection: keep-alive\r\n";
	static constexpr std::u8string_view OptionFor3 = u8"Accept: {}\r\n";


	std::optional<std::size_t> Formatter<Litchi::HttpOptionT, char8_t>::Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input)
	{
		auto Count = Format::FormatToUnSafe(Output, OptionFor1, Input.AcceptEncoding, Input.AcceptCharset);
		if (!Count.has_value())
			return {};
		Output = Output.subspan(*Count);
		if (Input.KeekAlive)
		{
			auto Count3 = Format::FormatToUnSafe(Output, OptionFor2);
			Output = Output.subspan(*Count3);
			*Count += *Count3;
		}
		if (!Input.Accept.empty())
		{
			auto Count3 = Format::FormatToUnSafe<char8_t>(Output, OptionFor3, Input.Accept);
			Output = Output.subspan(*Count3);
			*Count += *Count3;
		}
		return Count;
	}

	std::optional<std::size_t> Formatter<Litchi::HttpOptionT, char8_t>::FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input)
	{
		auto Count = Format::FormatSize(OptionFor1, Input.AcceptEncoding, Input.AcceptCharset);
		if (!Count.has_value())
			return {};
		if (Input.KeekAlive)
		{
			auto Count3 = Format::FormatSize(OptionFor2);
			*Count += *Count3;
		}
		if (!Input.Accept.empty())
		{
			auto Count3 = Format::FormatSize(OptionFor3, Input.Accept);
			*Count += *Count3;
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
		auto Re = *Potato::Format::FormatToUnSafe({ reinterpret_cast<char8_t*>(Output.data()), Output.size() }, HeadOnlyRequestFor, Method, Target, Host, Optional, ContextT);
		std::u8string_view Poi = { reinterpret_cast<char8_t*>(Output.data()), Output.size()};
		Potato::Document::Writer W1 (u8"Send.txt");
		W1.Write(Poi);
		W1.Flush();
		return Re;
	}

	bool Http11Agency::TryGenerateRespond()
	{
		constexpr std::u8string_view Spe = u8"\r\n\r\n";
		constexpr std::u8string_view SubSpe = u8"\r\n";

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
					auto Index = Str.find(Spe);
					if (Index < Str.size())
					{
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.begin() + Index);
						RespondHeadLength = Index;
						Str = Str.substr(0, Index);
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(Index + Spe.size());
						auto P = FindHeadOptionalValue(u8"Transfer-Encoding", Str);
						if (P.has_value() && *P == u8"chunked")
						{
							Status = StatusT::WaitChunkedContext;
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
						Str = Str.substr(Index);
						HexChunkedContextCount Count;
						Potato::Format::DirectScan(Str, Count);
						ContextLength = Count.Value;
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(Index + SubSpe.size());
						Status = StatusT::WaitChunkedContext;
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
						return false;
					}
				}
				break;
				case StatusT::WaitChunkedEnd:
				{
					if (TotalBuffer.size() >= SubSpe.size())
					{
						ReceiveBufferIndex = ReceiveBufferIndex.Sub(SubSpe.size());
						return true;
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


/*
namespace Potato::Format
{
	constexpr auto Trans(Litchi::HttpMethodT Method) -> std::u8string_view
	{
		switch (Method)
		{
		case Litchi::HttpMethodT::Get:
			return u8"GET";
		}
		return {};
	}

	template<>
	struct Formatter<Litchi::HttpMethodT, char8_t>
	{
		constexpr static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpMethodT Input) {
			auto Tar = Trans(Input);
			std::copy_n(Tar.data(), Tar.size() * sizeof(char8_t), Output.data());
			return Tar.size();
		}
		constexpr static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Parameter, Litchi::HttpMethodT Input) {
			auto Tar = Trans(Input);
			return Tar.size();
		}
	};

	template<>
	struct Formatter<Litchi::HttpTargetT, char8_t>
	{
		static constexpr std::u8string_view For1 = u8"{} {} {}\r\n";
		static constexpr std::u8string_view For2 = u8"Content-Type: {}\r\n";
		static constexpr std::u8string_view For3 = u8"Content-Length: {}\r\n";
		static constexpr std::u8string_view For4 = u8"Transfer-Encoding: chunked\r\n";

		constexpr static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpTargetT const& Input)
		{
			auto Count = Potato::Format::FormatToUnSafe(Output, For1, Input.Method, Input.Target, Pars);
			if(!Count.has_value())
				return {};
			Output = Output.subspan(*Count);
			if (!Input.ContextType.empty())
			{
				auto Count2 = Format::FormatToUnSafe(Output, For2, Input.ContextType);
				*Count += *Count2;
				Output = Output.subspan(*Count2);
				if (Input.ChunkedContext)
				{
					auto Count3 = Potato::Format::DirectFormatToUnSafe<char8_t>(Output, {}, For4);
					*Count += *Count3;
				}
			}
			return Count;
		}

		constexpr static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpTargetT const& Input)
		{
			auto Count = Format::FormatSize(For1, Input.Method, Input.Target, Pars);
			if (!Count.has_value())
				return {};
			if (!Input.ContextType.empty())
			{
				auto Count2 = Format::FormatSize(For2, Input.ContextType);
				*Count += *Count2;
				if (Input.ChunkedContext)
				{
					auto Count3 = Format::DirectFormatSize<char8_t>({}, For4);
					*Count += *Count3;
				}
			}
			return Count;
		}
	};

	template<>
	struct Formatter<Litchi::HttpOptionT, char8_t>
	{
		static constexpr std::u8string_view For1 = u8"AcceptEncoding: {}\r\nAcceptCharset: {}\r\n";
		static constexpr std::u8string_view For2 = u8"Connection: keep-alive\r\n";
		static constexpr std::u8string_view For3 = u8"Accept: {}\r\n";

		constexpr static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input)
		{
			auto Count = Format::FormatToUnSafe(Output, For1, Input.AcceptEncoding, Input.AcceptCharset);
			if (!Count.has_value())
				return {};
			Output = Output.subspan(*Count);
			if (Input.KeekAlive)
			{
				auto Count3 = Format::FormatToUnSafe(Output, For2);
				Output = Output.subspan(*Count3);
				*Count += *Count;
			}
			if (!Input.Accept.empty())
			{
				auto Count3 = Format::FormatToUnSafe<char8_t>(Output, For3, Input.Accept);
				Output = Output.subspan(*Count3);
				*Count += *Count;
			}
			return Count;
		}

		constexpr static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpOptionT const& Input)
		{
			auto Count = Format::FormatSize(For1, Input.AcceptEncoding, Input.AcceptCharset);
			if (!Count.has_value())
				return {};
			if (Input.KeekAlive)
			{
				auto Count3 = Format::FormatSize(For2);
				*Count += *Count;
			}
			if (!Input.Accept.empty())
			{
				auto Count3 = Format::FormatSize(For3, Input.Accept);
				*Count += *Count;
			}
			return Count;
		}
	};

	template<>
	struct Formatter<Litchi::HttpContextT char8_t>
	{
		static constexpr std::u8string_view For1 = u8"Cookie: {}\r\n";

		constexpr static std::optional<std::size_t> Format(std::span<char8_t> Output, std::basic_string_view<char8_t> Pars, Litchi::HttpContextT const& Input)
		{
			if (!Input.Cookie.empty())
			{
				return Format::FormatToUnSafe(Output, For1, Input.Cookie);
			}
			return 0;
		}

		constexpr static std::optional<std::size_t> FormatSize(std::basic_string_view<char8_t> Pars, Litchi::HttpContextT const& Input)
		{
			if (!Input.Cookie.empty())
			{
				return Format::FormatSize(For1, Input.Cookie);
			}
			return 0;
		}
	};
}

namespace Litchi
{

	std::size_t Http11Agency::FormatHttpRequestSize(HttpTargetT Target, HttpOptionT Option, HttpContextT Context)
	{
		if (Target.Context.empty())
		{
			return Potato::Format::FormatSize();
		}
		else {

		}
		
	}

	/*
	auto Http11Client::RequestLength(RequestMethodT Method, std::u8string_view Target, std::u8string_view Host, std::span<std::byte const> Content, std::u8string_view ContentType) -> std::size_t
	{
		if (Content.empty() || ContentType.empty())
		{
			return *Potato::StrFormat::FormatSize(NoContentFormatter, Method, (Target.empty() ? std::u8string_view{u8"/"} : Target), Host) * sizeof(char8_t);
		}
		return *Potato::StrFormat::FormatSize(HeadContentFormatter, Method, (Target.empty() ? std::u8string_view{ u8"/" } : Target), Host, ContentType, Content.size()) * sizeof(char8_t) + Content.size() ;
	}

	auto Http11Client::TranslateRequestTo(std::span<std::byte> OutputBuffer, RequestMethodT Method, std::u8string_view Target, std::u8string_view Host, std::span<std::byte const> Content, std::u8string_view ContentType) -> std::size_t
	{
		if (Content.empty() || ContentType.empty())
		{
			return *Potato::StrFormat::FormatToUnSafe({reinterpret_cast<char8_t*>(OutputBuffer.data()),  OutputBuffer.size() / sizeof(char8_t)}, NoContentFormatter, Method, (Target.empty() ? std::u8string_view{u8"/"} : Target), Host) * sizeof(char8_t);
		}
		else
		{
			auto Size = *Potato::StrFormat::FormatSize(HeadContentFormatter, Method, (Target.empty() ? std::u8string_view{ u8"/" } : Target), Host, ContentType, Content.size()) * sizeof(char8_t);
			std::copy_n(Content.begin(), Content.size(), OutputBuffer.subspan(Size).data());
			return Size + Content.size();
		}
	}
	auto Http11Client::FindHeadSize(std::u8string_view Str) -> std::optional<std::size_t>
	{
		auto Index = Str.find(HeadSperator());
		if (Index < Str.size())
		{
			return (Index + HeadSperator().size());
		}else
			return {};
	}
	auto Http11Client::FindHeadOptionalValue(std::u8string_view Key, std::u8string_view Head) -> std::optional<std::u8string_view>
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
					if(Index < Head.size())
						return Head.substr(0, Index);
				}
			}else
				return {};
		}
		return {};
	}

	auto Http11Client::FindHeadContentLength(std::u8string_view Head) -> std::optional<std::size_t>
	{
		auto Length = FindHeadOptionalValue(u8"Content-Length", Head);
		if (Length.has_value())
		{
			std::size_t Count = 0;
			Potato::StrFormat::DirectScan(*Length, Count);
			return Count;
		}
		return {};
	}

	auto Http11Client::FindSectionLength(std::u8string_view Str) -> std::optional<std::size_t>
	{
		auto Index = Str.find(SectionSperator());
		if(Index < Str.size())
			return Index + SectionSperator().size();
		return {};
	}
	auto Http11Client::IsChunkedContent(std::u8string_view Head) -> bool
	{
		auto Op = FindHeadOptionalValue(u8"Transfer-Encoding", Head);
		return Op.has_value() && *Op == u8"chunked";
	}
	auto Http11Client::IsResponseHead(std::u8string_view Head) -> bool
	{
		return Head.starts_with(u8"HTTP/1.1");
	}
	auto Http11Client::IsResponseHeadOK(std::u8string_view Head) -> bool
	{
		return Head.starts_with(u8"HTTP/1.1 200 OK\r\n");
	}

	auto Http11Client::SyncConnect(std::u8string_view Host) -> std::optional<std::tuple<std::error_code, EndPointT>>
	{
		std::promise<std::tuple<std::error_code, EndPointT>> Promise;
		auto Fur = Promise.get_future();
		{
			auto lg = std::lock_guard(SocketMutex);
			if(!Http11Client::AsyncConnect(Host, [&](std::error_code const& EC, EndPointT EP, Agency&){
				Promise.set_value({EC, std::move(EP)});
			}))
				return {};
		}
		return Fur.get();
	}

	static constexpr std::size_t CacheSize = 1024;

	std::span<std::byte> Http11Client::PrepareTBufferForReceive()
	{
		if (TBuffer.size() - TIndex.End() < CacheSize)
		{
			if (TIndex.Begin() != 0)
			{
				TBuffer.erase(TBuffer.begin(), TBuffer.begin() + TIndex.Begin());
				TBuffer.resize(TBuffer.capacity());
				TIndex.Offset = 0;
			}
			if (TBuffer.size() - TIndex.End() < CacheSize)
			{
				TBuffer.reserve(TBuffer.capacity() + CacheSize);
				TBuffer.resize(TBuffer.capacity());
			}
		}
		return Potato::Misc::IndexSpan<>{TIndex.End(), CacheSize}.Slice(TBuffer);
	}

	void Http11Client::ReceiveTBuffer(std::size_t Read)
	{
		TIndex.Length += Read;
	}

	void Http11Client::ClearTBuffer()
	{
		TBuffer.clear();
		TIndex = {};
	}

	auto Http11Client::ConsumeHead() -> std::optional<HeadR>
	{
		auto Span = TIndex.Slice(TBuffer);
		auto Head = std::u8string_view{reinterpret_cast<char8_t const*>(Span.data()), Span.size()};
		auto HeadSize = FindHeadSize(Head);
		if (HeadSize.has_value())
		{
			Head = Head.substr(0, *HeadSize);
			TIndex = TIndex.Sub(*HeadSize);
			HeadR Res;
			Res.HeadData = Span.subspan(0, *HeadSize);
			Res.IsOK = IsResponseHeadOK(Head);
			Res.HeadContentSize = FindHeadContentLength(Head);
			Res.NeedChunkedContent = IsChunkedContent(Head);
			return Res;
		}
		return {};
	}

	std::span<std::byte> Http11Client::ConsumeTBuffer(std::size_t MaxSize)
	{
		std::size_t MiniSize = std::min(MaxSize, TIndex.Count());
		auto Out = TIndex.Sub(0, MiniSize).Slice(TBuffer);
		TIndex = TIndex.Sub(MiniSize);
		return Out;
	}

	std::span<std::byte> Http11Client::ConsumeTBufferTo(std::span<std::byte> Input)
	{
		auto Out = ConsumeTBuffer(Input.size());
		std::copy_n(Out.begin(), Out.size(), Input.begin());
		return Input.subspan(Out.size());
	}

	std::optional<std::size_t> Http11Client::ConsumeChunkedContentSize(bool NeedContentEnd)
	{
		auto CurSpan = TIndex.Slice(TBuffer);
		if (NeedContentEnd)
		{
			if(CurSpan.size() < SectionSperator().size())
				return {};
			CurSpan = CurSpan.subspan(SectionSperator().size());
		}
		std::u8string_view Viewer {reinterpret_cast<char8_t const*>(CurSpan.data()), CurSpan.size()};
		auto SS = FindSectionLength(Viewer);
		if (SS.has_value())
		{
			Viewer = Viewer.substr(0, *SS - SectionSperator().size());
			CurSpan = CurSpan.subspan(*SS);
			HexChunkedContentCount Count;
			Potato::StrFormat::DirectScan(Viewer, Count);
			TIndex = TIndex.Sub(TIndex.Count() - CurSpan.size());
			return Count.Value;
		}
		return {};
	}

	bool Http11Client::TryConsumeContentEnd()
	{
		if (TIndex.Count() >= SectionSperator().size())
		{
			TIndex = TIndex.Sub(SectionSperator().size());
			return true;
		}
		return false;
	}

	std::optional<std::span<std::byte>> Http11Client::HandleRespond(RespondT& Res, SectionT Section, std::size_t SectionCount)
	{
		switch (Section)
		{
		case SectionT::Head:
			Res.Head.resize(SectionCount);
			return std::span<std::byte>{reinterpret_cast<std::byte*>(Res.Head.data()), Res.Head.size()};
		case SectionT::HeadContent:
		case SectionT::ChunkedContent:
			Res.Content.resize(Res.Content.size() + SectionCount);
			return std::span(Res.Content).subspan(Res.Content.size() - SectionCount);
		case SectionT::Finish:
			return std::nullopt;
		}
		return std::nullopt;
	}

	std::optional<std::vector<std::byte>> Http11Client::DecompressContent(std::u8string_view Head, std::span<std::byte const> Res)
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

	bool Http11ClientSequencer::ExecuteSendList()
	{
		if (!SendList.empty() && AbleToSend())
		{
			auto Top = std::move(*SendList.begin());
			SendList.pop_front();
			auto Span = std::span(Top.Buffer);
			bool Re = Http11Client::AsyncSend(Span, [Buffer = std::move(Top.Buffer), Func = std::move(Top.Func)](std::error_code const& EC, LastAgency& LAge) mutable {
				Agency Age{LAge};
				if (!EC)
				{
					Age->RespondList.push_back(std::move(Func));
				}
				else {
					Func(EC, {}, Age);
				}
				Age->ExecuteReceiveList();
			});
			assert(Re);
			return true;
		}
		return false;
	}

	bool Http11ClientSequencer::ExecuteReceiveList()
	{
		if (!RespondList.empty() && Http11Client::AbleToReceive())
		{
			auto Top = std::move(*RespondList.begin());
			RespondList.pop_front();
			bool Re = Http11Client::AsyncReceiveRespond([Top = std::move(Top)](std::error_code const& EC, RespondT Respond, LastAgency& LAge) mutable {
				Agency Age{ LAge };
				if (!EC)
				{
					Top(EC, std::move(Respond), Age);
				}
				else {
					Top(EC, {}, Age);
				}
				Age->ExecuteReceiveList();
			});
			assert(Re);
			return true;
		}
		return false;
	}

	auto Http11ClientSequencer::SyncConnect(std::u8string_view Host) -> std::optional<std::tuple<std::error_code, EndPointT>>
	{
		std::promise<std::tuple<std::error_code, EndPointT>> Promise;
		auto Fur = Promise.get_future();
		{
			auto lg = std::lock_guard(SocketMutex);
			if (!Http11ClientSequencer::AsyncConnect(Host, [&](std::error_code const& EC, EndPointT EP, Agency&) {
				Promise.set_value({ EC, std::move(EP) });
			}))
				return {};
		}
		return Fur.get();
	}
}
*/

namespace Potato::StrFormat
{
	/*
	bool Scanner<Litchi::Http11Client::HexChunkedContentCount, char8_t>::Scan(std::u8string_view Par, Litchi::Http11Client::HexChunkedContentCount& Input)
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
	*/
}