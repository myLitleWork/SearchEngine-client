//---------------------------------------------------------------------------

#pragma hdrstop

#include "UsbAuthToken.h"
#include "DeviceIdentity.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

//---------------------------------------------------------------------------
#pragma package(smart_init)

std::string NormalizeFlashSerial(std::string s)
{
	return device_identity::NormalizeUsbDeviceId(std::move(s));
}

std::string UsbAuthTokenFileName()
{
	return "searchclient-auth-token.json";
}

bool TryParseAuthToken(const std::string& tokenPathUtf8, UsbAuthToken& out)
{
	out = UsbAuthToken{};
	if (tokenPathUtf8.empty())
		return false;

	std::ifstream in(tokenPathUtf8, std::ios::binary);
	if (!in)
		return false;

	std::ostringstream ss;
	ss << in.rdbuf();
	const std::string raw = ss.str();
	if (raw.empty())
		return false;

	nlohmann::json j;
	try {
		j = nlohmann::json::parse(raw);
	} catch (...) {
		return false;
	}

	if (!j.is_object())
		return false;
	if (j.value("format", std::string()) != "searchclient-auth-token")
		return false;

	const int formatVersion = j.value("format_version", 0);
	if (formatVersion != 1)
		return false;

	const std::string clientId = device_identity::TrimCopy(j.value("client_id", std::string()));
	const std::string clientName = device_identity::TrimCopy(j.value("client_name", std::string()));
	const std::string deviceType = device_identity::TrimCopy(j.value("device_type", std::string()));
	if (clientId.empty() || clientName.empty())
		return false;
	if (!device_identity::IsSupportedDeviceType(deviceType))
		return false;

	std::string deviceId;
	if (deviceType == device_identity::kDeviceTypeUsb) {
		deviceId = device_identity::NormalizeUsbDeviceId(j.value("device_id", std::string()));
		if (deviceId.empty())
			return false;
	} else {
		const auto uuid = device_identity::NormalizeComputerUuid(j.value("device_id", std::string()));
		if (!uuid)
			return false;
		deviceId = *uuid;
	}

	if (!j.contains("signature") || !j["signature"].is_object())
		return false;
	const std::string alg = j["signature"].value("alg", std::string());
	if (alg != "RS256")
		return false;
	if (!j["signature"].contains("encoding") || !j["signature"]["encoding"].is_string())
		return false;
	const std::string encoding = j["signature"]["encoding"].get<std::string>();
	if (encoding.empty() || encoding != "base64")
		return false;
	const std::string signatureValue = j["signature"].value("value", std::string());
	if (signatureValue.empty())
		return false;

	out.clientId = clientId;
	out.clientName = clientName;
	out.deviceType = deviceType;
	out.deviceId = deviceId;
	out.tokenPath = tokenPathUtf8;
	out.signatureValue = signatureValue;
	out.formatVersion = formatVersion;
	return true;
}

bool TryLoadUsbAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualHardwareSerial,
	UsbAuthToken& out)
{
	out = UsbAuthToken{};
	const std::string actual = device_identity::NormalizeUsbDeviceId(actualHardwareSerial);
	if (tokenPathUtf8.empty() || actual.empty())
		return false;
	UsbAuthToken token;
	if (!TryParseAuthToken(tokenPathUtf8, token))
		return false;
	if (token.deviceType != device_identity::kDeviceTypeUsb)
		return false;
	if (token.deviceId != actual)
		return false;
	out = token;
	return true;
}

bool TryLoadComputerAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualComputerUuid,
	UsbAuthToken& out)
{
	out = UsbAuthToken{};
	const auto live = device_identity::NormalizeComputerUuid(actualComputerUuid);
	if (tokenPathUtf8.empty() || !live)
		return false;
	UsbAuthToken token;
	if (!TryParseAuthToken(tokenPathUtf8, token))
		return false;
	if (token.deviceType != device_identity::kDeviceTypeComputer)
		return false;
	if (token.deviceId != *live)
		return false;
	out = token;
	return true;
}
