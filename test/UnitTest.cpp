#include "Litchi/LitchiContext.h"
#include "Litchi/LitchiSocketConnect.h"
#include "Litchi/LitchiHttpConnect.h"
#include "Potato/PotatoStrEncode.h"
#include "Potato/PotatoDocument.h"
#include <chrono>
#include <iostream>

using namespace Litchi;
int main()
{

	Context::MulityThreadAgency<> Context{1};

	auto HttpPtr = Http11::Create(Context.GetIOContext());

	Http11::RequestT Re;

	auto Re2 = Http11::TranslateRequest(Re, u8"www.baidu.com");

	std::promise<void> Pro;

	auto Fur = Pro.get_future();

	HttpPtr->Connect(u8"www.baidu.com", [&](std::error_code, TcpSocket::EndPointT EP){ Pro.set_value(); });

	Fur.get();

	std::promise<Http11::RespondT> Pro2;

	auto Fur2 = Pro2.get_future();

	HttpPtr->Send(Re, [&](std::error_code EC, std::size_t){
		if (!EC)
		{
			HttpPtr->Receive([&](std::error_code EC, Http11::RespondT Res){Pro2.set_value(std::move(Res));});
		}else
			Pro2.set_value({});
	});

	auto P22 = Fur2.get();

	Potato::Document::Writer Wri(u8"asdasd.txt");
	Wri.Write(P22.Respond);
	Wri.Flush();

	volatile int i = 0;

	return 0;
}