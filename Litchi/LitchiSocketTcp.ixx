module;

#include <cassert>
#include <cerrno>
#include "AsioWrapper/LitchiAsioWrapper.h"

export module LitchiSocketTcp;

import std;
import PotatoPointer;
import LitchiContext;


export namespace Litchi::TCP
{

	struct Socket : public Potato::Pointer::DefaultIntrusiveInterface
	{
		using Ptr = Potato::Pointer::IntrusivePtr<Socket>;
		static Ptr Create(Context::Ptr Owner, std::pmr::memory_resource* IMemoryResource = std::pmr::get_default_resource());

		template<typename FunT>
		void AsyncConnect(std::u8string_view Host, std::u8string_view Server, FunT&& Func, std::pmr::memory_resource* Resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, std::error_code const&, Ptr>);

		bool Connect(std::u8string_view Host, std::u8string_view Server, std::pmr::memory_resource* MemoryResource = std::pmr::get_default_resource())
		{
			std::promise<bool> Pro;
			auto Fur = Pro.get_future();
			AsyncConnect(
				Host, Server,
				[&](std::error_code const& EC, Ptr)
				{
					Pro.set_value(!EC);
				},
				MemoryResource
			);
			return Fur.get();
		}

		template<typename FunT>
		void AsyncSend(std::span<std::byte const*> Data, FunT&& Func, std::pmr::memory_resource* Resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, std::error_code const&, std::size_t, Ptr>);

		template<typename FunT>
		void AsyncReadSome(std::span<std::byte> Data, FunT&& Func, std::pmr::memory_resource* Resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, std::error_code const&, std::size_t, Ptr>)
		{
			Socket::Ptr ThisPtr{ this };
			auto Block = CreateTemporaryBlockKeeper(std::forward<FunT>(Func), std::move(ThisPtr), Resource);
			using Type = decltype(Block);
			if(Block != nullptr)
			{
				Owner->AddRequest();
				SocketPtr->ReceiveSome(
					Data.data(), Data.size(),
					[](void* AppendData, std::error_code const& EC)
					{
						auto Block = static_cast<Type>(AppendData);
						auto OwnerPtr = std::move(Block->Ptr);
						assert(OwnerPtr);
						Block->Func(EC, OwnerPtr);
						DestroyTemporaryBlockKeeper(Block);
						OwnerPtr->Owner->SubRequest();
					}, Block, Resource
				);
			}else
			{
				std::error_code EC{ ENOMEM, std::system_category() };
				Func(
					EC,
					std::move(ThisPtr)
				);
			}
		}

	protected:

		Socket(Context::Ptr Owner, void* Adress, std::pmr::memory_resource* IMResource);
		~Socket();
		virtual void Release() override;

		std::pmr::memory_resource* IMResource = nullptr;
		Context::Ptr Owner;
		AsioWrapper::TCPSocket* SocketPtr = nullptr;

		friend struct Potato::Pointer::DefaultIntrusiveInterface;
	};

	template<typename FunT>
	void Socket::AsyncConnect(std::u8string_view Host, std::u8string_view Server, FunT&& Func, std::pmr::memory_resource* Resource)
		requires(std::is_invocable_v<FunT, std::error_code const&, Ptr>)
	{
		Socket::Ptr ThisPtr{this};
		auto Block = CreateTemporaryBlockKeeper(std::forward<FunT>(Func), std::move(ThisPtr), Resource);
		using Type = decltype(Block);
		if(Block != nullptr)
		{
			Owner->AddRequest();
			SocketPtr->Connect(
				Host.data(), Host.size(),
				Server.data(), Server.size(),
				[](void* AppendData, std::error_code const& EC)
				{
					auto Block = static_cast<Type>(AppendData);
					auto OwnerPtr = std::move(Block->Ptr);
					assert(OwnerPtr);
					Block->Func(EC, OwnerPtr);
					DestroyTemporaryBlockKeeper(Block);
					OwnerPtr->Owner->SubRequest();
				}, Block, Resource
			);
		}else
		{
			std::error_code EC{ ENOMEM, std::system_category()};
			Func(
				EC,
				std::move(ThisPtr)
			);
		}
	}

	template<typename FunT>
	void Socket::AsyncSend(std::span<std::byte const*> Data, FunT&& Func, std::pmr::memory_resource* Resource)
		requires(std::is_invocable_v<FunT, std::error_code const&, std::size_t, Ptr>)
	{
		Socket::Ptr ThisPtr{ this };
		auto Block = CreateTemporaryBlockKeeper(std::forward<FunT>(Func), std::move(ThisPtr), Resource);
		using Type = decltype(Block);
		if (Block != nullptr)
		{
			Owner->AddRequest();
			SocketPtr->Send(
				Data,
				[](void* AppendData, std::error_code const& EC, unsigned long long Sended)
				{
					auto Block = static_cast<Type>(AppendData);
					auto OwnerPtr = std::move(Block->Ptr);
					assert(OwnerPtr);
					Block->Func(EC, Sended, OwnerPtr);
					DestroyTemporaryBlockKeeper(Block);
					OwnerPtr->Owner->SubRequest();
				}, Block, Resource
			);
		}else
		{
			std::error_code EC{ ENOMEM, std::system_category() };
			Func(
				EC,
				std::move(ThisPtr)
			);
		}
	}

