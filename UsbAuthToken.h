//---------------------------------------------------------------------------

#ifndef UsbAuthTokenH
#define UsbAuthTokenH

#include <string>

struct UsbAuthToken
{
	std::string clientId;
	std::string clientName;
	std::string deviceType;
	std::string deviceId;
	std::string tokenPath;
	std::string signatureValue;
	int formatVersion = 0;
};

// Unified searchclient-auth-token.json (format_version 1, device_type/device_id).
// Client does not verify RSA; it only parses fields and matches hardware id.
bool TryParseAuthToken(const std::string& tokenPathUtf8, UsbAuthToken& out);

bool TryLoadUsbAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualHardwareSerial,
	UsbAuthToken& out);

bool TryLoadComputerAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualComputerUuid,
	UsbAuthToken& out);

std::string NormalizeFlashSerial(std::string s);

std::string UsbAuthTokenFileName();

#endif
