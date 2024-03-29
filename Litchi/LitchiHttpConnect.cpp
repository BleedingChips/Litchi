module;

#include "AsioWrapper/LitchiAsioWrapper.h"
#include <cassert>

module LitchiHttp;

namespace Potato::Format
{

	auto Trans(Litchi::Http::MethodT Method) -> std::u8string_view
	{
		switch (Method)
		{
		case Litchi::Http::MethodT::Get:
			return u8"GET";
		}
		return {};
	}

	auto Trans(Litchi::Http::ConnectionT Connect) -> std::u8string_view
	{
		switch (Connect)
		{
		case Litchi::Http::ConnectionT::None:
			return {};
		case Litchi::Http::ConnectionT::Close:
			return u8"Connection: close\r\n";
		}
		return {};
	}

	template<>
	struct Formatter<Litchi::Http::MethodT, char8_t>
	{
		bool operator()(FormatWritter<char8_t>& Writer, std::basic_string_view<char8_t> Parameter, Litchi::Http::MethodT Input)
		{
			auto Tar = Trans(Input);
			Writer.Write(std::span(Tar));
			return !Tar.empty();
		}
	};

	template<>
	struct Formatter<Litchi::Http::ConnectionT, char8_t>
	{
		bool operator()(FormatWritter<char8_t>& Writer, std::basic_string_view<char8_t> Parameter, Litchi::Http::ConnectionT const& Input)
		{
			auto Tar = Trans(Input);
			Writer.Write(Tar);
			return true;
		}
	};

	static constexpr std::u8string_view OptionF2 = u8"Accept-Encoding: {}\r\n";
	static constexpr std::u8string_view OptionF3 = u8"Accept-Charset: {}\r\n";
	static constexpr std::u8string_view OptionF4 = u8"Accept: {}\r\n";

	template<>
	struct Formatter<Litchi::Http::OptionT, char8_t>
	{
		bool operator()(FormatWritter<char8_t>& Writer, std::basic_string_view<char8_t> Pars, Litchi::Http::OptionT const& Input)
		{
			bool Re = true;
			Re = Re && Formatter<Litchi::Http::ConnectionT, char8_t>{}(Writer, {}, Input.Connection);
			if (!Input.AcceptEncoding.empty())
			{
				Re = Re && Formatter<std::u8string_view, char8_t>{}(Writer, OptionF2, Input.AcceptEncoding);
			}
			if (!Input.AcceptCharset.empty())
			{
				Re = Re && Formatter<std::u8string_view, char8_t>{}(Writer, OptionF3, Input.AcceptCharset);
			}
			if (!Input.Accept.empty())
			{
				Re = Re && Formatter<std::u8string_view, char8_t>{}(Writer, OptionF4, Input.Accept);
			}
			return Re;
		}
	};



	static constexpr std::u8string_view ContextFor1 = u8"Cookie: {}\r\n";

	template<>
	struct Formatter<Litchi::Http::ContextT, char8_t>
	{
		bool operator()(FormatWritter<char8_t>& Writer, std::basic_string_view<char8_t> Pars, Litchi::Http::ContextT const& Input)
		{
			return Formatter<std::u8string_view, char8_t>{}(Writer, ContextFor1, Input.Cookie);
		}
	};

	template<>
	struct Scanner<Litchi::Http::HexChunkedContextCount, char8_t>
	{
		bool Scan(std::u8string_view Par, Litchi::Http::HexChunkedContextCount& Input)
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

namespace Litchi::Http::ErrorCode
{
	struct HttpErrorCodeCategoryT : public std::error_category
	{
		virtual const char* name() const noexcept {return "Http Error";}

		virtual std::string message(int _Errval) const
		{
			HttpErrorCode Code = static_cast<HttpErrorCode>(_Errval);
			switch(Code)
			{
			case HttpErrorCode::SendRequestInSequence:
				return "SendRequestInSequence";
				break;
			}
			return "Unknow";
		}
	};

	std::error_category const& HttpErrorCodeCategory()
	{
		static HttpErrorCodeCategoryT Instance;
		return Instance;
	}

	std::error_code const& SendInSequence()
	{
		static std::error_code EC{ *ErrorCode::HttpErrorCode::SendRequestInSequence, ErrorCode::HttpErrorCodeCategory() };
		return EC;
	}

	std::error_code const& RequestInSequence()
	{
		static std::error_code EC{ *ErrorCode::HttpErrorCode::ReceiveRequestInSequence, ErrorCode::HttpErrorCodeCategory() };
		return EC;
	}
}

namespace Litchi::Http
{
	auto Http11::Create(Context::Ptr Owner, std::pmr::memory_resource* IMemoryResource)
		-> Ptr
	{
		if (Owner && IMemoryResource != nullptr)
		{
			auto Layout = AsioWrapper::TCPSocket::GetLayout();
			assert(Layout.Align == alignof(Http11));
			auto MPtr = IMemoryResource->allocate(
				Layout.Size + sizeof(Http11),
				alignof(Http11)
			);
			if (MPtr != nullptr)
			{
				auto P = reinterpret_cast<std::byte*>(MPtr) + sizeof(Http11);
				auto ResoAdress = new Http11{ std::move(Owner), P, IMemoryResource };
				return Ptr{ ResoAdress };
			}
		}
		return {};
	};

