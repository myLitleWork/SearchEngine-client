#include <vcl.h>
#pragma hdrstop

#include "DeviceAuthSelfTest.h"

#ifdef _DEBUG

#include "AuthPaths.h"
#include "DeviceAuthSelect.h"
#include "DeviceIdentity.h"
#include "UsbAuthToken.h"
#include "UsbTokenRetrySelfTest.h"

#include <nlohmann/json.hpp>

#include <Windows.h>
#include <fstream>
#include <string>
#include <vector>

namespace
{
	class ScopedLocalAppData
	{
		std::wstring previous_;
		bool present_ = false;
	public:
		explicit ScopedLocalAppData(const std::wstring& value)
		{
			std::vector<wchar_t> buffer(32768, L'\0');
			const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer.data(),
				static_cast<DWORD>(buffer.size()));
			present_ = n > 0 && n < buffer.size();
			if (present_)
				previous_.assign(buffer.data(), n);
			SetEnvironmentVariableW(L"LOCALAPPDATA", value.c_str());
		}
		~ScopedLocalAppData()
		{
			SetEnvironmentVariableW(L"LOCALAPPDATA", present_ ? previous_.c_str() : nullptr);
		}
	};

	std::wstring tempDir()
	{
		wchar_t buffer[MAX_PATH] = {};
		const DWORD n = GetTempPathW(MAX_PATH, buffer);
		if (n == 0 || n >= MAX_PATH)
			return {};
		return std::wstring(buffer, n) + L"SearchClientDeviceAuthSelfTest-" +
			std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount());
	}

	std::string wideToUtf8(const std::wstring& value)
	{
		if (value.empty())
			return {};
		const int length = WideCharToMultiByte(
			CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
			nullptr, 0, nullptr, nullptr);
		if (length <= 0)
			return {};
		std::string result(static_cast<std::size_t>(length), '\0');
		WideCharToMultiByte(
			CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
			&result[0], length, nullptr, nullptr);
		return result;
	}

	bool writeFile(const std::wstring& path, const std::string& body)
	{
		CreateDirectoryW(tempDir().c_str(), nullptr);
		std::ofstream out(wideToUtf8(path), std::ios::binary | std::ios::trunc);
		if (!out)
			return false;
		out << body;
		return static_cast<bool>(out);
	}

	std::string makeToken(
		const std::string& deviceType,
		const std::string& deviceId,
		const std::string& clientId,
		const std::string& clientName,
		const std::string& signature)
	{
		nlohmann::json token{
			{"format", "searchclient-auth-token"},
			{"format_version", 1},
			{"client_id", clientId},
			{"client_name", clientName},
			{"device_type", deviceType},
			{"device_id", deviceId},
			{"signature", {
				{"alg", "RS256"},
				{"encoding", "base64"},
				{"value", signature}}}};
		return token.dump();
	}

	std::string makeTokenWithSignature(
		const std::string& deviceType,
		const std::string& deviceId,
		const std::string& clientId,
		const std::string& clientName,
		const nlohmann::json& signature)
	{
		nlohmann::json token{
			{"format", "searchclient-auth-token"},
			{"format_version", 1},
			{"client_id", clientId},
			{"client_name", clientName},
			{"device_type", deviceType},
			{"device_id", deviceId},
			{"signature", signature}};
		return token.dump();
	}

	DeviceAuthIdentity sample(
		const std::string& clientId,
		const std::string& deviceId,
		const std::string& signature,
		const std::string& deviceType = "computer")
	{
		DeviceAuthIdentity identity;
		identity.clientId = clientId;
		identity.clientName = "desk";
		identity.deviceType = deviceType;
		identity.deviceId = deviceId;
		identity.signature = signature;
		identity.tokenPath = "token.json";
		return identity;
	}
}

bool IsDeviceAuthSelfTestInvocation()
{
	return ParamCount() >= 1 && SameText(ParamStr(1), L"--device-auth-self-test");
}

