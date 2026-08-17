#pragma once

#include <optional>
#include <string>

struct DeviceAuthIdentity
{
	std::string clientId;
	std::string clientName;
	std::string deviceType;
	std::string deviceId;
	std::string signature;
	std::string tokenPath;
};

struct UsbAuthProbe
{
	std::string tokenPath;
	std::string driveLetter;
	std::string claimedDeviceId;
	std::string liveHardwareSerial;
};

bool AuthTupleEquals(const DeviceAuthIdentity& a, const DeviceAuthIdentity& b);

bool ShouldInvalidateServerAuth(
	const DeviceAuthIdentity& previous,
	const std::optional<DeviceAuthIdentity>& next);

std::string BuildAuthenticateV1Json(const DeviceAuthIdentity& identity);

std::optional<DeviceAuthIdentity> TryUsbIdentityFromProbe(const UsbAuthProbe& probe);

std::optional<DeviceAuthIdentity> SelectPcAuthIdentity(
	const std::string& programDataTokenPathUtf8,
	const std::string& exeDirTokenPathUtf8,
	const std::string& liveComputerUuid);

std::optional<DeviceAuthIdentity> SelectDeviceAuthIdentity(
	const std::optional<UsbAuthProbe>& usbProbe,
	const std::string& programDataTokenPathUtf8,
	const std::string& exeDirTokenPathUtf8,
	const std::string& liveComputerUuid);