	/*
	struct SocketAgency
	{
		using PtrT = Potato::Misc::IntrusivePtr<SocketAgency>;

		template<typename RespondFunction>
		auto AsyncConnect(std::u8string_view Host, std::u8string_view Service, RespondFunction Func) -> bool
			requires(std::is_invocable_v<RespondFunction, ErrorT, SocketAgency&>)
		{
			if (AbleToConnect())
			{
				Connecting = true;
				AddRef();
				ConnectExe(Host, Service, [Func = std::move(Func), this](ErrorT Error) {
					Lock([&, this]() {
						Connecting = false;
						Func(Error, *this);
						SubRef();
					});
				});
				return true;
			}
			else {
				Func(ErrorT::ChannelOccupy, *this);
				return false;
			}
		}

		template<typename RespondFunction>
		auto AsyncSend(std::span<std::byte const> SendBuffer, RespondFunction Func) -> bool
			requires(std::is_invocable_v<RespondFunction, ErrorT, std::size_t, SocketAgency&>)
		{
			if (AbleToSend())
			{
				Sending = true;
				AddRef();
				SendExe(SendBuffer, [Func = std::move(Func), this](ErrorT Error, std::size_t SendSize) {
					Lock([&]() {
						Sending = false;
						Func(Error, SendSize, *this);
						SubRef();
					});
				});
				return true;
			}
			Func(ErrorT::ChannelOccupy, 0, *this);
			return false;
		}

		template<typename RespondFunction>
		auto AsyncReceiveSome(std::span<std::byte> PersistenceBuffer, RespondFunction Func) -> bool
			requires(std::is_invocable_v<RespondFunction, ErrorT, std::size_t, SocketAgency&>)
		{
			if (AbleToReceive())
			{
				Receiving = true;
				AddRef();
				ReceiveSomeExe(PersistenceBuffer, [Func = std::move(Func), this](ErrorT Error, std::size_t ReceivedSize) {
					Lock([&]() {
						Receiving = false;
						Func(Error, ReceivedSize, *this);
						SubRef();
					});
				});
				return true;
			}
			Func(ErrorT::ChannelOccupy, 0, *this);
			return false;
		}

		auto Close() -> void {
			CloseExe();
			ResetState();
		}

		auto Cancel() -> void {
			CancelExe();
			ResetState();
		}

		bool IsConnecting() const { return Connecting; }
		bool IsSending() const { return Sending; }
		bool IsReceiving() const { return Receiving; }
		virtual std::u8string_view GetCurrentIpAdress() const { return u8""; };

		void AddRef() const { RefCount.AddRef(); }
		void SubRef() const { if (RefCount.SubRef()) { Release(); } }

		SocketAgency() {}

	protected:

		bool AbleToConnect() const { return !IsConnecting() && !IsSending() && !IsReceiving(); }
		bool AbleToSend() const { return !IsConnecting() && !IsSending(); }
		bool AbleToReceive() const { return !IsConnecting() && !IsReceiving(); }

		template<typename RespondFunction>
		auto Lock(RespondFunction&& Func)
		{
			auto Lg = std::lock_guard(SocketMutex);
			Func();
		}

		std::mutex SocketMutex;
		mutable Potato::Misc::AtomicRefCount RefCount;
		bool Connecting = false;
		bool Sending = false;
		bool Receiving = false;

		void ResetState()
		{
			Connecting = false;
			Sending = false;
			Receiving = false;
		}

		virtual void ConnectExe(std::u8string_view Host, std::u8string_view Service, std::function<void(ErrorT)>) = 0;
		virtual void SendExe(std::span<std::byte const> SendBuffer, std::function<void(ErrorT, std::size_t)>) = 0;
		virtual void ReceiveSomeExe(std::span<std::byte> PersistenceBuffer, std::function<void(ErrorT, std::size_t)>) = 0;
		virtual void CloseExe() = 0;
		virtual void CancelExe() = 0;
		virtual void Release() const = 0;
		virtual ~SocketAgency() {}

		template<typename Type>
		friend struct AgencyTWrapper;
	};

	template<typename AgencyT>
	struct AgencyWrapperT : protected AgencyT::PtrT
	{
		template<typename RespondFunc>
		bool Lock(RespondFunc&& Func) requires(std::is_invocable_v<RespondFunc, AgencyT&>)
		{
			if (*this)
			{
				AgencyT::PtrT::GetPointer()->Lock([&]() {
					std::forward<RespondFunc>(Func)(*AgencyT::PtrT::GetPointer());
				});
				return true;
			}
			return false;
		}
		AgencyWrapperT() = default;
		AgencyWrapperT(AgencyWrapperT&&) = default;
		AgencyWrapperT(AgencyWrapperT const&) = default;
		AgencyWrapperT(AgencyT::PtrT Ptr) : AgencyT::PtrT(std::move(Ptr)) {}
		AgencyWrapperT& operator=(AgencyWrapperT const&) = default;
		AgencyWrapperT& operator=(AgencyWrapperT&&) = default;
	};

	using Socket = AgencyWrapperT<SocketAgency>;
	*/
}