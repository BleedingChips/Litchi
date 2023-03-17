import Litchi.Context;
import Litchi.Compression;
import Potato.STD;
import Potato.Document;

using namespace Litchi;

int main()
{
	
	std::cout<< "Begin !" << std::endl;

	auto Ptr = Context::CreateBackEnd();

	auto I = Ptr->CreateHttp11();

	std::promise<void> P;

	auto Fur = P.get_future();

	I.Lock([&](Http11Agency& Age){
		std::cout << "Connended" << std::endl;
		Age.AsyncConnect(u8"www.baidu.com", [&](ErrorT Error, Http11Agency& Age) {
			std::cout << "2" << std::endl;
			Age.AsyncRequestHeadOnly(HttpMethodT::Get, {}, {}, {}, [&](
				ErrorT Error, std::size_t Readed, Http11Agency& Age
			)
			{
				std::cout << "3" << std::endl;
				Age.AsyncReceive([&](ErrorT Error, HttpRespondT Res, Http11Agency& Age){
					std::cout << "4" << std::endl;
					Potato::Document::Writer Write(u8"RespondHead.txt");
					Write.Write(Res.Head);
					Write.Flush();

					auto P22 = Http11Agency::DecompressContent(Res.Head, Res.Context);
					std::u8string_view RespondContext;
					if (P22.has_value())
					{
						RespondContext = { reinterpret_cast<char8_t const*>(P22->data()), P22->size() };
					}
					else {
						RespondContext = { reinterpret_cast<char8_t const*>(Res.Context.data()), Res.Context.size() };
					}
					
					Potato::Document::Writer Write2(u8"RespondContext.html");
					Write2.Write(RespondContext);
					Write2.Flush();

					P.set_value();
				});
			});
		});
	});

	

	Fur.get();

	return 0;
}