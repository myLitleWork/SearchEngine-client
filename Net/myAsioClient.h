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
		// Values 4..9 belonged to removed client features. Keep the wire slots
		// reserved so every active command retains its protocol number.
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
		//--------------------
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
		MessageNotFound = 27,
		MessageSaveFailed = 28,
		MessageReadFailed = 29,
		ResponseQueueFull = 30,
		CommandExecutionFailed = 31,
		InternalError = 32
	};

	struct ErrorResponseV1
	{
		std::uint32_t version{ERROR_RESPONSE_VERSION};
		std::uint32_t errorCode{0};
	};

	struct ProtocolCapabilitiesV1
	{
		std::uint32_t version{PROTOCOL_CAPABILITIES_VERSION};
		std::uint32_t capabilities{0};
	};
}

static_assert(sizeof(header) == 16, "The wire header must remain 16 bytes");
static_assert(static_cast<std::uint64_t>(COMMAND::SOMEERROR) == 0);
static_assert(static_cast<std::uint64_t>(COMMAND::PING) == 18);
static_assert(static_cast<std::uint64_t>(COMMAND::SERVER_BUSY_ERROR) == 27);
static_assert(static_cast<std::uint64_t>(COMMAND::END_COMMAND) == 28);
static_assert(static_cast<std::uint64_t>(COMMAND::ERROR_RESPONSE) == 29);
static_assert(static_cast<std::uint64_t>(COMMAND::NEGOTIATE_PROTOCOL_V1) == 30);
static_assert(sizeof(search_protocol::ErrorResponseV1) == 8,
	"ERROR_RESPONSE v1 payload must remain 8 bytes");
static_assert(sizeof(search_protocol::ProtocolCapabilitiesV1) == 8,
	"Protocol capabilities v1 payload must remain 8 bytes");

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
					 std::string* errMsg = nullptr);

	bool requestBinary(const std::vector<uint8_t>& request,
					   COMMAND command,
					   std::vector<uint8_t>& out,
					   std::atomic_uint64_t* progress = nullptr,
					   std::string* errMsg = nullptr);

	bool requestFileToPath(const std::string& request,
						  COMMAND command,
						  const std::string& outPath,
						  FileProgressCallback onProgress = nullptr,
						  std::uint64_t* receivedSize = nullptr,
						  std::string* errMsg = nullptr,
						  bool flushStreamToOs = false);


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
			throw std::runtime_error("Socket is not initialized");
		}
		return *socket_;
	}

	~MyAsioClient() noexcept;  // деструктор
};
//---------------------------------------------------------------------------
#endif
