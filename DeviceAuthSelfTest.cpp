#include <vcl.h>
#pragma hdrstop

#include "DeviceAuthSelfTest.h"

#ifdef _DEBUG

#include "DeviceAuthSelect.h"
#include "DeviceIdentity.h"
#include "UsbAuthToken.h"

#include <nlohmann/json.hpp>

#include <Windows.h>
#include <fstream>
#include <string>

namespace
{
	std::wstring tempDir()
	{
		wchar_t buffer[MAX_PATH] = {};
		const DWORD n = GetTempPathW(MAX_PATH, buffer);
		if (n == 0 || n >= MAX_PATH)
			return {};
		return std::wstring(buffer, n) + L"SearchClientDeviceAuthSelfTest";
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
	const std::wstring programDataDir = root + L"\\ProgramData";
	const std::wstring exeDir = root + L"\\Exe";
	CreateDirectoryW(programDataDir.c_str(), nullptr);
	CreateDirectoryW(exeDir.c_str(), nullptr);
	const std::wstring programDataToken = programDataDir + L"\\searchclient-auth-token.json";
	const std::wstring exeToken = exeDir + L"\\searchclient-auth-token.json";
	const std::wstring usbToken = root + L"\\usb-searchclient-auth-token.json";
	const std::string programDataUtf8 = wideToUtf8(programDataToken);
	const std::string exeUtf8 = wideToUtf8(exeToken);
	const std::string usbUtf8 = wideToUtf8(usbToken);
	const std::string liveUuid = *uuid;

	if (!writeFile(programDataToken, makeToken(
			"computer", liveUuid, "pc-1", "desk", "sig-programdata")))
		return 7;
	auto selected = SelectPcAuthIdentity(programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->clientId != "pc-1" || selected->signature != "sig-programdata")
		return 8;

	if (!writeFile(exeToken, makeToken(
			"computer", liveUuid, "pc-exe", "desk", "sig-exe")))
		return 9;
	DeleteFileW(programDataToken.c_str());
	selected = SelectPcAuthIdentity(programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->clientId != "pc-exe")
		return 10;

	if (!writeFile(programDataToken, "{not-json"))
		return 11;
	selected = SelectPcAuthIdentity(programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->clientId != "pc-exe")
		return 12;

	if (!writeFile(programDataToken, makeToken(
			"usb", "OTHERUSB", "wrong-type", "desk", "sig-wrong")))
		return 13;
	selected = SelectPcAuthIdentity(programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->clientId != "pc-exe")
		return 14;

	if (!writeFile(programDataToken, makeToken(
			"computer", "BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB", "other-pc", "desk", "sig-other")))
		return 15;
	selected = SelectPcAuthIdentity(programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->clientId != "pc-exe")
		return 16;

	if (!writeFile(usbToken, makeToken(
			"usb", "USBSERIAL01", "usb-1", "flash-user", "sig-usb")))
		return 17;
	UsbAuthProbe usb;
	usb.tokenPath = usbUtf8;
	usb.driveLetter = "E:";
	usb.claimedDeviceId = "USBSERIAL01";
	usb.liveHardwareSerial = "USBSERIAL01";
	selected = SelectDeviceAuthIdentity(usb, programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->deviceType != "usb" || selected->deviceId != "USBSERIAL01")
		return 18;

	selected = SelectDeviceAuthIdentity(std::nullopt, programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer" || selected->clientId != "pc-exe")
		return 19;

	usb.liveHardwareSerial.clear();
	selected = SelectDeviceAuthIdentity(usb, programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer" || selected->deviceId != liveUuid)
		return 20;

	usb.liveHardwareSerial = "DIFFERENTUSB";
	selected = SelectDeviceAuthIdentity(usb, programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer")
		return 21;

	if (!writeFile(usbToken, makeToken(
			"usb", "FOREIGNUSB", "usb-foreign", "flash-user", "sig-foreign")))
		return 22;
	usb.claimedDeviceId = "USBSERIAL01";
	usb.liveHardwareSerial = "USBSERIAL01";
	selected = SelectDeviceAuthIdentity(usb, programDataUtf8, exeUtf8, liveUuid);
	if (!selected || selected->deviceType != "computer")
		return 23;

	UsbAuthToken parsed;
	if (TryParseAuthToken(usbUtf8, parsed) && parsed.deviceId == "USBSERIAL01")
		return 24;

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
		return 25;

	DeviceAuthIdentity pcReq = sample("pc-exe", liveUuid, "sig-exe");
	const auto pcJson = nlohmann::json::parse(BuildAuthenticateV1Json(pcReq));
	if (pcJson.contains("flash_serial") ||
		pcJson.at("device_type") != "computer" ||
		pcJson.at("device_id") != liveUuid)
		return 26;

	const auto previousPc = sample("pc-old", liveUuid, "sig-old");
	const auto nextPc = sample("pc-new", liveUuid, "sig-new");
	if (!ShouldInvalidateServerAuth(previousPc, nextPc))
		return 27;
	if (ShouldInvalidateServerAuth(nextPc, nextPc))
		return 28;

	const auto previousUsb = sample("usb-1", "USBSERIAL01", "sig-usb", "usb");
	if (!ShouldInvalidateServerAuth(previousUsb, pcReq))
		return 29;
	if (!ShouldInvalidateServerAuth(previousPc, std::nullopt))
		return 30;

	auto changedId = previousPc;
	changedId.deviceId = "CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC";
	if (!ShouldInvalidateServerAuth(previousPc, changedId))
		return 31;
	auto changedSig = previousPc;
	changedSig.signature = "sig-rotated";
	if (!ShouldInvalidateServerAuth(previousPc, changedSig))
		return 32;

	DeleteFileW(programDataToken.c_str());
	DeleteFileW(exeToken.c_str());
	DeleteFileW(usbToken.c_str());
	RemoveDirectoryW(programDataDir.c_str());
	RemoveDirectoryW(exeDir.c_str());
	RemoveDirectoryW(root.c_str());
	return 0;
}

#endif