	void Http11::Release()
	{
		auto OResource = IMResource;
		this->~Http11();
		auto Layout = AsioWrapper::TCPSocket::GetLayout();
		OResource->deallocate(this, Layout.Size + sizeof(Http11), alignof(Http11));
	}

	static constexpr std::u8string_view HeadOnlyRequestFor = u8"{} {} HTTP/1.1\r\nHost: {}\r\n{}{}Content-Length: 0\r\n\r\n";

	std::size_t Http11::FormatSizeHeadOnlyRequest(MethodT Method, std::u8string_view Target, OptionT const& Optional, ContextT const& ContextT)
	{
		Potato::Format::FormatWritter<char8_t> Writer;
		Potato::Format::Format(Writer, HeadOnlyRequestFor, Method, Target, Host, Optional, ContextT);
		return Writer.GetWritedSize();
	}

	std::size_t Http11::FormatToHeadOnlyRequest(std::span<std::byte> Output, MethodT Method, std::u8string_view Target, OptionT const& Optional, ContextT const& ContextT)
	{
		Potato::Format::FormatWritter<char8_t> Writer{
			{reinterpret_cast<char8_t*>(Output.data()), Output.size()}
		};
		Potato::Format::Format(Writer, HeadOnlyRequestFor, Method, Target, Host, Optional, ContextT);
		return Writer.GetWritedSize();
	}

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

	/*
	void Http11::FixInputBuffer(ReadRespond Readed)
	{
		if(ReceiveBuffer.size() > AvailableReceiveBuffer.End())
		{
			AvailableReceiveBuffer.BackwardEnd(Readed.LastReadSize);
			ReceiveBuffer.resize(
				AvailableReceiveBuffer.End()
			);
		}
	}

	
	*/

	std::span<std::byte> Http11::FixOutputBuffer()
	{
		if (AvailableReceiveBuffer.Begin() * 2 >= ReceiveBuffer.size())
		{
			ReceiveBuffer.erase(
				ReceiveBuffer.begin(),
				ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin()
			);
			AvailableReceiveBuffer = {
			0,
				AvailableReceiveBuffer.Size()
			};
		}
		auto Size = ReceiveBuffer.size() % 1024;
		if (Size == 0)
		{
			ReceiveBuffer.resize(
				ReceiveBuffer.size() + 1024
			);
			return std::span(ReceiveBuffer).subspan(AvailableReceiveBuffer.End());
		}
		else
		{
			ReceiveBuffer.resize(
				2048 - Size
			);
			return std::span(ReceiveBuffer).subspan(AvailableReceiveBuffer.End());
		}
	}

