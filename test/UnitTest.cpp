#include "Litchi/LitchiContext.h"
#include "Litchi/LitchiSocketConnect.h"
#include "Litchi/LitchiHttpConnect.h"
#include "Potato/PotatoStrEncode.h"
#include "Potato/PotatoDocument.h"
#include "Litchi/LitchiCompression.h"
#include <chrono>
#include <iostream>

using namespace Litchi;
int main()
{

	Context::MulityThreadAgency<> Context{1};

	using Http = Http11Client;

	auto HttpPtr = Http::Create(Context.GetIOContext());

	std::u8string Head;
	std::u8string HeadContent;
	std::vector<std::byte> ChunkedContent;

	std::promise<void> Promise;
	std::future<void> Fur = Promise.get_future();


	HttpPtr->SyncConnect(u8"www.baidu.com");

	HttpPtr->Lock([&](Http::Agency& Age){
		Age.Send({}, [&](std::error_code const& EC, Http::Agency& Age){
			if (!EC)
			{
				Age.Receive([&](std::error_code const& EC, Http::SectionT Section, std::span<std::byte const> Buffer, Http::Agency&){
					std::cout << Buffer.size() << std::endl;
					switch (Section)
					{
					case Http::SectionT::Head:
						Head = std::u8string{reinterpret_cast<char8_t const*>(Buffer.data()), Buffer.size() / sizeof(char8_t)};
						break;
					case Http::SectionT::HeadContent:
						HeadContent = std::u8string{ reinterpret_cast<char8_t const*>(Buffer.data()), Buffer.size() / sizeof(char8_t) };
						break;
					case Http::SectionT::ChunkedContent:
						ChunkedContent.insert(ChunkedContent.end(), Buffer.begin(), Buffer.end());
						break;
					case Http::SectionT::Finish:
						Promise.set_value();
						break;
					}
				});
			}
			else {
				Promise.set_value();
			}
		});
	});

	Fur.get();

	Potato::Document::Writer Wri(u8"Respond.txt");
	Wri.Write(Head);
	Wri.Write(HeadContent);
	Wri.Flush();

	std::ofstream of("ChunkedContent.gz", std::ios::binary);
	of.write(reinterpret_cast<char*>(ChunkedContent.data()), ChunkedContent.size());
	of.flush();

	std::vector<std::byte> Decompression;

	Litchi::GZipDecompress(ChunkedContent, [&](std::span<std::byte const> Input){
		Decompression.insert(Decompression.end(), Input.begin(), Input.end());
	});

	std::ofstream of2("ChunkedContent.html", std::ios::binary);
	of2.write(reinterpret_cast<char*>(Decompression.data()), Decompression.size());
	of2.flush();

	return 0;
}