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
	std::string signatureValue; // base64; empty for legacy v1/none
	int formatVersion = 0;
};

// Загрузка searchclient-auth-token.json (format_version 1 или 2).
// Криптоподпись на клиенте НЕ проверяется — только serial + поля.
// actualHardwareSerial — факт с тома (уже нормализованный UPPERCASE).
bool TryLoadUsbAuthToken(const std::string& tokenPathUtf8,
	const std::string& actualHardwareSerial,
	UsbAuthToken& out);

std::string NormalizeFlashSerial(std::string s);

std::string UsbAuthTokenFileName();

#endif
