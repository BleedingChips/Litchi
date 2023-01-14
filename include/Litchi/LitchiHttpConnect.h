#pragma once
#include "LitchiSocketConnect.h"
#include <map>
#include <optional>
#include <string_view>
#include "Potato/PotatoStrFormat.h"

namespace Litchi
{
	
	struct Http11Client : protected TcpSocket
	{

		using EndPointT = typename TcpSocket::EndPointT;

		using PtrT = IntrusivePtr<Http11Client>;

		static auto Create(asio::io_context& Content) -> PtrT { return new Http11Client{Content}; }

		enum class RequestMethodT : uint8_t
		{
			GET,
		};

		struct HexChunkedContentCount
		{
			std::size_t Value = 0;
		};

		static constexpr auto HeadSperator() -> std::u8string_view { return u8"\r\n\r\n"; }
		static constexpr auto SectionSperator() -> std::u8string_view { return u8"\r\n"; }

		static auto RequestLength(RequestMethodT Method, std::u8string_view Target, std::u8string_view Host, std::span<std::byte const> Content, std::u8string_view ContentType) -> std::size_t;
		static auto TranslateRequestTo(std::span<std::byte> OutputBuffer, RequestMethodT Method, std::u8string_view Target, std::u8string_view Host, std::span<std::byte const> Content, std::u8string_view ContentType) -> std::size_t;
		static auto FindHeadSize(std::u8string_view Str) -> std::optional<std::size_t>;
		static auto FindHeadOptionalValue(std::u8string_view Key, std::u8string_view Head) -> std::optional<std::u8string_view>;
		static auto FindHeadContentLength(std::u8string_view Head) -> std::optional<std::size_t>;
		static auto FindSectionLength(std::u8string_view Str) -> std::optional<std::size_t>;
		static auto IsChunkedContent(std::u8string_view Head) -> bool;
		static auto IsResponseHead(std::u8string_view Head) -> bool;
		static auto IsResponseHeadOK(std::u8string_view Head) -> bool;

		enum SectionT
		{
			Head,
			HeadContent,
			ChunkedContent,
			Finish,
		};

		struct Agency : protected TcpSocket::Agency
		{
			template<typename RespondFunction>
			bool Send(std::u8string_view Target, RespondFunction Func, RequestMethodT Method = RequestMethodT::GET, std::span<std::byte const> Content = {}, std::u8string_view ContentType = {})
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, Agency&>)
			{
				return (*this)->Http11Client::AsyncSend(Target, std::move(Func), Method, Content, ContentType);
			}

			template<typename RespondFunction>
			bool Receive(RespondFunction Func)
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, SectionT, std::span<std::byte const>, Agency&>)
			{
				return (*this)->Http11Client::AsyncReceive(std::move(Func));
			}

		protected:
			
			Http11Client* operator->() { return static_cast<Http11Client*>(Socket); }
			Http11Client* operator*() { return static_cast<Http11Client*>(Socket); }
			Agency(TcpSocket::Agency& Age) : TcpSocket::Agency(Age) {}
			Agency(TcpSocket::Agency const& Age) : TcpSocket::Agency(Age) {}
			Agency(Http11Client* Age) : TcpSocket::Agency(Age) {}

