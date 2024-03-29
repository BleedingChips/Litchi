import std;
import PotatoDocument;
import PotatoTaskSystem;
import LitchiContext;
import LitchiSocketTcp;
import LitchiHttp;
import LitchiCompression;


using namespace Litchi;

int main()
{

	auto TaskSystem = Potato::Task::TaskContext::Create();
	TaskSystem->FireThreads();
	auto Con = Context::Create(TaskSystem);
	auto Res = TCP::Socket::Create(Con);

	auto Http1 = Http::Http11::Create(Con);

	std::promise<bool> Uio;

	auto Fur = Uio.get_future();

	Http1->AsyncConnect(
		u8"www.baidu.com",
		[&](std::error_code const& EC)
		{
			Uio.set_value(!EC);
		}
	);

	bool Value = Fur.get();

	

	if (Value)
	{
		std::promise<bool> Uio2;

		auto Fur2 = Uio2.get_future();

		std::cout << "Request Head Only" << std::endl;

		Http1->AsyncRequestHeadOnly(
			Http::MethodT::Get,
			{}, {}, {}, [&](std::error_code const& EC)
			{
				Uio2.set_value(!EC);
			}
		);

		auto Value2 = Fur2.get();

		std::u8string Str;

		std::promise<Http::Respond> PRes;

		auto F2 = PRes.get_future();

		Http1->AsyncReceive(
			[&](std::error_code const& RC, Http::Respond Res)
			{
				PRes.set_value(std::move(Res));
			}
		);

		auto Iu = F2.get();

		volatile int i = 0;


		/*
		Age.AsyncRequestHeadOnly(HttpMethodT::Get, {}, {}, {}, [&](
			ErrorT Error, std::size_t Readed, Http11Agency& Age
			)
			{
				std::cout << "Request Down" << std::endl;

				if (*Error)
				{
					std::cout << "Start Receive" << std::endl;
					Age.AsyncReceive([&](ErrorT Error, HttpRespondT Res, Http11Agency& Age) {
						std::cout << "Receive Down" << std::endl;
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
				}
				else {
					std::cout << "Receive Fauilt :" << static_cast<std::size_t>(Error) << std::endl;
					P.set_value();
				}
			});
			*/
	}

	Res.Reset();
	TaskSystem->WaitTask();

	/*
	std::cout<< "Begin !" << std::endl;

	auto Ptr = Context::CreateBackEnd();

	auto I = Ptr->CreateHttp11();

	std::promise<void> P;

	auto Fur = P.get_future();

	I.Lock([&](Http11Agency& Age){
		std::cout << "Connenct" << std::endl;
		Age.AsyncConnect(u8"www.baidu.com", [&](ErrorT Error, Http11Agency& Age) {
			std::cout << "Connect Done" << std::endl;
			if (*Error)
			{
				std::cout << "Request Head Only" << std::endl;
				Age.AsyncRequestHeadOnly(HttpMethodT::Get, {}, {}, {}, [&](
					ErrorT Error, std::size_t Readed, Http11Agency& Age
					)
					{
						std::cout << "Request Down" << std::endl;

						if (*Error)
						{
							std::cout << "Start Receive" << std::endl;
							Age.AsyncReceive([&](ErrorT Error, HttpRespondT Res, Http11Agency& Age) {
								std::cout << "Receive Down" << std::endl;
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
						}
						else {
							std::cout << "Receive Fauilt :" << static_cast<std::size_t>(Error) << std::endl;
							P.set_value();
						}
					});
			}
			else {
				std::cout << "Connect Fauilt :" << static_cast<std::size_t>(Error) << std::endl;
				P.set_value();
			}
			
		});
	});

	

	Fur.get();
	*/

	return 0;
}