#pragma once
#include "LitchiSocketConnect.h"
#include "Potato/PotatoMisc.h"
#include "Potato/PotatoIntrusivePointer.h"
#include <map>
#include <optional>
namespace Litchi
{
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


}