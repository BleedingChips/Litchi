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

	std::promise<Http11Client::RespondT> Promise;
	std::future<Http11Client::RespondT> Fur = Promise.get_future();


	HttpPtr->SyncConnect(u8"www.baidu.com");

	HttpPtr->Lock([&](Http::Agency& Age){
		Age.Send({}, [&](std::error_code const& EC, Http::Agency& Age){
			if (!EC)
			{
				Age.ReceiveRespond([&](std::error_code const& EC, Http11Client::RespondT T, Http11Client::Agency& Age){
					Promise.set_value(T);
				});
			}
			else {
				Promise.set_value({});
			}
		});
	});

	auto Result = Fur.get();

	Potato::Document::Writer Wri(u8"Respond.txt");
	Wri.Write(Result.Head);
	Wri.Flush();
	std::ofstream of2("ChunkedContent.html", std::ios::binary);
	auto Temp = Http11Client::DecompressContent(Result.Head, Result.Content);
	if (Temp.has_value())
	{
		of2.write(reinterpret_cast<char*>(Temp->data()), Temp->size());
	}
	
	
	of2.flush();

	return 0;
}