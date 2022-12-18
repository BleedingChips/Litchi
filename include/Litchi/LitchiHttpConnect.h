#pragma once
#include "LitchiSocketConnect.h"
#include <map>
#include <optional>
namespace Litchi
{

	

	struct Http11Angency : protected Tcp::SocketAngency
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