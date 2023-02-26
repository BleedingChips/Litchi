import Litchi.Context;

int main()
{
	return 0;
}
/*
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

	using Http = Http11ClientSequencer;

	auto HttpPtr = Http::Create(Context.GetIOContext());

	std::promise<Http11Client::RespondT> Promise;
	std::future<Http11Client::RespondT> Fur = Promise.get_future();

	std::promise<Http11Client::RespondT> Promise2;
	std::future<Http11Client::RespondT> Fur2 = Promise2.get_future();


	HttpPtr->SyncConnect(u8"www.baidu.com");

	HttpPtr->Lock([&](Http::Agency& Age){
		Age.Send({}, [&](std::error_code const& EC, Http::RespondT Res, Http::Agency& Age){
			Promise.set_value(Res);
		});
		Age.Send({}, [&](std::error_code const& EC, Http::RespondT Res, Http::Agency& Age) {
			Promise2.set_value(Res);
		});
	});

	auto Result = Fur.get();
	auto Result2 = Fur2.get();

	{
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
	}

	{
		Potato::Document::Writer Wri(u8"Respond2.txt");
		Wri.Write(Result2.Head);
		Wri.Flush();
		std::ofstream of2("ChunkedContent2.html", std::ios::binary);
		auto Temp = Http11Client::DecompressContent(Result2.Head, Result2.Content);
		if (Temp.has_value())
		{
			of2.write(reinterpret_cast<char*>(Temp->data()), Temp->size());
		}


		of2.flush();
	}

	

	return 0;
}
*/