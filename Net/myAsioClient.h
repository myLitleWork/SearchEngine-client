//---------------------------------------------------------------------------

#ifndef myAsioClientH
#define myAsioClientH

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <boost/asio.hpp>

	enum { max_length = 2048 };
	enum class COMMAND : uint_fast64_t
	{
		SOMEERROR = 0,
		SOLOREQUEST,
		FILETEXT,
		JSONREGUEST,
		// Values 3..9 are historical wire slots without production handlers.
		// Keep the ordinals reserved so every active command retains its number.
		RESERVED_COMMAND_4,
		RESERVED_COMMAND_5,
		RESERVED_COMMAND_6,
		RESERVED_COMMAND_7,
		RESERVED_COMMAND_8,
		RESERVED_COMMAND_9,
		GETSQLJSONANSWEAR,
		GETBINFILE,
		GET_VH_TELEGI_FROM_SQL,
		GET_ISH_TELEGI_FROM_SQL,
		START_UPDATE_BASE,
		LOAD_TLG_TO_SEND,
		// HISTORICAL RESERVED / DISABLED ON CURRENT SERVER / DO NOT USE.
		GET_MESSAGE,
		USER_REGISTRY,
		PING,
		GET_VH_TELEGA_WAY,
		GET_ISH_TELEGA_WAY,
		GET_OPIS_BASE,
		LOAD_RAZN,
		GET_ATTACHMENTS,
		GET_ISH_PDTV,
		GET_TELEGA_ATACHMENTS,
		GET_TELEGA_SINGLE_ATACHMENT,
		SERVER_BUSY_ERROR = 27,
		END_COMMAND = 28,
		// Terminal response-only command. Its payload is
		// search_protocol::ErrorResponseV1.
		ERROR_RESPONSE = 29,
		// Zero-payload request; response payload is ProtocolCapabilitiesV1.
		NEGOTIATE_PROTOCOL_V1 = 30,
		AUTHENTICATE_V1 = 31,
		GET_TELEGA_TEXT = 32,
		GET_ISH_PDTV_TEXT = 33,
		UPLOAD_TLG_TO_SEND_V1 = 34,
		UPLOAD_RAZN_V1 = 35,
		//--------------------
        // HISTORICAL RESERVED composite marker / DISABLED ON CURRENT SERVER.
        // DO NOT USE and DO NOT REUSE this wire value.
        SAVE_MESSAGE_TO = 2781032419,
		//--------------------
	};

	struct header
	{
		uint_fast64_t size{0};
		COMMAND command{0};
	};

namespace search_protocol
{
	inline constexpr std::uint32_t ERROR_RESPONSE_VERSION = 1;
	inline constexpr std::uint32_t PROTOCOL_CAPABILITIES_VERSION = 1;
	inline constexpr std::uint32_t CAPABILITY_TYPED_ERRORS_V1 = 1u << 0;
	inline constexpr std::uint32_t CAPABILITY_CLIENT_AUTH_V1 = 1u << 1;

	// Wire values mirror command_execution::ErrorCode on the server. Keep all
	// numeric values explicit: client and server are independent repositories.
	enum class ErrorCode : std::uint32_t
	{
		PayloadTooLarge = 1,
		InvalidCommand = 2,
		CommandNotRegistered = 3,
		ServerBusy = 4,
		ServerStopping = 5,
		InvalidRequest = 6,
		InvalidJson = 7,
		InvalidBinaryPayload = 8,
		FileNotFound = 9,
		FileMetadataFailed = 10,
		FileOpenFailed = 11,
		FileReadFailed = 12,
		FileWriteFailed = 13,
		DirectoryCreateFailed = 14,
		ConfigurationError = 15,
		DatabaseOpenFailed = 16,
		DatabaseBusy = 17,
		DatabaseQueryFailed = 18,
		DatabaseSchemaFailed = 19,
		SerializationFailed = 20,
		IndexUnavailable = 21,
		IndexDataInconsistent = 22,
		IndexUpdateInProgress = 23,
		IndexUpdateFailed = 24,
		OperatorNotRegistered = 25,
		AttachmentNotFound = 26,
		// Historical reserved wire codes. Keep numbers stable.
		MessageNotFound = 27,
		MessageSaveFailed = 28,
		MessageReadFailed = 29,
		ResponseQueueFull = 30,
		CommandExecutionFailed = 31,
		InternalError = 32,
		AuthFailed = 33,
		AuthClientDisabled = 34,
		AuthRequired = 35,
		AuthClientIdMissing = 36,
		AuthClientNameMissing = 37,
		AuthDeviceTypeMissing = 38,
		AuthDeviceIdMissing = 39,
		AuthSignatureMissing = 40,
		AuthClientIdNotFound = 41,
		AuthClientNameMismatch = 42,
		AuthDeviceTypeMismatch = 43,
		AuthDeviceIdMismatch = 44,
		AuthSignatureInvalid = 45,
		DataSourceDisabled = 46,
		DataSourceUnavailable = 47,
		ArchiveAdminRequired = 48
	};

