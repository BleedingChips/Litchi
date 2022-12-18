#include "Litchi/LitchiContext.h"
#include "Litchi/LitchiSocketConnect.h"
#include "Potato/PotatoStrEncode.h"
#include "Potato/PotatoDocument.h"
#include <chrono>
#include <iostream>

using namespace Litchi;

int main()
{
	/*
	auto Http = HttpConnection::Create(u8"www.baidu.com", 80);
	*/
	{
		Context Context;
		//std::this_thread::sleep_for(std::chrono::seconds{1});
		Tcp::SocketAngency::Ptr Ptr = Tcp::SocketAngency::Create(Context);
		auto Future = Ptr->Connect(u8"www.baidu.com", u8"http");
		auto Re = Future.get();
		std::u8string_view Request = u8"GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";
		std::promise<std::error_code> SRP;
		auto SRFuture = SRP.get_future();

		auto RSpan = std::span(Request);
		std::span<std::byte const> TRSpan{reinterpret_cast<std::byte const*>(RSpan.data()), RSpan.size() * sizeof(char8_t)};


		Ptr->Send(TRSpan, [SRP = std::move(SRP)](std::error_code SR) mutable {
			SRP.set_value(SR);
		});

		auto R1 = SRFuture.get();

		std::array<std::byte, 1024> TemBuffer;

		std::promise<std::u8string_view> RecP;
		auto RECF = RecP.get_future();

		auto CurSpan = std::span(TemBuffer);
		std::this_thread::sleep_for(std::chrono::seconds{1});

		Ptr->Recive(CurSpan, [&](std::error_code RR, std::size_t WC) mutable {
			RecP.set_value(std::u8string_view{reinterpret_cast<char8_t*>(CurSpan.data()), WC});
		});

		auto Re233 = RECF.get();

		//auto Info = Potato::StrEncode::StrEncoder<char8_t, wchar_t>::RequireSpace(Re233);

		/*
		Potato::Document::Writer Write(u8"HTTPOut.txt");
		Write.Write(Re233);
		Write.Flush();
		*/

		/*
		std::wstring Buffer;
		Buffer.resize(Info.TargetSpace);
		Potato::StrEncode::StrEncoder<char8_t, wchar_t>::EncodeUnSafe(Re233, Buffer, Info.CharacterCount);
		std::wcout << Buffer << std::endl;
		*/


		volatile int i = 0;
	}
	return 0;
}