			friend struct Http11Client;
			friend struct TcpSocket;
		};

		auto SyncConnect(std::u8string_view Host) -> std::optional<std::tuple<std::error_code, EndPointT>>;

		template<typename WrapperFunction>
		void Lock(WrapperFunction&& Func)
		{
			auto lg = std::lock_guard(SocketMutex);
			Agency Age{this};
			std::forward<WrapperFunction>(Func)(Age);
		}

	protected:

		template<typename RespondFunction>
		bool AsyncSend(std::u8string_view Target, RespondFunction Func, RequestMethodT Method, std::span<std::byte const> Content, std::u8string_view ContentType)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, Agency&>)
		{
			if (TcpSocket::AbleToSend())
			{
				std::vector<std::byte> SendBuffer;
				SendBuffer.resize(RequestLength(Method, Target, Host, Content, ContentType));
				auto Span = std::span(SendBuffer);
				TranslateRequestTo(Span, Method, Target, Host, Content, ContentType);
				return TcpSocket::AsyncSend(Span, [SendBuffer = std::move(SendBuffer), Func = std::move(Func)](std::error_code const& EC, std::size_t ReadSize, TcpSocket::Agency& Age) {
					Agency HttpAge{Age};
					Func(EC, HttpAge);
				});
			}
			return false;
		}

		template<typename RespondFunction>
		bool AsyncConnect(std::u8string_view Host, RespondFunction Func)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, EndPointT, Agency&>)
		{
			return TcpSocket::AsyncConnect(Host, u8"http", [Host = std::u8string{Host}, Func = std::move(Func)](std::error_code const& EC, EndPointT EP, TcpSocket::Agency& Age){
				Agency HttpAge{Age};
				if(!EC)
					HttpAge->Host = std::move(Host);
				Func(EC, std::move(EP), HttpAge);
			});
		}

		template<typename RespondFunction>
		bool AsyncReceive(RespondFunction Func)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, SectionT, std::span<std::byte const>, Agency&>)
		{
			if (AbleToReceive())
			{
				ProtocolReceiving = true;
				this->ReceiveHeadExecute({}, 0, std::move(Func));
				return true;
			}
			return false;
		}

		using TcpSocket::TcpSocket;
		bool ProtocolReceiving = false;
		std::u8string Host;
		std::vector<std::byte> TemporaryBuffer;

	private:

		static constexpr std::size_t CacheSize = 2048; 
		
		bool AbleToReceive() const { return !ProtocolReceiving && TcpSocket::AbleToReceive(); }

		template<typename RespondFunction>
		void ReceiveHeadExecute(std::error_code const& LastEC, std::size_t StartOffset, RespondFunction Func)
		{
			if (LastEC)
			{
				TemporaryBuffer.clear();
				Agency Age{this};
				Func(LastEC, SectionT::Finish, {}, Age);
				return;
			}
				
			auto UsedBuffer = std::span(TemporaryBuffer).subspan(StartOffset);
			if (!UsedBuffer.empty())
			{
				std::u8string_view Head = { reinterpret_cast<char8_t const*>(UsedBuffer.data()), UsedBuffer.size() / sizeof(char8_t)};
				auto HeadSize = FindHeadSize(Head);
				if (HeadSize.has_value())
				{
					const std::size_t ByteSize = *HeadSize * sizeof(char8_t);
					Head = Head.substr(0, *HeadSize);
					if (IsResponseHead(Head))
					{
						auto CurSpan = UsedBuffer.subspan(0, ByteSize);
						Agency Age{this};
						Func(LastEC, SectionT::Head, CurSpan, Age);
						if (IsResponseHeadOK(Head))
						{
							auto ContentLength = FindHeadContentLength(Head);
							if (ContentLength.has_value())
							{
								this->ReceiveHeadContentExecute(LastEC, StartOffset + ByteSize, *ContentLength, std::move(Func));
							}
							else if (IsChunkedContent(Head))
							{
								this->ReceiveChunkedContentExecute(LastEC, StartOffset + ByteSize, {}, std::move(Func));
							}
							else {
								TemporaryBuffer.erase(TemporaryBuffer.begin(), TemporaryBuffer.begin() + StartOffset + ByteSize);
								ProtocolReceiving = false;
							}
							return;
						}
						Func(LastEC, SectionT::Finish, {}, Age);
						return;
					}
					else {
						this->ReceiveHeadExecute(LastEC, StartOffset + ByteSize, std::move(Func));
						return;
					}
				}
				else {
					TemporaryBuffer.erase(TemporaryBuffer.begin(), TemporaryBuffer.begin() + StartOffset);
				}
			}
			auto OldSize = TemporaryBuffer.size();
			TemporaryBuffer.resize(OldSize + CacheSize);
			TcpSocket::AsyncReceiveSome(std::span(TemporaryBuffer).subspan(OldSize), [OldSize, Func = std::move(Func)](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable {
				Agency HttpAge{Age};
				HttpAge->TemporaryBuffer.resize(OldSize + Read);
				if (!EC)
				{
					HttpAge->ReceiveHeadExecute(EC, 0, std::move(Func));
				}
				else {
					HttpAge->TemporaryBuffer.clear();
					HttpAge->ProtocolReceiving = false;
					Func(EC, SectionT::Finish, {}, HttpAge);
				}
			});
		}

		template<typename RespondFunction>
		void ReceiveHeadContentExecute(std::error_code const& LastEC, std::size_t StartOffset, std::size_t ContentLength, RespondFunction Func)
		{
			if (StartOffset + ContentLength <= TemporaryBuffer.size())
			{
				auto Span = std::span(TemporaryBuffer).subspan(StartOffset, ContentLength);
				Agency HttpAge{this};
				Func(LastEC, SectionT::HeadContent, Span, HttpAge);
				TemporaryBuffer.erase(TemporaryBuffer.begin(), TemporaryBuffer.begin() + StartOffset + ContentLength);
				ProtocolReceiving = false;
				Func(LastEC, SectionT::Finish, Span, HttpAge);
				return;
			}
			else {
				TemporaryBuffer.erase(TemporaryBuffer.begin(), TemporaryBuffer.begin() + StartOffset);
				auto OldSize = TemporaryBuffer.size();
				assert(OldSize < ContentLength);
				TemporaryBuffer.resize(ContentLength);
				TcpSocket::AsyncReceive(std::span(TemporaryBuffer).subspan(OldSize), [OldSize, ContentLength, Func = std::move(Func)](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable{
					Agency HttpAge{Age};
					if (!EC)
					{
						assert(Read + OldSize == ContentLength);
						HttpAge->ReceiveHeadContentExecute(EC, 0, ContentLength, std::move(Func));
					}
					else {
						HttpAge->TemporaryBuffer.clear();
						HttpAge->ProtocolReceiving = false;
						Func(EC, SectionT::Finish, {}, HttpAge);
					}
				});
			}
		}

		template<typename RespondFunction>
		void ReceiveChunkedContentExecute(std::error_code const& LastEC, std::size_t StartOffset, std::optional<std::size_t> Data, RespondFunction Func)
		{
			if (TemporaryBuffer.size() > StartOffset)
			{
				if (Data.has_value())
				{
					if (*Data + StartOffset + SectionSperator().size() <= TemporaryBuffer.size())
					{
						Agency HttpAge{this};
						if (*Data == 0)
						{
							TemporaryBuffer.erase(TemporaryBuffer.begin(), TemporaryBuffer.begin() + StartOffset + SectionSperator().size());
							ProtocolReceiving = false;
							Func(LastEC, SectionT::Finish, {}, HttpAge);
						}
						else {
							auto Span = std::span(TemporaryBuffer).subspan(StartOffset, *Data);
							Func(LastEC, SectionT::ChunkedContent, Span, HttpAge);
							this->ReceiveChunkedContentExecute(LastEC, StartOffset + *Data +  SectionSperator().size(), {}, std::move(Func));
						}
						return;
					}
				}
				else {
					auto Span = std::span(TemporaryBuffer).subspan(StartOffset);
					std::u8string_view Str{reinterpret_cast<char8_t const*>(Span.data()), Span.size() / sizeof(char8_t)};
					auto SectionSize = FindSectionLength(Str);
					if (SectionSize.has_value())
					{
						assert(*SectionSize >= SectionSperator().size());
						Str = Str.substr(0, *SectionSize - SectionSperator().size());
						HexChunkedContentCount Count;
						Potato::StrFormat::DirectScan(Str, Count);
						this->ReceiveChunkedContentExecute(LastEC, StartOffset + *SectionSize, Count.Value, std::move(Func));
						return;
					}
				}
			}
			TemporaryBuffer.erase(TemporaryBuffer.begin(), TemporaryBuffer.begin() + StartOffset);
			auto OldSize = TemporaryBuffer.size();
			if (Data.has_value())
			{
				assert(OldSize < *Data + SectionSperator().size());
				TemporaryBuffer.resize(*Data + SectionSperator().size());
			}
			else {
				TemporaryBuffer.resize(OldSize + CacheSize);
			}
			TcpSocket::AsyncReceiveSome(std::span(TemporaryBuffer).subspan(OldSize), [Data, OldSize, Func = std::move(Func)](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable {
				Agency HttpAge{Age};
				HttpAge->TemporaryBuffer.resize(OldSize + Read);
				if (!EC)
				{
					HttpAge->ReceiveChunkedContentExecute(EC, 0, Data, std::move(Func));
				}
				else {
					Func(EC, SectionT::Finish, {}, HttpAge);
				}
			});
		}

		friend struct IntrusiceObjWrapper;
	};
}

namespace Potato::StrFormat
{
	template<>
	struct Scanner<Litchi::Http11Client::HexChunkedContentCount, char8_t>
	{
		bool Scan(std::u8string_view Par, Litchi::Http11Client::HexChunkedContentCount& Input);
	};
}