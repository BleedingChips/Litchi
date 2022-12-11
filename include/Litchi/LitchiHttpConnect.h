#pragma once
#include "asio.hpp"
#include "Potato/PotatoMisc.h"
#include "Potato/PotatoIntrusivePointer.h"
#include <map>
namespace Litchi
{

	struct HttpRequest
	{
		

		

		
	};

	struct HttpRespond
	{

	};


	struct HttpConnection
	{

		struct Wrapper
		{
			void AddRef(HttpConnection* In){ In->RefCount.AddRef(); }
			void SubRef(HttpConnection* In){ if(In->RefCount.SubRef()) delete In; }
		};

		using Ptr = Potato::Misc::IntrusivePtr<HttpConnection, HttpConnection::Wrapper>;

		static Ptr Create(std::u8string_view Host, uint16_t Port = 80);

		enum class Method
		{
			GET
		};

		enum class Version
		{
			_1
		};

		struct Request
		{
			Method RequestMethod;
			Version RequestVersion;
			std::u8string Target;
			std::map<std::u8string, std::u8string> AdditionalOptional;
			std::u8string RequestData;
		};

		struct Respond
		{
			std::u8string TotalRespondData;
			Version RespondVersion;
			std::size_t StatuCode;
			Potato::Misc::IndexSpan<> ServerType;
			Potato::Misc::IndexSpan<> ContentType;
			Potato::Misc::IndexSpan<> Content;
		};

		void SendRequest(HttpRequest const& Request);

	private:

		Potato::Misc::AtomicRefCount RefCount;
		asio::io_context Context;
		asio::ip::tcp::socket Socket;

		HttpConnection() : Context(), Socket(Context) {}
		~HttpConnection();
	};


}