	struct ErrorResponseV1
	{
		std::uint32_t version{ERROR_RESPONSE_VERSION};
		std::uint32_t errorCode{0};
	};

	// Client-side request result metadata. This structure is not serialized.
	struct ServerErrorInfo
	{
		bool hasCode{false};
		bool isTyped{false};
		std::uint32_t rawCode{0};

		void reset() noexcept
		{
			hasCode = false;
			isTyped = false;
			rawCode = 0;
		}

		void setRaw(std::uint32_t code, bool typed) noexcept
		{
			hasCode = true;
			isTyped = typed;
			rawCode = code;
		}

		void set(ErrorCode code, bool typed) noexcept
		{
			setRaw(static_cast<std::uint32_t>(code), typed);
		}

		[[nodiscard]] bool is(ErrorCode code) const noexcept
		{
			return hasCode && rawCode == static_cast<std::uint32_t>(code);
		}
	};

	struct ProtocolCapabilitiesV1
	{
		std::uint32_t version{PROTOCOL_CAPABILITIES_VERSION};
		std::uint32_t capabilities{0};
	};
}

std::string serverErrorUserMessage(std::uint32_t rawCode);
std::string serverErrorCodeName(std::uint32_t rawCode);
std::string serverErrorCatalogMessage(std::uint32_t rawCode);
bool serverErrorCatalogLoaded();
bool tryExtractServerErrorCode(const std::string& errUtf8, std::uint32_t& code);
std::string legacyServerErrorUserMessage();
std::string busyServerErrorUserMessage();
bool isAuthRejectionErrorCode(std::uint32_t rawCode) noexcept;

static_assert(sizeof(header) == 16, "The wire header must remain 16 bytes");
static_assert(static_cast<std::uint64_t>(COMMAND::SOMEERROR) == 0);
static_assert(static_cast<std::uint64_t>(COMMAND::FILETEXT) == 2);
static_assert(static_cast<std::uint64_t>(COMMAND::JSONREGUEST) == 3);
static_assert(static_cast<std::uint64_t>(COMMAND::RESERVED_COMMAND_4) == 4);
static_assert(static_cast<std::uint64_t>(COMMAND::RESERVED_COMMAND_5) == 5);
static_assert(static_cast<std::uint64_t>(COMMAND::RESERVED_COMMAND_6) == 6);
static_assert(static_cast<std::uint64_t>(COMMAND::RESERVED_COMMAND_7) == 7);
static_assert(static_cast<std::uint64_t>(COMMAND::RESERVED_COMMAND_8) == 8);
static_assert(static_cast<std::uint64_t>(COMMAND::RESERVED_COMMAND_9) == 9);
static_assert(static_cast<std::uint64_t>(COMMAND::GETBINFILE) == 11);
static_assert(static_cast<std::uint64_t>(COMMAND::PING) == 18);
static_assert(static_cast<std::uint64_t>(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) == 26);
static_assert(static_cast<std::uint64_t>(COMMAND::SERVER_BUSY_ERROR) == 27);
static_assert(static_cast<std::uint64_t>(COMMAND::END_COMMAND) == 28);
static_assert(static_cast<std::uint64_t>(COMMAND::ERROR_RESPONSE) == 29);
static_assert(static_cast<std::uint64_t>(COMMAND::NEGOTIATE_PROTOCOL_V1) == 30);
static_assert(static_cast<std::uint64_t>(COMMAND::AUTHENTICATE_V1) == 31);
static_assert(static_cast<std::uint64_t>(COMMAND::LOAD_TLG_TO_SEND) == 15);
static_assert(static_cast<std::uint64_t>(COMMAND::LOAD_RAZN) == 22);
static_assert(static_cast<std::uint64_t>(COMMAND::GET_MESSAGE) == 16);
static_assert(static_cast<std::uint64_t>(COMMAND::SAVE_MESSAGE_TO) == 2781032419ULL);
static_assert(static_cast<std::uint64_t>(COMMAND::GET_TELEGA_TEXT) == 32);
static_assert(static_cast<std::uint64_t>(COMMAND::GET_ISH_PDTV_TEXT) == 33);
static_assert(static_cast<std::uint64_t>(COMMAND::UPLOAD_TLG_TO_SEND_V1) == 34);
static_assert(static_cast<std::uint64_t>(COMMAND::UPLOAD_RAZN_V1) == 35);
static_assert(sizeof(search_protocol::ErrorResponseV1) == 8,
	"ERROR_RESPONSE v1 payload must remain 8 bytes");
