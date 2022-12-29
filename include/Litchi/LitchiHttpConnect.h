#pragma once
#include "LitchiSocketConnect.h"
#include <map>
#include <optional>
#include <string_view>

namespace Litchi
{

	struct Http11
	{

		using EndPointT = typename TcpSocket::EndPointT;

		enum class RequestMethodT : uint8_t
		{
			GET,
		};

		struct RequestT
		{
			RequestMethodT Method = RequestMethodT::GET;
			std::u8string Target;
			std::u8string Context;
		};

		struct RespondT
		{
			std::u8string Respond;
		};

		static auto TranslateRequest(RequestT const& Re, std::u8string_view Host) -> std::u8string;
		static auto ProtocolLength(std::u8string_view Input) -> std::optional<std::size_t>;

		struct Agency : protected TcpSocket::Agency
		{
			
			using PtrT = IntrusivePtr<Agency>;

			template<typename RespondFunction>
			static auto Create(asio::io_context& Context, std::u8string Host, RespondFunction Func) -> void
				requires(std::is_invocable_v<RespondFunction, std::error_code, EndPointT, PtrT>)
			{
				auto Resolver = TcpResolver::Create(Context);
				PtrT This = new Agency{ Context, std::move(Host)};
				auto TResolver = Resolver.GetPointer();
				auto TThis = This.GetPointer();
				TResolver->Resolve(TThis->Host, u8"http", [Func = std::move(Func), Resolver = std::move(Resolver), This = std::move(This)](std::error_code EC, TcpResolver::ResultT RR) mutable {
					if (!EC && RR.size() >= 1)
					{
						std::vector<EndPointT> EndPoints;
						EndPoints.reserve(RR.size() - 1);
						auto Ite = RR.begin();
						EndPointT Top = *Ite;
						++Ite;
						for (; Ite != RR.end(); ++Ite)
							EndPoints.push_back(*Ite);
						std::reverse(EndPoints.begin(), EndPoints.end());
						Agency::ConnectExecute(std::move(This), Top, std::move(EndPoints), std::move(Func));
					}
					else {
						Func(EC, {}, {});
					}
				});
			}

			template<typename RespondFunction>
			static auto SendRequest(RequestT const& Request, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code>)
			{
				auto Str = TranslateRequest(Request, Host);
				auto Span = std::span(Str);
				return TcpSocket::Agency::Send(Span, [Str = std::move(Str), Func = std::move(Func), This = PtrT{this}](std::error_code EC) mutable {
					Func(EC);
				});
			}

			template<typename RespondFunction>
			static auto ReceiveRequest(RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, ReceiveT>)
			{
				Agency::ReceiveExecute(PtrT{this}, std::move(Func));
			}

		protected:

			Agency(asio::io_context& Context, std::u8string Host) : TcpSocket::Agency(Context), Host(std::move(Host)) {}

			auto DetectExecute(TcpSocket::Agency::ReceiveT Re) -> std::optional<std::span<std::byte>>
			{
				auto Lg = std::lock_guard(ReceiveMutex);
				ReceiveBuffer.insert(ReceiveBuffer.end(), ReceiveBuffer.begin(), ReceiveBuffer.begin() + Re.CurrentLength);
				std::u8string_view Str(ReceiveBuffer.data(), ReceiveBuffer.size());
				auto Length = ProtocolLength(Str);
				if (Length.has_value() && *Length <= ReceiveBuffer.size())
				{
					return {};
				}else
					return std::span<std::byte>{reinterpret_cast<std::byte*>(ReceiveBuffer.data()), ReceiveBuffer.size()};
			}

