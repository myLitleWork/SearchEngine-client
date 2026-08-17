#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace device_identity
{
	inline const char* kDeviceTypeUsb = "usb";
	inline const char* kDeviceTypeComputer = "computer";

	inline std::string TrimCopy(std::string value)
	{
		auto notSpace = [](unsigned char c) { return !std::isspace(c); };
		value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
		value.erase(
			std::find_if(value.rbegin(), value.rend(), notSpace).base(),
			value.end());
		return value;
	}

	inline bool IsSupportedDeviceType(const std::string& deviceType)
	{
		return deviceType == kDeviceTypeUsb || deviceType == kDeviceTypeComputer;
	}

	inline std::string NormalizeUsbDeviceId(std::string value)
	{
		value = TrimCopy(std::move(value));
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
			return static_cast<char>(std::toupper(c));
		});
		return value;
	}

	// Canonical SMBIOS UUID: 8-4-4-4-12 uppercase hex. Accepts braces and optional
	// hyphens. Rejects empty, all-zero, and all-F placeholders.
	inline std::optional<std::string> NormalizeComputerUuid(std::string value)
	{
		value = TrimCopy(std::move(value));
		if (value.size() >= 2 && value.front() == '{' && value.back() == '}') {
			value = value.substr(1, value.size() - 2);
			value = TrimCopy(std::move(value));
		}

		std::string hex;
		hex.reserve(32);
		for (std::size_t i = 0; i < value.size(); ++i) {
			const unsigned char c = static_cast<unsigned char>(value[i]);
			if (c == '-')
				continue;
			if (!std::isxdigit(c))
				return std::nullopt;
			hex.push_back(static_cast<char>(std::toupper(c)));
		}
		if (hex.size() != 32)
			return std::nullopt;
		if (hex == std::string(32, '0') || hex == std::string(32, 'F'))
			return std::nullopt;

		std::string canonical;
		canonical.reserve(36);
		canonical.append(hex, 0, 8);
		canonical.push_back('-');
		canonical.append(hex, 8, 4);
		canonical.push_back('-');
		canonical.append(hex, 12, 4);
		canonical.push_back('-');
		canonical.append(hex, 16, 4);
		canonical.push_back('-');
		canonical.append(hex, 20, 12);
		return canonical;
	}
}
