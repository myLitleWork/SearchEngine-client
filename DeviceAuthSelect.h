#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

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

struct UsbAuthSelection
{
	std::optional<DeviceAuthIdentity> identity;
	std::size_t winningProbeIndex = static_cast<std::size_t>(-1);
};

UsbAuthSelection SelectUsbAuthIdentityFromProbes(
	const std::vector<UsbAuthProbe>& probes);

std::optional<DeviceAuthIdentity> SelectPcAuthIdentity(
	const std::string& programDataTokenPathUtf8,
	const std::string& exeDirTokenPathUtf8,
	const std::string& liveComputerUuid);

std::optional<DeviceAuthIdentity> SelectDeviceAuthIdentity(
	const std::vector<UsbAuthProbe>& usbProbes,
	const std::string& programDataTokenPathUtf8,
	const std::string& exeDirTokenPathUtf8,
	const std::string& liveComputerUuid);
