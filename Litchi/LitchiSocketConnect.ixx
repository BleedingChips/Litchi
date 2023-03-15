module;

export module Litchi.Socket;

export import Litchi.Memory;
export import Potato.SmartPtr;

export namespace Litchi
{

	enum class ErrorT : uint8_t
	{
		None = 0,
		ChannelOccupy,
		Unknow,
	};

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


	/*
	struct TcpSocket
	{
		using EndPointT = asio::ip::tcp::endpoint;
		using ResolverT = asio::ip::tcp::resolver::results_type;
		using PtrT = IntrusivePtr<TcpSocket>;

		static auto Create(asio::io_context& Context) -> PtrT { return new TcpSocket(Context); }

		struct Agency
		{

			template<typename RespondFunction>
			auto Connect(EndPointT EP, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, EndPointT, Agency&>)
			{
				return Socket->AsyncConnect(EP, std::move(Func));
			}

			template<typename RespondFunction>
			auto Connect(std::u8string_view Host, std::u8string_view Service, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, EndPointT, Agency&>)
			{
				return Socket->AsyncConnect(Host, Service, std::move(Func));
			}
				
			template<typename RespondFunction>
			auto Send(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, std::size_t, Agency&>)
			{
				return Socket->AsyncSend(PersistenceBuffer, std::move(Func));
			}

			template<typename RespondFunction>
			auto ReceiveSome(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, std::size_t, Agency&>)
			{
				return Socket->AsyncReceiveSome(PersistenceBuffer, std::move(Func));
			}

			template<typename RespondFunction>
			auto Receive(std::span<std::byte const> PersistenceBuffer, RespondFunction Func) -> bool
				requires(std::is_invocable_v<RespondFunction, std::error_code const&, std::size_t, Agency&>)
			{
				return Socket->AsyncReceive(PersistenceBuffer, std::move(Func));
			}

			void Cancel() { Socket -> CancelExecute(); }
			void Close() { Socket->CloseExecute(); }

			bool AbleToConnect() const { return Socket->AbleToConnect(); }
			bool AbleToSend() const { return Socket->AbleToSend(); }
			bool AbleToReceive() const { return Socket->AbleToReceive(); }

		protected:

			Agency(Agency const&) = default;
			Agency(TcpSocket* Socket) : Socket(Socket) {}
			TcpSocket* Socket = nullptr;


			friend class TcpSocket;
		};

		template<typename AgencyFunc>
		void Lock(AgencyFunc&& Func) requires(std::is_invocable_v<AgencyFunc, Agency&>)
		{
			auto lg = std::lock_guard(SocketMutex);
			Func(Agency(this));
		}

		auto SyncConnect(EndPointT) -> std::optional<std::error_code>;
		auto SyncConnect(std::u8string_view Host, std::u8string_view Service) -> std::optional<std::tuple<std::error_code, EndPointT>>;
		
		auto SyncCancel() -> void;
		auto SyncClose() -> void;
		

	protected:

		template<typename RespondFunc>
		auto AsyncConnect(EndPointT EP, RespondFunc Fun) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code const&, EndPointT, Agency&>)
		{
			if (AbleToConnect())
			{
				Connecting = true;
				TcpSocket::ConnectExecute(PtrT{this}, EP, {}, std::move(Fun));
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncConnect(std::u8string_view Host, std::u8string_view Service, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code const&, EndPointT, Agency&>)
		{
			if (AbleToConnect())
			{
				Connecting = true;
				Resolver.async_resolve(
					std::string_view{ reinterpret_cast<char const*>(Host.data()), Host.size() },
					std::string_view{ reinterpret_cast<char const*>(Service.data()), Service.size() },
					[This = PtrT{ this }, Func = std::move(Func)](std::error_code const& EC, ResolverT RR) mutable {
						assert(This);
						if (!EC && RR.size() >= 1)
						{
							std::vector<EndPointT> EndPoints;
							EndPoints.resize(RR.size() - 1);
							auto Ite = RR.begin();
							auto Top = *Ite;
							for (++Ite; Ite != RR.end(); ++Ite)
								EndPoints.push_back(*Ite);
							std::reverse(EndPoints.begin(), EndPoints.end());
							{
								auto lg = std::lock_guard(This->SocketMutex);
								TcpSocket::ConnectExecute(std::move(This), std::move(Top), std::move(EndPoints), std::move(Func));
							}
						}
						else {
							auto lg = std::lock_guard(This->SocketMutex);
							This->Connecting = false;
							Agency Age{ This.GetPointer() };
							Func(EC, {}, Age);
						}
					}
				);
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncSend(std::span<std::byte const> PersistenceBuffer, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code const&, std::size_t, Agency&>)
		{
			if (AbleToSend())
			{
				Sending = true;
				TcpSocket::SendExecute(0, PtrT{this}, PersistenceBuffer, std::move(Func));
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncReceiveSome(std::span<std::byte> PersistenceBuffer, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code const&, std::size_t, Agency&>)
		{
			if (AbleToReceive())
			{
				Receiving = true;
				Socket.async_read_some(
					asio::mutable_buffer{ PersistenceBuffer.data(), PersistenceBuffer .size()},
					[Func = std::move(Func), This = PtrT{this}](std::error_code const& EC, std::size_t Readed) mutable {
						auto lg = std::lock_guard(This->SocketMutex);
						This->Receiving = false;
						Agency Age{ This.GetPointer() };
						Func(EC, Readed, Age);
					}
				);
				return true;
			}
			return false;
		}

		template<typename RespondFunc>
		auto AsyncReceive(std::span<std::byte> PersistenceBuffer, RespondFunc Func) -> bool
			requires(std::is_invocable_v<RespondFunc, std::error_code const&, std::size_t, Agency&>)
		{
			if (AbleToReceive())
			{
				TcpSocket::ReceiveExecute(0, PersistenceBuffer, PtrT{this}, std::move(Func));
				return true;
			}
			return false;
		}

		auto CloseExecute() -> void;
		auto CancelExecute() -> void;

		bool IsConnecting() const { return Connecting; }
		bool IsSending() const { return Sending; }
		bool IsReceiving() const { return Receiving; }

		bool AbleToConnect() const { return !IsConnecting() && !IsSending() && !IsReceiving(); }
		bool AbleToSend() const { return !IsConnecting() && !IsSending(); }
		bool AbleToReceive() const { return !IsConnecting() && !IsReceiving(); }

		TcpSocket(asio::io_context& Context) : Socket(Context), Resolver(Context) {}

		std::mutex SocketMutex;
		asio::ip::tcp::socket Socket;
		asio::ip::tcp::resolver Resolver;

		bool Connecting = false;
		bool Sending = false;
		bool Receiving = false;

	private:

		template<typename RespondFunction>
		static auto ConnectExecute(PtrT This, EndPointT CurEndPoint, std::vector<EndPointT> EndPoints, RespondFunction Func) -> void
		{
			assert(This);
			TcpSocket* TThis = This.GetPointer();
			TThis->Socket.async_connect(CurEndPoint, [CurEndPoint, This = std::move(This), EndPoints = std::move(EndPoints), Func = std::move(Func)](std::error_code const& EC) mutable {
				if (EC && EC != asio::error::operation_aborted && !EndPoints.empty())
				{
					auto Top = std::move(*EndPoints.rbegin());
					EndPoints.pop_back();
					auto lg = std::lock_guard(This->SocketMutex);
					TcpSocket::ConnectExecute(std::move(This), std::move(Top), std::move(EndPoints), std::move(Func));
				}
				else {
					auto lg = std::lock_guard(This->SocketMutex);
					This->Connecting = false;
					Agency Age{ This.GetPointer() };
					Func(EC, CurEndPoint, Age);
				}
			});
		}

		template<typename RespondFunction>
		static void SendExecute(std::size_t TotalWrite, PtrT This, std::span<std::byte const> Buffer, RespondFunction Func)
		{
			assert(This);
			TcpSocket* TThis = This.GetPointer();
			TThis->Socket.async_write_some(asio::const_buffer{ Buffer.data(), Buffer.size() },
				[This = std::move(This), Buffer, TotalWrite, Func = std::move(Func)](std::error_code const& EC, std::size_t Writed) mutable {
					if (!EC && Writed < Buffer.size())
					{
						auto lg = std::lock_guard(This->SocketMutex);
						TcpSocket::SendExecute(TotalWrite + Writed, std::move(This), Buffer.subspan(Writed), std::move(Func));
					}
					else {
						auto lg = std::lock_guard(This->SocketMutex);
						This->Sending = false;
						Agency Age{ This.GetPointer() };
						Func(EC, TotalWrite + Writed, Age);
					}
				}
			);
		}

		template<typename RespondFunction>
		static void ReceiveExecute(std::size_t TotalSize, std::span<std::byte> IteBuffer, PtrT This, RespondFunction Func)
		{
			assert(This);
			TcpSocket* TThis = This.GetPointer();
			TThis->Socket.async_read_some(asio::mutable_buffer{ IteBuffer.data(), IteBuffer.size() },
				[TotalSize, IteBuffer, This = std::move(This), Func = std::move(Func)](std::error_code const& EC, std::size_t Readed) mutable {
					if (!EC && IteBuffer.size() > Readed)
					{
						auto lg = std::lock_guard(This->SocketMutex);
						TcpSocket::ReceiveExecute(TotalSize + Readed, IteBuffer.subspan(Readed), std::move(This), std::move(Func));
					}
					else {
						auto lg = std::lock_guard(This->SocketMutex);
						This->Receiving = false;
						Agency Age{ This.GetPointer() };
						Func(EC, TotalSize + Readed, Age);
					}
				}
			);
		}

		friend struct IntrusiceObjWrapper;
	};
	*/
}