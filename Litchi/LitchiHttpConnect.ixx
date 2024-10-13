module;

export module LitchiHttp;

export import PotatoFormat;
export import Litchi.Socket;

export namespace Litchi
{
	enum class HttpMethodT
	{
		Get,
	};

	enum class HttpConnectionT
	{
		None,
		Close,
	};

	struct HttpOptionT
	{
		HttpConnectionT Connection = HttpConnectionT::None;
		std::u8string_view Accept = u8"text/html";
		std::u8string_view AcceptEncoding = u8"gzip";
		std::u8string_view AcceptCharset = u8"utf-8";
	};

	struct HttpContextT
	{
		std::u8string_view Cookie;
	};
}

export namespace Litchi
{

	struct HttpRespondT
	{
		std::u8string_view Head;
		std::span<std::byte const> Context;
	};

	auto FindHeadOptionalValue(std::u8string_view Key, std::u8string_view Head) -> std::optional<std::u8string_view>;

	struct Http11Agency : protected SocketAgency
	{
		template<typename T>
		using AllocatorT = Potato::Misc::AllocatorT<T>;

		using PtrT = Potato::Misc::IntrusivePtr<Http11Agency>;

		using SocketAgency::AddRef;
		using SocketAgency::SubRef;
		using SocketAgency::GetCurrentIpAdress;

		bool IsConnecting() const { return SocketAgency::IsConnecting(); }
		bool IsSending() const { return SocketAgency::IsSending(); }
		bool IsReceiving() const { return SocketAgency::IsReceiving(); }

		void AddRef() const { SocketAgency::AddRef(); }
		void SubRef() const { SocketAgency::SubRef(); }

		Http11Agency(AllocatorT<Http11Agency> Allocator = {})
			: SendingBuffer(Allocator), ReceiveBuffer(Allocator)
		{}

		template<typename RespondFun>
		bool AsyncConnect(std::u8string_view Host, RespondFun Fun)
			requires(std::is_invocable_v<RespondFun, ErrorT, Http11Agency&>)
		{
			if (SocketAgency::AbleToConnect())
			{
				std::u8string THost{Host};
				SocketAgency::AsyncConnect(
					Host,
					u8"Http",
					[Fun = std::move(Fun), THost = std::move(THost)](ErrorT Error, SocketAgency& Age) {
						auto& HttpAge = static_cast<Http11Agency&>(Age);
						if(Error == ErrorT::None)
							HttpAge.Host = std::move(THost);
						Fun(Error, HttpAge);
					}
				);
				return true;
			}
			Fun(ErrorT::ChannelOccupy, *this);
			return false;
		}


		template<typename RespondFun>
		bool AsyncRequestHeadOnly(HttpMethodT Method, std::u8string_view Target, HttpOptionT const& Optional, HttpContextT const& ContextT, RespondFun const& Func)
			requires(std::is_invocable_v<RespondFun, ErrorT, std::size_t, Http11Agency&>)
		{
			constexpr std::u8string_view FormatPar = u8"{} {} HTTP/1.1\r\n{}{}Context-Length: 0\r\n\r\n";
			if (SocketAgency::AbleToSend())
			{
				if(Target.empty())
					Target = std::u8string_view{u8"/"};
				SendingBuffer.clear();
				auto RequireSize = FormatSizeHeadOnlyRequest(Method, Target, Optional, ContextT);
				SendingBuffer.resize(RequireSize);
				FormatToHeadOnlyRequest(std::span(SendingBuffer), Method, Target, Optional, ContextT);
				SocketAgency::AsyncSend(std::span(SendingBuffer), [Func = std::move(Func)](ErrorT Err, std::size_t Send, SocketAgency& Agency) {
					return Func(Err, Send, static_cast<Http11Agency&>(Agency));
				});
				return true;
			}
			return false;
		}

		template<typename RespondFun>
		bool AsyncReceive(RespondFun Func)
			requires(std::is_invocable_v<RespondFun, ErrorT, HttpRespondT, Http11Agency&>)
		{
			if (SocketAgency::AbleToReceive())
			{
				CurrentRespond.clear();
				Status = StatusT::WaitHead;
				RespondHeadLength = 0;
				ReceiveExecute(std::move(Func));
				return true;
			}
			Func(ErrorT::ChannelOccupy, HttpRespondT{}, *this);
			return false;
		}

		static std::optional<std::vector<std::byte>> DecompressContent(std::u8string_view Head, std::span<std::byte const> Res);

	protected:

		std::size_t FormatSizeHeadOnlyRequest(HttpMethodT Method, std::u8string_view Target, HttpOptionT const& Optional, HttpContextT const& ContextT);
		std::size_t FormatToHeadOnlyRequest(std::span<std::byte> Output, HttpMethodT Method, std::u8string_view Target, HttpOptionT const& Optional, HttpContextT const& ContextT);

		template<typename RespondFunc>
		void ReceiveExecute(RespondFunc Func)
		{
			if (!TryGenerateRespond())
			{
				if (ReceiveBufferIndex.Begin() * 2 >= ReceiveBuffer.size())
				{
					ReceiveBuffer.erase(
						ReceiveBuffer.begin(),
						ReceiveBuffer.begin() + ReceiveBufferIndex.Begin()
					);
					ReceiveBufferIndex.Offset = 0;
				}

				std::size_t SuggestSize = 4096;

				if (Status == StatusT::WaitChunkedContext || Status == StatusT::WaitHeadContext)
				{
					SuggestSize = ContextLength;
				}

				ReceiveBuffer.resize(std::max(ReceiveBufferIndex.End() + SuggestSize, ReceiveBuffer.capacity()));
				SocketAgency::AsyncReceiveSome(std::span(ReceiveBuffer).subspan(ReceiveBufferIndex.End()),
					[Func = std::move(Func)](ErrorT EC, std::size_t Receive, SocketAgency& Agency) {
						auto& HAge = static_cast<Http11Agency&>(Agency);
						HAge.ReceiveBuffer.resize(HAge.ReceiveBufferIndex.End() + Receive);
						HAge.ReceiveBufferIndex.Length += Receive;
						if (EC == ErrorT::None)
						{
							HAge.ReceiveExecute(std::move(Func));
						}
						else {
							HAge.ReceiveBuffer.clear();
							HAge.ReceiveBufferIndex = {};
							Func(EC, {}, HAge);
						}
					}
				);
			}
			else {
				HttpRespondT Res;
				Res.Head = std::u8string_view{
					reinterpret_cast<char8_t const*>(CurrentRespond.data()),
					RespondHeadLength
				};
				Res.Context = std::span(CurrentRespond).subspan(RespondHeadLength);
				Func(ErrorT::None, Res, *this);
			}
		}

		bool TryGenerateRespond();

		enum class StatusT
		{
			WaitHead,
			WaitHeadContext,
			WaitChunkedContextHead,
			WaitChunkedContext,
			WaitChunkedEnd,
		};

		std::u8string Host;
		std::vector<std::byte, AllocatorT<std::byte>> SendingBuffer;
		std::vector<std::byte, AllocatorT<std::byte>> ReceiveBuffer;
		Potato::Misc::IndexSpan<> ReceiveBufferIndex;
		std::vector<std::byte, AllocatorT<std::byte>> CurrentRespond;
		StatusT Status = StatusT::WaitHead;
		std::size_t RespondHeadLength = 0;
		std::size_t ContextLength = 0;
		bool ReachEnd = false;

		template<typename Type>
		friend struct AgencyWrapperT;
	};

	using Http11 = AgencyWrapperT<Http11Agency>;
}

