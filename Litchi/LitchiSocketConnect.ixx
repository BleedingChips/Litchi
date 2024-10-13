module;

export module LitchiSocket;

import std;
import PotatoPointer;
import PotatoIR;

export namespace Litchi
{

	enum class ErrorT
	{
		None = 0,
		ChannelOccupy,
		Eof,
		HostNotFound,
		Unknow,


	};

	constexpr bool operator *(ErrorT Error) { return Error == ErrorT::None; }

	struct SocketAgency: public Potato::IR::MemoryResourceRecordIntrusiveInterface
	{
		using PtrT = Potato::Pointer::IntrusivePtr<SocketAgency>;

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

		

	protected:

		SocketAgency(Potato::IR::MemoryResourceRecord re) : MemoryResourceRecordIntrusiveInterface(re) {}

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
}