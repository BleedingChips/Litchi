#include "Litchi/LitchiContext.h"
#include "Litchi/LitchiSocketConnect.h"
#include "Potato/PotatoStrEncode.h"
#include "Potato/PotatoDocument.h"
#include <chrono>
#include <iostream>

using namespace Litchi;
using namespace Litchi::Tcp;

int main()
{
	/*
	auto Http = HttpConnection::Create(u8"www.baidu.com", 80);
	*/
	{
		Context Context;
		//std::this_thread::sleep_for(std::chrono::seconds{1});
		auto Ptr = SocketAngency<>::Create(Context.GetIOContext());

		{
			std::promise<std::error_code> Pro1;
			auto Fur = Pro1.get_future();
			Ptr->Connect(u8"www.baidu.com", u8"http", [&](std::error_code EC, SocketAngency<>::EndPointT) {
				Pro1.set_value(EC);
				});

			auto Re = Fur.get();
		}

		std::u8string_view Request = u8"GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";

		{
			std::promise<std::error_code> Pro1;
			auto Fur = Pro1.get_future();

			auto RSpan = std::span(Request);
			std::span<std::byte const> TRSpan{ reinterpret_cast<std::byte const*>(RSpan.data()), RSpan.size() * sizeof(char8_t) };

			Ptr->Send(TRSpan, [&](std::error_code SR) mutable {
				Pro1.set_value(SR);
			});

			auto Re = Fur.get();
		}
		
		
		{
			std::array<std::byte, 102400> TemBuffer;

			std::promise<std::u8string_view> Pro;
			auto Fur = Pro.get_future();

			auto CurSpan = std::span(TemBuffer);

			std::optional<std::size_t> TotalCount;

			Ptr->Recive(CurSpan, [&](std::error_code EC, SocketAngency<>::ReciveResult EE) mutable -> bool {
				if (EC)
				{
					if (TotalCount.has_value())
					{
						if (EE.TotalRead >= *TotalCount)
						{
							std::u8string_view Str{ reinterpret_cast<char8_t*>(CurSpan.data()), EE.TotalRead };
							Pro.set_value(Str);
							return false;
						}
						else
							return true;
					}
					else {

						//std::u8string_view Str{ reinterpret_cast<char8_t*>(CurSpan.data()), EE.TotalRead };
						//auto Index = Str.find(u8"");
					}
				}
				else {
					std::u8string_view Str{ reinterpret_cast<char8_t*>(CurSpan.data()), EE.TotalRead };
					Pro.set_value(Str);
					return false;
				}
				
				return true;
				
				//Str.find(u8"")

				//Pro.set_value(std::u8string_view{ reinterpret_cast<char8_t*>(CurSpan.data()), WC });
			});

			auto Re233 = Fur.get();

			Potato::Document::Writer Write(u8"HTTPOut.txt");
			Write.Write(Re233);
			Write.Flush();
		}

		

		//auto Info = Potato::StrEncode::StrEncoder<char8_t, wchar_t>::RequireSpace(Re233);

		/*
		
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