static_assert(sizeof(search_protocol::ProtocolCapabilitiesV1) == 8,
	"Protocol capabilities v1 payload must remain 8 bytes");
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthFailed) == 33u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientDisabled) == 34u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthRequired) == 35u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientIdMissing) == 36u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientNameMissing) == 37u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceTypeMissing) == 38u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceIdMissing) == 39u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthSignatureMissing) == 40u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientIdNotFound) == 41u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientNameMismatch) == 42u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceTypeMismatch) == 43u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceIdMismatch) == 44u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthSignatureInvalid) == 45u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::DataSourceDisabled) == 46u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::DataSourceUnavailable) == 47u);
static_assert(static_cast<std::uint32_t>(search_protocol::ErrorCode::ArchiveAdminRequired) == 48u);

using boost::asio::ip::tcp;


class MyAsioClient
{
public:
	using FileProgressCallback = std::function<void(std::uint64_t, std::uint64_t)>;
	enum class ProtocolNegotiationResult
	{
		TypedErrorsV1,
		LegacyOnCurrentConnection,
		LegacyReconnectRequired,
		Failed
	};

private:
    boost::asio::io_context io_context_;
    tcp::resolver* resolver_{nullptr};
	tcp::socket*   socket_{nullptr};

    header myHeader_{};
    std::mutex io_mutex_;                 // защищаем сокет
    std::atomic<uint64_t> last_total_{0}; // последний ожидаемый объём

public:
	void disConnect();
	bool isConnected();

	bool requestBinFile(const std::string& path,
                    std::vector<uint8_t>& out,
                    std::atomic_uint64_t* progress = nullptr,
                    std::string* errMsg = nullptr);

	ProtocolNegotiationResult negotiateProtocol(
		std::string* errMsg = nullptr);

	// A request/response pair is one protocol transaction.  Keeping the
	// io_mutex_ locked across both halves prevents another worker from placing
	// its request between our write and read.
	bool requestText(const std::string& request,
					 COMMAND command,
					 std::string& out,
					 std::atomic_uint64_t* progress = nullptr,
					 std::string* errMsg = nullptr,
					 search_protocol::ServerErrorInfo* serverError = nullptr);

	bool requestBinary(const std::vector<uint8_t>& request,
					   COMMAND command,
					   std::vector<uint8_t>& out,
					   std::atomic_uint64_t* progress = nullptr,
					   std::string* errMsg = nullptr,
					   search_protocol::ServerErrorInfo* serverError = nullptr);

	bool requestFileToPath(const std::string& request,
						  COMMAND command,
						  const std::string& outPath,
						  FileProgressCallback onProgress = nullptr,
						  std::uint64_t* receivedSize = nullptr,
						  std::string* errMsg = nullptr,
						  bool flushStreamToOs = false,
						  bool replaceAfterSuccess = false);

	bool uploadFileV1(const std::wstring& localPath,
					  COMMAND command,
					  std::string* savedName = nullptr,
					  std::string* errMsg = nullptr,
					  search_protocol::ServerErrorInfo* serverError = nullptr);


    // уже есть у тебя:
    bool tryConnect(const std::string& host, const std::string& port, std::string& errMsg);
    bool tryConnect(const AnsiString& host, const AnsiString& port, AnsiString& errMsg);

    // === запись ===
    bool socketWrite(const std::string& s, COMMAND command, std::string* errMsg = nullptr);
    bool socketWrite(const std::vector<uint8_t>& data, COMMAND command, std::string* errMsg = nullptr);

    // === чтение текстом ===
    bool socketRead(std::string& out,
                    std::atomic_uint64_t* progress = nullptr,
                    std::string* errMsg = nullptr);

    // === чтение бинарно ===
    bool socketReadBin(std::vector<uint8_t>& out,
                       std::atomic_uint64_t* progress = nullptr,
                       std::string* errMsg = nullptr);

	uint64_t last_total_bytes() const noexcept { return last_total_.load(std::memory_order_relaxed); }

	bool readHeader(header& hdr, std::string* errMsg = nullptr);

	tcp::socket& getSocket() {
		if (!socket_) {
			throw std::runtime_error("Сокет не инициализирован");
		}
		return *socket_;
	}

	~MyAsioClient() noexcept;  // деструктор
};
//---------------------------------------------------------------------------
#endif