int RunDeviceAuthSelfTestFromCommandLine()
{
	if (const int retryResult = RunUsbTokenRetrySelfTest())
		return retryResult;
	using device_identity::NormalizeComputerUuid;
	using device_identity::NormalizeUsbDeviceId;

	const auto uuid = NormalizeComputerUuid(" {aabbccdd-eeff-1122-3344-556677889900} ");
	if (!uuid || *uuid != "AABBCCDD-EEFF-1122-3344-556677889900")
		return 1;
	if (NormalizeComputerUuid("aabbccddeeff11223344556677889900") != uuid)
		return 2;
	if (NormalizeComputerUuid("00000000-0000-0000-0000-000000000000"))
		return 3;
	if (NormalizeComputerUuid("FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"))
		return 4;
	if (NormalizeComputerUuid(""))
		return 5;
	if (NormalizeUsbDeviceId("  abc123  ") != "ABC123")
		return 6;

	const std::wstring root = tempDir();
	CreateDirectoryW(root.c_str(), nullptr);
	const std::wstring userRoot = root + L"\\UserA";
	CreateDirectoryW(userRoot.c_str(), nullptr);
	ScopedLocalAppData userEnvironment(userRoot);
	const std::wstring userDir = auth_paths::localAppDataSearchEngineDirectory();
	const std::wstring exeDir = root + L"\\Exe";
	CreateDirectoryW(userDir.c_str(), nullptr);
	CreateDirectoryW(exeDir.c_str(), nullptr);
	const std::wstring userToken = auth_paths::localAppDataAuthTokenPath();
	const std::wstring exeToken = exeDir + L"\\searchclient-auth-token.json";
	const std::wstring usbToken = root + L"\\usb-searchclient-auth-token.json";
	const std::wstring usbTokenA = root + L"\\usb-a-searchclient-auth-token.json";
	const std::wstring usbTokenB = root + L"\\usb-b-searchclient-auth-token.json";
	const std::wstring encodingToken = root + L"\\encoding-searchclient-auth-token.json";
	const std::string userUtf8 = wideToUtf8(userToken);
	const std::string usbUtf8 = wideToUtf8(usbToken);
	const std::string usbAUtf8 = wideToUtf8(usbTokenA);
	const std::string usbBUtf8 = wideToUtf8(usbTokenB);
	const std::string encodingUtf8 = wideToUtf8(encodingToken);
	const std::string liveUuid = *uuid;

	if (!writeFile(userToken, makeToken(
			"computer", liveUuid, "pc-1", "desk", "sig-user")))
		return 7;
	auto selected = SelectPcAuthIdentity(userUtf8, liveUuid);
	if (!selected || selected->clientId != "pc-1" || selected->signature != "sig-user")
		return 8;

	if (!writeFile(exeToken, makeToken(
			"computer", liveUuid, "pc-exe", "desk", "sig-exe")))
		return 9;
	DeleteFileW(userToken.c_str());
	selected = SelectPcAuthIdentity(userUtf8, liveUuid);
	if (selected)
		return 10;

	if (!writeFile(userToken, "{not-json"))
		return 11;
	selected = SelectPcAuthIdentity(userUtf8, liveUuid);
	if (selected)
		return 12;

	if (!writeFile(userToken, makeToken(
			"usb", "OTHERUSB", "wrong-type", "desk", "sig-wrong")))
		return 13;
	selected = SelectPcAuthIdentity(userUtf8, liveUuid);
	if (selected)
		return 14;

	if (!writeFile(userToken, makeToken(
			"computer", "BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB", "other-pc", "desk", "sig-other")))
		return 15;
	selected = SelectPcAuthIdentity(userUtf8, liveUuid);
	if (selected)
		return 16;

	// The shared EXE token above must not rescue an invalid/missing user token.
	if (!writeFile(userToken, makeToken(
			"computer", liveUuid, "pc-1", "desk", "sig-user")))
		return 46;
	const std::wstring secondRoot = root + L"\\UserB";
	CreateDirectoryW(secondRoot.c_str(), nullptr);
	SetEnvironmentVariableW(L"LOCALAPPDATA", secondRoot.c_str());
	const auto secondDir = auth_paths::localAppDataSearchEngineDirectory();
	const auto secondToken = auth_paths::localAppDataAuthTokenPath();
	if (secondToken == userToken ||
		SelectPcAuthIdentity(auth_paths::toUtf8(secondToken), liveUuid))
		return 47;
	CreateDirectoryW(secondDir.c_str(), nullptr);
	if (!writeFile(secondToken, makeToken(
			"computer", liveUuid, "pc-user-b", "second-user", "sig-user-b")))
		return 48;
	selected = SelectPcAuthIdentity(auth_paths::toUtf8(auth_paths::localAppDataAuthTokenPath()), liveUuid);
	if (!selected || selected->clientId != "pc-user-b")
		return 49;
	SetEnvironmentVariableW(L"LOCALAPPDATA", nullptr);
	if (!auth_paths::localAppDataAuthTokenPath().empty() ||
		SelectPcAuthIdentity(auth_paths::toUtf8(auth_paths::localAppDataAuthTokenPath()), liveUuid))
		return 50;
	SetEnvironmentVariableW(L"LOCALAPPDATA", L"relative-profile");
	if (!auth_paths::localAppDataAuthTokenPath().empty())
		return 51;
	SetEnvironmentVariableW(L"LOCALAPPDATA", userRoot.c_str());
	selected = SelectPcAuthIdentity(auth_paths::toUtf8(auth_paths::localAppDataAuthTokenPath()), liveUuid);
	if (!selected || selected->clientId != "pc-1")
		return 52;
	DeleteFileW(secondToken.c_str());
	RemoveDirectoryW(secondDir.c_str());
	RemoveDirectoryW(secondRoot.c_str());

	if (!writeFile(usbToken, makeToken(
			"usb", "USBSERIAL01", "usb-1", "flash-user", "sig-usb")))
		return 17;
	UsbAuthProbe usb;
	usb.tokenPath = usbUtf8;
	usb.driveLetter = "E:";
	usb.claimedDeviceId = "USBSERIAL01";
	usb.liveHardwareSerial = "USBSERIAL01";
	selected = SelectDeviceAuthIdentity({usb}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "usb" || selected->deviceId != "USBSERIAL01")
		return 18;

	selected = SelectDeviceAuthIdentity({}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer" || selected->clientId != "pc-1")
		return 19;

	usb.liveHardwareSerial.clear();
	selected = SelectDeviceAuthIdentity({usb}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer" || selected->deviceId != liveUuid)
		return 20;

	usb.liveHardwareSerial = "DIFFERENTUSB";
	selected = SelectDeviceAuthIdentity({usb}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer")
		return 21;

	if (!writeFile(usbToken, makeToken(
			"usb", "FOREIGNUSB", "usb-foreign", "flash-user", "sig-foreign")))
		return 22;
	usb.claimedDeviceId = "USBSERIAL01";
	usb.liveHardwareSerial = "USBSERIAL01";
	selected = SelectDeviceAuthIdentity({usb}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer")
		return 23;

	if (!writeFile(usbToken, makeToken(
			"usb", "USBSERIAL01", "usb-1", "flash-user", "sig-usb")))
		return 24;
	UsbAuthToken parsed;
	if (!TryParseAuthToken(usbUtf8, parsed) || parsed.deviceId != "USBSERIAL01")
		return 25;

	if (!writeFile(encodingToken, makeTokenWithSignature(
			"usb", "USBSERIAL01", "usb-enc", "flash-user",
			nlohmann::json{{"alg", "RS256"}, {"value", "sig-no-encoding"}})))
		return 33;
	if (TryParseAuthToken(encodingUtf8, parsed))
		return 34;

	if (!writeFile(encodingToken, makeTokenWithSignature(
			"usb", "USBSERIAL01", "usb-enc", "flash-user",
			nlohmann::json{{"alg", "RS256"}, {"encoding", ""}, {"value", "sig-empty"}})))
		return 35;
	if (TryParseAuthToken(encodingUtf8, parsed))
		return 36;

	if (!writeFile(encodingToken, makeTokenWithSignature(
			"usb", "USBSERIAL01", "usb-enc", "flash-user",
			nlohmann::json{{"alg", "RS256"}, {"encoding", "hex"}, {"value", "sig-wrong"}})))
		return 37;
	if (TryParseAuthToken(encodingUtf8, parsed))
		return 38;

	if (!writeFile(usbTokenA, makeToken(
			"usb", "USBSERIAL01", "usb-a", "flash-a", "sig-usb-a")))
		return 39;
	if (!writeFile(usbTokenB, makeToken(
			"usb", "USBSERIAL02", "usb-b", "flash-b", "sig-usb-b")))
		return 40;
	UsbAuthProbe usbB;
	usbB.tokenPath = usbBUtf8;
	usbB.driveLetter = "F:";
	usbB.claimedDeviceId = "USBSERIAL02";
	usbB.liveHardwareSerial.clear();
	UsbAuthProbe usbA;
	usbA.tokenPath = usbAUtf8;
	usbA.driveLetter = "E:";
	usbA.claimedDeviceId = "USBSERIAL01";
	usbA.liveHardwareSerial = "USBSERIAL01";
	selected = SelectDeviceAuthIdentity({usbB, usbA}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "usb" || selected->clientId != "usb-a")
		return 41;
	const auto multiUsb = SelectUsbAuthIdentityFromProbes({usbB, usbA});
	if (!multiUsb.identity || multiUsb.winningProbeIndex != 1)
		return 42;

	usbA.liveHardwareSerial.clear();
	selected = SelectDeviceAuthIdentity({usbB, usbA}, userUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer" || selected->clientId != "pc-1")
		return 43;
	const auto allStaleUsb = SelectUsbAuthIdentityFromProbes({usbB, usbA});
	if (allStaleUsb.identity || allStaleUsb.winningProbeIndex != static_cast<std::size_t>(-1))
		return 44;

	DeviceAuthIdentity usbReq;
	usbReq.clientId = "usb-1";
	usbReq.clientName = "flash-user";
	usbReq.deviceType = "usb";
	usbReq.deviceId = "USBSERIAL01";
	usbReq.signature = "sig-usb";
	const auto usbJson = nlohmann::json::parse(BuildAuthenticateV1Json(usbReq));
	if (!usbJson.contains("device_type") || !usbJson.contains("device_id") ||
		usbJson.contains("flash_serial") ||
		usbJson.at("device_type") != "usb" ||
		usbJson.at("device_id") != "USBSERIAL01")
		return 26;

	DeviceAuthIdentity pcReq = sample("pc-exe", liveUuid, "sig-exe");
	const auto pcJson = nlohmann::json::parse(BuildAuthenticateV1Json(pcReq));
	if (pcJson.contains("flash_serial") ||
		pcJson.at("device_type") != "computer" ||
		pcJson.at("device_id") != liveUuid)
		return 27;

	const auto previousPc = sample("pc-old", liveUuid, "sig-old");
	const auto nextPc = sample("pc-new", liveUuid, "sig-new");
	if (!ShouldInvalidateServerAuth(previousPc, nextPc))
		return 28;
	if (ShouldInvalidateServerAuth(nextPc, nextPc))
		return 29;

	const auto previousUsb = sample("usb-1", "USBSERIAL01", "sig-usb", "usb");
	if (!ShouldInvalidateServerAuth(previousUsb, pcReq))
		return 30;
	if (!ShouldInvalidateServerAuth(previousPc, std::nullopt))
		return 31;

	auto changedId = previousPc;
	changedId.deviceId = "CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC";
	if (!ShouldInvalidateServerAuth(previousPc, changedId))
		return 32;
	auto changedSig = previousPc;
	changedSig.signature = "sig-rotated";
	if (!ShouldInvalidateServerAuth(previousPc, changedSig))
		return 45;

	DeleteFileW(userToken.c_str());
	DeleteFileW(exeToken.c_str());
	DeleteFileW(usbToken.c_str());
	DeleteFileW(usbTokenA.c_str());
	DeleteFileW(usbTokenB.c_str());
	DeleteFileW(encodingToken.c_str());
	RemoveDirectoryW(userDir.c_str());
	RemoveDirectoryW(userRoot.c_str());
	RemoveDirectoryW(exeDir.c_str());
	RemoveDirectoryW(root.c_str());
	return 0;
}

#endif