			template<typename RespondFunction>
			static auto ReceiveExecute(PtrT This, RespondFunction Func)
			{
				RespondT Res;
				{
					auto lg = std::lock_guard(This->ReceiveMutex);
					std::u8string_view Str(This->ReceiveBuffer.data(), This->ReceiveBuffer.size());
					auto Length = ProtocolLength(Str);
					if (Length.has_value() && This->ReceiveBuffer.size() >= *Length)
					{
						Res.Respond = std::u8string_view{ This->ReceiveBuffer }.substr(0, *Length);
						ReceiveBuffer.erase(This->ReceiveBuffer.begin(), This->ReceiveBuffer.begin() + *Length);
					}
					else {
						TcpSocket::Agency::Receive(ReceiveBuffer, [This = PtrT{ this }](TcpSocket::Agency::ReceiveT Re) {
							return This->DetectExecute(Re);
						}, [This = PtrT{ this }](std::error_code EC, TcpSocket::Agency::ReceiveT RE) {
							auto lg = std::lock_guard(This->ReceiveMutex);
							std::u8string_view Str(This->ReceiveBuffer.data(), This->ReceiveBuffer.size());
							auto Length = ProtocolLength(Str);
							assert(Length.has_value());
							RespondT NewRes;
							NewRes.Respond = std::u8string_view{ This->ReceiveBuffer }.substr(0, *Length);
							This->ReceiveBuffer.erase(This->ReceiveBuffer.begin(), This->ReceiveBuffer.begin() + *Length);
							Res = std::move(NewRes);
						});
						return;
					}
				}
				Func({}, std::move(Res));
			}

			template<typename RespondFunction>
			static auto ConnectExecute(PtrT This, EndPointT TopEndPoint, std::vector<EndPointT> EndPoints, RespondFunction Func) -> void
			{
				assert(This);
				auto TThis = This.GetPointer();
				auto Re = TThis->Connect(TopEndPoint, [TopEndPoint, This = std::move(This), EndPoints = std::move(EndPoints), Func = std::move(Func)](std::error_code EC) mutable {
					if (!EC)
					{
						Func(EC, TopEndPoint, std::move(This));
					}
					else if (!EndPoints.empty())
					{
						auto Top = std::move(*EndPoints.rbegin());
						EndPoints.pop_back();
						Agency::ConnectExecute(std::move(This), Top, std::move(EndPoints), std::move(Func));
					}
					else {
						Func(EC, {}, {});
					}
				});
				assert(Re);
			}

			std::u8string Host;

			std::mutex ReceiveMutex;
			std::vector<char8_t> ReceiveBuffer;

			std::array<char8_t, 2048> ReceiveBuffer;

			friend struct IntrusiceObjWrapper;
		};

		using PtrT = Agency::PtrT;

		template<typename RespondFunction>
		static auto Create(asio::io_context& Context, std::u8string Host, RespondFunction Func) -> void
			requires(std::is_invocable_v<RespondFunction, std::error_code, EndPointT, PtrT>) { return Agency::Create(Context, std::move(Host), std::move(Func)); }
		
	};

	/*
	struct Http11Angency : protected TcpSocket::
	{

		enum class RequestMethod
		{
			GET,
			POST,
		};

		struct Request
		{
			RequestMethod Methos;
			std::u8string_view Target;
			std::u8string Cookie;
		};

		auto Connection(std::u8string_view Host) -> std::future<std::error_code>;
		auto Close() -> std::future<bool>;

		template<typename RespondFunc>
		void Request();

	private:
		
		std::mutex SendMutex;
		std::vector<std::byte> TemporarySendRequest;
		Potato::Misc::IndexSpan<> SendRequestIndex;
		std::array<std::byte, 1024> CurrentSendRequest;
		std::size_t SendIte;

		std::array<std::byte, 1024> DefaultBuffer;

		std::u8string Host;
	};
	*/


	/*
	struct HttpConnection11 : protected TcpSocketConnection
	{
		bool Connection(std::u8string_view Host, uint16_t Ports = 80);
		bool Close() { return TcpSocketConnection::Close(); }
		uint16_t Ports = 0;
		std::size_t RequestCount = 0;

		enum class Method
		{
			OPT,
			GET,
			HEA,
			POS,
			PUT,
			DEL,
			TRA,
			CON
		};

		struct Request
		{
			std::u8string_view Target;
			Method RequestMethod;
			bool KeepAlive;
			std::u8string_view AcceptLanguage;
		};

		struct Respond
		{
			std::size_t RequestCount;
			std::size_t MaxRequestCount;
			std::size_t RespondCode;
			std::u8string_view RespondStatus;
			std::map<std::u8string_view, std::u8string_view> Parmetters;
			std::span<std::byte const> Datas;
		};

		template<typename RespondFunction>
		std::optional<std::size_t> Request(Request const& Request, RespondFunction Function)
			requires(std::is_invocable_v<RespondFunction, Respond>)
		{
			return {};
		}
	};
	*/

}