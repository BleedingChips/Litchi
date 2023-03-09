module;

export module Litchi.Http;

export import Litchi.Socket;

namespace Litchi
{
	
	struct Http11Agency : protected SocketAgency
	{

		using SocketAgency::AddRef;
		using SocketAgency::SubRef;
		using SocketAgency::GetCurrentIpAdress;

		void AddRef() const { SocketAgency::AddRef(); }
		void SubRef() const { SocketAgency::SubRef(); }

		Http11Agency(AllocatorT<Http11Agency> Alloctaor) : SocketAgency(Allocator) {}

		std::vector<std::byte, AllocatorT<std::byte>> Receive;

		virtual std::u8string_view GetCurrentIpAdress() const { return u8""; };

	};


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
		static auto DecompressContent(std::u8string_view Head, std::span<std::byte const> Content) -> std::optional<std::vector<std::byte>>;

		struct RespondT
		{
			std::u8string Head;
			std::vector<std::byte> Content;

			bool AutoDecompress() {
				auto Re = DecompressContent(Head, Content);
				if (Re.has_value())
				{
					Content = std::move(*Re);
					return true;
				}
				return false;
			}

			std::u8string_view ContentToStringView() const { return {reinterpret_cast<char8_t const*>(Content.data()), Content.size()}; }
		};

		enum SectionT
		{
			Head,
			HeadContent,
			ChunkedContent,
			Finish,
		};

		static std::optional<std::span<std::byte>> HandleRespond(RespondT& Res, SectionT Section, std::size_t SectionCount);

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
				requires(std::is_invocable_r_v<std::span<std::byte>, RespondFunction, std::error_code const&, SectionT, std::size_t, Agency&>)
			{
				return (*this)->Http11Client::AsyncReceive(std::move(Func));
			}

