//---------------------------------------------------------------------------

#pragma hdrstop

#include "UsbAuthToken.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

std::string TrimCopy(std::string s)
{
	auto notSpace = [](unsigned char c) { return !std::isspace(c); };
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
	s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
	return s;
}

} // namespace

std::string NormalizeFlashSerial(std::string s)
{
	s = TrimCopy(std::move(s));
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	return s;
}

std::string UsbAuthTokenFileName()
{
	return "searchclient-auth-token.json";
}

bool TryLoadUsbAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualHardwareSerial,
	UsbAuthToken& out)
{
	out = UsbAuthToken{};
	if (tokenPathUtf8.empty() || actualHardwareSerial.empty())
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
	if (formatVersion != 1 && formatVersion != 2)
		return false;

	const std::string clientName = TrimCopy(j.value("client_name", std::string()));
	const std::string flashSerial = NormalizeFlashSerial(j.value("flash_serial", std::string()));
	if (clientName.empty() || flashSerial.empty())
		return false;

	const std::string actual = NormalizeFlashSerial(actualHardwareSerial);
	if (flashSerial != actual)
		return false;

	std::string signatureValue;
	if (j.contains("signature") && j["signature"].is_object()) {
		const std::string alg = j["signature"].value("alg", std::string());
		if (formatVersion == 1) {
			if (!alg.empty() && alg != "none")
				return false;
		} else {
			// v2: accept RS256 (server verifies). No local crypto check.
			if (alg != "RS256")
				return false;
			signatureValue = j["signature"].value("value", std::string());
			if (signatureValue.empty())
				return false;
		}
	} else if (formatVersion == 2) {
		return false;
	}

	out.clientId = TrimCopy(j.value("client_id", std::string()));
	out.clientName = clientName;
	out.flashSerial = flashSerial;
	out.tokenPath = tokenPathUtf8;
	out.signatureValue = signatureValue;
	out.formatVersion = formatVersion;
	return true;
}
