//---------------------------------------------------------------------------

#ifndef UsbAuthTokenH
#define UsbAuthTokenH

#include <string>

struct UsbAuthToken
{
	std::string clientId;
	std::string clientName;
	std::string flashSerial; // UPPERCASE, trimmed
	std::string tokenPath;
};

// Загрузка и проверка searchclient-auth-token.json (format_version 1).
// actualHardwareSerial — факт с тома (уже нормализованный UPPERCASE).
bool TryLoadUsbAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualHardwareSerial,
	UsbAuthToken& out);

std::string NormalizeFlashSerial(std::string s);

std::string UsbAuthTokenFileName();

#endif