	auto Http11::Handle(std::error_code const& EC, ReadRespond const& LastRespond, Respond& OutputRespond) -> std::optional<ReadRequire>
	{
		constexpr std::u8string_view Spe = u8"\r\n\r\n";
		constexpr std::u8string_view SubSpe = u8"\r\n";

		if(LastRespondContextSize.has_value())
		{
			OutputRespond.Context.resize(*LastRespondContextSize + LastRespond.LastReadSize);
			LastRespondContextSize.reset();
		}else
		{
			ReceiveBuffer.resize(AvailableReceiveBuffer.End() + LastRespond.LastReadSize);
			AvailableReceiveBuffer.BackwardEnd(LastRespond.LastReadSize);
		}

		while(true)
		{
			switch(Type)
			{
			case ReceiveType::Head:
				{
					std::u8string_view TotalStr{ reinterpret_cast<char8_t*>(ReceiveBuffer.data()), ReceiveBuffer.size()};;
					std::u8string_view HeadStr = AvailableReceiveBuffer.Slice(TotalStr);
					auto K = HeadStr.find(Spe);
					if (K == std::u8string_view::npos)
					{
						if(EC)
						{
							OutputRespond.Head = HeadStr;
							ReceiveBuffer.clear();
							AvailableReceiveBuffer = {};
							Type = ReceiveType::Finish;
							return std::nullopt;
						}else
							return ReadRequire{false, FixOutputBuffer()};
					}else
					{
						auto Str = HeadStr.substr(0, K);
						AvailableReceiveBuffer = AvailableReceiveBuffer.SubIndex(K);
						OutputRespond.Head = Str;
						auto P = FindHeadOptionalValue(u8"Transfer-Encoding", Str);
						if (P.has_value() && *P == u8"chunked")
						{
							Type = ReceiveType::ChunkedContent;
							ContextRequire.reset();
						}
						else
						{
							P = FindHeadOptionalValue(u8"Content-Length", Str);
							if (P.has_value())
							{
								std::size_t ContextLength = 0;
								Potato::Format::DirectScan(*P, ContextLength);
								Type = ReceiveType::Content;
								ContextRequire = ContextLength;
							}
							else
							{
								Type = ReceiveType::Finish;
								return std::nullopt;
							}
						}
					}
					break;
				}
			case ReceiveType::Content:
				{
					OutputRespond.Context.resize(*ContextRequire);
					if (AvailableReceiveBuffer.Size() >= *ContextRequire)
					{
						std::copy(
							ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin(),
							ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin() + *ContextRequire,
							OutputRespond.Context.begin()
						);
						AvailableReceiveBuffer = AvailableReceiveBuffer.SubIndex(*ContextRequire);
						Type = ReceiveType::Finish;
						return std::nullopt;
					}else
					{
						std::copy(
							ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin(),
							ReceiveBuffer.begin() + AvailableReceiveBuffer.End(),
							OutputRespond.Context.begin()
						);
						auto OutputSpan = std::span(OutputRespond.Context).subspan(AvailableReceiveBuffer.Size());
						LastRespondContextSize = AvailableReceiveBuffer.Size();
						AvailableReceiveBuffer = {};
						Type = ReceiveType::Finish;
						
						return ReadRequire{true, OutputSpan };
					}
					break;
				}
			case ReceiveType::ChunkedContent:
				{
					if(!ContextRequire.has_value())
					{
						std::u8string_view TotalStr{ reinterpret_cast<char8_t*>(ReceiveBuffer.data()), ReceiveBuffer.size() };;
						std::u8string_view HeadStr = AvailableReceiveBuffer.Slice(TotalStr);
						auto Index = HeadStr.find(SubSpe);
						if(Index == std::u8string_view::npos)
						{
							if (EC)
							{
								ReceiveBuffer.clear();
								AvailableReceiveBuffer = {};
								Type = ReceiveType::Finish;
								return std::nullopt;
							}
							else
								return ReadRequire{ false, FixOutputBuffer() };
						}else
						{
							HeadStr = HeadStr.substr(0, Index);
							HexChunkedContextCount Count;
							Potato::Format::DirectScan(HeadStr, Count);
							ContextRequire = Count.Value;
							AvailableReceiveBuffer = AvailableReceiveBuffer.SubIndex(Index + SubSpe.size());
							if (*ContextRequire == 0)
							{
								Type = ReceiveType::Finish;
								return std::nullopt;
							}
							break;
						}
					}else
					{
						auto OldContextSize = OutputRespond.Context.size();
						OutputRespond.Context.resize(OldContextSize + *ContextRequire);
						if (AvailableReceiveBuffer.Size() >= *ContextRequire)
						{
							std::copy(
								ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin(),
								ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin() + *ContextRequire,
								OutputRespond.Context.begin() + OldContextSize
							);
							AvailableReceiveBuffer = AvailableReceiveBuffer.SubIndex(*ContextRequire);
							Type = ReceiveType::Finish;
							ContextRequire.reset();
							return std::nullopt;
						}
						else
						{
							std::copy(
								ReceiveBuffer.begin() + AvailableReceiveBuffer.Begin(),
								ReceiveBuffer.begin() + AvailableReceiveBuffer.End(),
								OutputRespond.Context.begin() + OldContextSize
							);
							auto OutputSpan = std::span(OutputRespond.Context).subspan(OldContextSize + AvailableReceiveBuffer.Size());
							LastRespondContextSize = AvailableReceiveBuffer.Size();
							AvailableReceiveBuffer = {};
							Type = ReceiveType::Finish;
							ContextRequire.reset();
							return ReadRequire{ true, OutputSpan };
						}
						break;
					}
					break;
				}
			default:
				return std::nullopt;
			}
		}
	}

}

/*
namespace Litchi
{
	struct HexChunkedContextCount
	{
		std::size_t Value;
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

	

	bool Http11Agency::TryGenerateRespond()
	{
		constexpr std::u8string_view Spe = u8"\r\n\r\n";
		constexpr std::u8string_view SubSpe = u8"\r\n";
		constexpr std::u8string_view RespondHead = u8"HTTP/1.1 ";

		while (true)
		{
			auto TotalBuffer = ReceiveBufferIndex.Slice(std::span(ReceiveBuffer));

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
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(Index);
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
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(ContextLength);
						return true;
					}
					else {
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.end());
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(ReceiveBufferIndex.Size());
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
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(Index + SubSpe.size());
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
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(ContextLength);
						Status = StatusT::WaitChunkedEnd;
					}
					else {
						CurrentRespond.insert(CurrentRespond.end(), TotalBuffer.begin(), TotalBuffer.end());
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(ReceiveBufferIndex.Size());
						ContextLength -= TotalBuffer.size();
						return false;
					}
				}
				break;
				case StatusT::WaitChunkedEnd:
				{
					if (TotalBuffer.size() >= SubSpe.size())
					{
						ReceiveBufferIndex = ReceiveBufferIndex.SubIndex(SubSpe.size());
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
*/