			template<typename RespondFunction>
			bool ReceiveRespond(RespondFunction Func)
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, RespondT, Agency&>)
			{
				return (*this)->Http11Client::AsyncReceiveRespond(std::move(Func));
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
		bool AsyncSend(std::span<std::byte> RowData, RespondFunction Func)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, Agency&>)
		{
			if (TcpSocket::AbleToSend())
			{
				return TcpSocket::AsyncSend(RowData, [Func = std::move(Func)](std::error_code const& EC, std::size_t ReadSize, TcpSocket::Agency& Age) mutable {
					Agency HttpAge{ Age };
					Func(EC, HttpAge);
				});
			}
			return false;
		}

		template<typename RespondFunction>
		bool AsyncConnect(std::u8string_view Host, RespondFunction Func)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, EndPointT, Agency&>)
		{
			return TcpSocket::AsyncConnect(Host, u8"http", [Host = std::u8string{Host}, Func = std::move(Func)](std::error_code const& EC, EndPointT EP, TcpSocket::Agency& Age) mutable {
				Agency HttpAge{Age};
				if(!EC)
					HttpAge->Host = std::move(Host);
				Func(EC, std::move(EP), HttpAge);
			});
		}

		template<typename RespondFunction>
		bool AsyncReceive(RespondFunction Func)
			requires(std::is_invocable_r_v<std::span<std::byte>, RespondFunction, std::error_code const&, SectionT, std::size_t, Agency&>)
		{
			if (AbleToReceive())
			{
				ProtocolReceiving = true;
				this->ReceiveHeadExecute(std::move(Func));
				return true;
			}
			return false;
		}

		template<typename RespondFunction>
		bool AsyncReceiveRespond(RespondFunction Func)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, RespondT, Agency&>)
		{
			return this->AsyncReceive([Func = std::move(Func), Res = RespondT{}](std::error_code const& EC, SectionT Section, std::size_t SectionCount, Agency& Age) mutable -> std::span<std::byte> {
				if (!EC)
				{
					auto Re = HandleRespond(Res, Section, SectionCount);
					if (Re.has_value())
						return *Re;
					else {
						Func({}, std::move(Res), Age);
					}
				}
				else {
					Func(EC, {}, Age);
				}
			return {};
				});
		}

		using TcpSocket::TcpSocket;
		bool ProtocolReceiving = false;
		std::u8string Host;
		Potato::Misc::IndexSpan<> TIndex;
		std::vector<std::byte> TBuffer;

		bool AbleToReceive() const { return !ProtocolReceiving && TcpSocket::AbleToReceive(); }

	private:
		
		

		std::span<std::byte> PrepareTBufferForReceive();
		std::span<std::byte> ConsumeTBuffer(std::size_t MaxSize);
		std::span<std::byte> ConsumeTBufferTo(std::span<std::byte> Output);
		void ReceiveTBuffer(std::size_t ReceiveSize);
		void ClearTBuffer();

		struct HeadR
		{
			std::span<std::byte> HeadData;
			bool IsOK = false;
			bool NeedChunkedContent = false;
			std::optional<std::size_t> HeadContentSize;
		};

		std::optional<HeadR> ConsumeHead();
		std::optional<std::size_t> ConsumeChunkedContentSize(bool NeedContentEnd);
		bool TryConsumeContentEnd();

		template<typename RespondFunction>
		void ReceiveHeadExecute(RespondFunction Func)
		{
			auto Head = ConsumeHead();
			if (Head.has_value())
			{
				Agency Age{this};
				auto Span = Func({}, SectionT::Head, Head->HeadData.size(), Age);
				std::copy_n(Head->HeadData.begin(), Head->HeadData.size(), Span.begin());
				if (Head->IsOK)
				{
					if (Head->HeadContentSize.has_value())
					{
						this->ReceiveHeadContentExecute(*Head->HeadContentSize, std::move(Func));
						return;
					}
					else if (Head->NeedChunkedContent)
					{
						this->ReceiveChunkedContentExecute(false, std::move(Func));
						return;
					}
				}
				ProtocolReceiving = false;
				Func({}, SectionT::Finish, 0, Age);
				return;
			}
			else {
				auto Span = PrepareTBufferForReceive();
				TcpSocket::AsyncReceiveSome(Span, [Func = std::move(Func)](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable {
					Agency Http{Age};
					if (!EC)
					{
						Http->ReceiveTBuffer(Read);
						Http->ReceiveHeadExecute(std::move(Func));
					}
					else {
						Http->ProtocolReceiving = false;
						Http->ClearTBuffer();
						Func(EC, SectionT::Head, 0, Http);
					}
				});
			}
		}

		template<typename RespondFunction>
		void ReceiveHeadContentExecute(std::size_t RequireSize, RespondFunction Func)
		{
			Agency Age{this};
			auto Output = Func({}, SectionT::HeadContent, RequireSize, Age);
			auto O1 = ConsumeTBufferTo(Output);
			if (!O1.empty())
			{
				TcpSocket::AsyncReceive(O1, [Size = Output.size() - O1.size(), Func = std::move(Func)](std::error_code const& EC, std::size_t ReadSize, TcpSocket::Agency& Age) mutable {
					Agency Http{Age};
					Http->ProtocolReceiving = false;
					if (EC)
					{
						Http->ClearTBuffer();
						Func(EC, SectionT::HeadContent, ReadSize + Size, Http);
					}
					else {
						Func(EC, SectionT::Finish, 0, Http);
					}
				});
			}
			else {
				Age->ProtocolReceiving = false;
				Func({}, SectionT::Finish, 0, Age);
			}
		}

		template<typename RespondFunc>
		void ReceiveChunkedContentExecute(bool ReceiveContentEnd, RespondFunc Func)
		{
			auto Re = ConsumeChunkedContentSize(ReceiveContentEnd);
			if (Re.has_value())
			{
				Agency Http{this};
				if (*Re != 0)
				{
					auto Span = Func({}, SectionT::ChunkedContent, *Re, Http);
					auto S1 = ConsumeTBufferTo(Span);
					if (!S1.empty())
					{
						TcpSocket::AsyncReceive(S1, [Func = std::move(Func), Size = Span.size() - S1.size()](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable {
							Agency Http{Age};
							if (!EC)
							{
								Http->ReceiveChunkedContentExecute(true, std::move(Func));
							}
							else {
								Http->ClearTBuffer();
								Http->ProtocolReceiving = false;
								Func(EC, SectionT::ChunkedContent, Size + Read, Http);
							}
						});
					}
					else {
						ReceiveChunkedContentExecute(true, std::move(Func));
					}
				}
				else {
					if (TryConsumeContentEnd())
					{
						ProtocolReceiving = false;
						Func({}, SectionT::Finish, 0, Http);
					}
					else {
						auto Span = PrepareTBufferForReceive();
						TcpSocket::AsyncReceive(Span.subspan(0, 2), [Func = std::move(Func)](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable {
							Agency Http{Age};
							Http->ReceiveTBuffer(Read);
							bool Re = Http->TryConsumeContentEnd();
							assert(Re);
							Http->ProtocolReceiving = false;
							Func({}, SectionT::Finish, 0, Http);
						});
					}
				}
			}
			else {
				auto Output = PrepareTBufferForReceive();
				TcpSocket::AsyncReceiveSome(Output, [ReceiveContentEnd, Func = std::move(Func)](std::error_code const& EC, std::size_t Read, TcpSocket::Agency& Age) mutable{
					Agency Http{Age};
					if (!EC)
					{
						Http->ReceiveTBuffer(Read);
						Http->ReceiveChunkedContentExecute(ReceiveContentEnd, std::move(Func));
					}
					else {
						Http->ClearTBuffer();
						Http->ProtocolReceiving = false;
						Func(EC, SectionT::ChunkedContent, Read, Http);
					}
				});
			}
		}

		friend struct IntrusiceObjWrapper;
	};

	struct Http11ClientSequencer : protected Http11Client
	{
		using PtrT = IntrusivePtr<Http11ClientSequencer>;
		static auto Create(asio::io_context& Content) -> PtrT { return new Http11ClientSequencer{Content}; }
		using EndPointT = Http11Client::EndPointT;
		using RespondT = Http11Client::RespondT;

		auto SyncConnect(std::u8string_view Host) -> std::optional<std::tuple<std::error_code, EndPointT>>;

		struct Agency : protected Http11Client::Agency
		{
			template<typename RespondFunction>
			bool Send(std::u8string_view Target, RespondFunction Func, RequestMethodT Method = RequestMethodT::GET, std::span<std::byte const> Content = {}, std::u8string_view ContentType = {})
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, RespondT, Agency&>)
			{
				return (*this)->AsyncSend(Target, std::move(Func), Method, Content, ContentType);
			}
		protected:
			Http11ClientSequencer* operator->() { return static_cast<Http11ClientSequencer*>(Socket); }
			Agency(Http11ClientSequencer* Ptr) : Http11Client::Agency(Ptr) {}
			Agency(Http11Client::Agency& Age) : Http11Client::Agency(Age) {}

			friend struct Http11ClientSequencer;
		};

		template<typename WraFunc>
		void Lock(WraFunc Func)
			requires(std::is_invocable_v<WraFunc, Agency&>)
		{
			Http11Client::Lock([Func = std::move(Func)](LastAgency& LAge) {
				Agency Age{LAge};
				Func(Age);
			});
		}


	protected:

		using LastAgency = Http11Client::Agency;

		template<typename ResFunc>
		bool AsyncConnect(std::u8string_view Host, ResFunc Func)
			requires(std::is_invocable_v<ResFunc, std::error_code const&, EndPointT, Agency&>)
		{
			return Http11Client::AsyncConnect(Host, [Func = std::move(Func)](std::error_code const& EC, EndPointT EP, LastAgency& LAge) mutable {
				Agency Age{ LAge };
				Func(EC, std::move(EP), Age);
				Age->ExecuteSendList();
			});
		}

		template<typename RespondFunction>
		bool AsyncSend(std::u8string_view Target, RespondFunction Func, RequestMethodT Method, std::span<std::byte const> Content, std::u8string_view ContentType)
			requires(std::is_invocable_v<RespondFunction, std::error_code const&, RespondT, Agency&>)
		{
			std::vector<std::byte> SendBuffer;
			SendBuffer.resize(RequestLength(Method, Target, Host, Content, ContentType));
			auto Span = std::span(SendBuffer);
			TranslateRequestTo(Span, Method, Target, Host, Content, ContentType);

			if (AbleToSend())
			{
				bool Re = Http11Client::AsyncSend(Span, [SendBuffer = std::move(SendBuffer), Func = std::move(Func)](std::error_code const& EC, Http11Client::Agency& Age) mutable {
					Agency Seque{ Age };
					if (!EC)
					{
						if (Seque->AbleToReceive())
						{
							bool Re = Seque->Http11Client::AsyncReceiveRespond([Func = std::move(Func)](std::error_code const& EC, RespondT Res, LastAgency& LAge) mutable {
								Agency Age{LAge};
								Func(EC, Res, Age);
								Age->ExecuteReceiveList();
							});
							assert(Re);
						}
						else {
							Seque->RespondList.push_back(std::move(Func));
						}
					}
					else
					{
						Func(EC, {}, Seque);
					}
					Seque->ExecuteSendList();
				});
				assert(Re);
				return true;
			}
			else {
				SendList.push_back({std::move(SendBuffer), std::move(Func)});
				return false;
			}
		}

		struct SendT
		{
			std::vector<std::byte> Buffer;
			std::function<void(std::error_code const&, RespondT, Agency&)> Func;
		};

		std::deque<SendT> SendList;
		std::deque<std::function<void(std::error_code const&, RespondT, Agency&)>> RespondList;

		Http11ClientSequencer(asio::io_context& Content) : Http11Client(Content) {}

	private:

		bool ExecuteSendList();
		bool ExecuteReceiveList();

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