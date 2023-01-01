#pragma once
#include "LitchiSocketConnect.h"
#include <map>
#include <optional>
#include <string_view>

namespace Litchi
{
	
	struct Http11
	{

		using EndPointT = typename TcpSocket::EndPointT;

		enum class RequestMethodT : uint8_t
		{
			GET,
		};

		struct RequestT
		{
			RequestMethodT Method = RequestMethodT::GET;
			std::u8string Target;
			std::u8string Context;
		};

		struct RespondT
		{
			std::u8string Respond;
		};

		static auto TranslateRequest(RequestT const& Re, std::u8string_view Host) -> std::u8string;
		static auto ProtocolLength(std::u8string_view Input) -> std::optional<std::size_t>;


		struct Agency : protected TcpSocket::Agency
		{
			
			using PtrT = IntrusivePtr<Agency>;

			static auto Create(asio::io_context& Context) -> PtrT { return new Agency{ Context }; }

			template<typename RespondFunction>
			auto Connect(std::u8string_view Host, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, EndPointT>)
			{
				return TcpSocket::Agency::Connect(Host, u8"http", [This = PtrT{this}, Host = std::u8string{Host}, Func = std::move(Func)](std::error_code EC, EndPointT EP) mutable {
					if (!EC)
					{
						auto lg = std::lock_guard(This->AgencyMutex);
						This->Host = std::move(Host);
					}
					Func(EC, EP);
				});
			}

			template<typename RespondFunction>
			auto Send(RequestT const& Request, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, std::size_t>)
			{
				auto lg = std::lock_guard(AgencyMutex);
				auto Str = TranslateRequest(Request, Host);
				std::span<std::byte const> Span = { reinterpret_cast<std::byte const*>(Str.data()), Str.size() * sizeof(char8_t) };
				return TcpSocket::Agency::Send(
					Span,
					[Str = std::move(Str), Func = std::move(Func)](std::error_code EC, std::size_t Length) mutable {
					Func(EC, Length);
				});
				return true;
			}

			template<typename RespondFunction>
			auto Receive(RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code, RespondT>)
			{
				return TcpSocket::Agency::FlexibleReceive([](std::span<std::byte const> Input) -> std::optional<std::size_t> {
					return ProtocolLength({reinterpret_cast<char8_t const*>(Input.data()), Input.size() / sizeof(char8_t)});
				}, [This = PtrT{ this }, Func = std::move(Func)](std::error_code EC, std::span<std::byte const> Out) mutable {
					RespondT Res;
					Res.Respond = std::u8string{ reinterpret_cast<char8_t const*>(Out.data()), Out.size() / sizeof(char8_t) };
					Func(EC, std::move(Res));
				});
				return true;
			}

		protected:

			Agency(asio::io_context& Context) : TcpSocket::Agency(Context) {}

			std::mutex AgencyMutex;
			std::u8string Host;

			friend struct IntrusiceObjWrapper;
		};

		using PtrT = Agency::PtrT;

		static auto Create(asio::io_context& Context) -> PtrT { return Agency::Create(Context); }
	};
}