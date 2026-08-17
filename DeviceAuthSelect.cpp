//---------------------------------------------------------------------------

#pragma hdrstop

#include "DeviceAuthSelect.h"
#include "DeviceIdentity.h"
#include "UsbAuthToken.h"

#include <nlohmann/json.hpp>

//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

DeviceAuthIdentity FromToken(const UsbAuthToken& token)
{
	DeviceAuthIdentity identity;
	identity.clientId = token.clientId;
	identity.clientName = token.clientName;
	identity.deviceType = token.deviceType;
	identity.deviceId = token.deviceId;
	identity.signature = token.signatureValue;
	identity.tokenPath = token.tokenPath;
	return identity;
}

} // namespace

bool AuthTupleEquals(const DeviceAuthIdentity& a, const DeviceAuthIdentity& b)
{
	return a.clientId == b.clientId &&
		a.clientName == b.clientName &&
		a.deviceType == b.deviceType &&
		a.deviceId == b.deviceId &&
		a.signature == b.signature;
}

bool ShouldInvalidateServerAuth(
	const DeviceAuthIdentity& previous,
	const std::optional<DeviceAuthIdentity>& next)
{
	if (!next)
		return true;
	return !AuthTupleEquals(previous, *next);
}

std::string BuildAuthenticateV1Json(const DeviceAuthIdentity& identity)
{
	nlohmann::json body{
		{"client_id", identity.clientId},
		{"client_name", identity.clientName},
		{"device_type", identity.deviceType},
		{"device_id", identity.deviceId},
		{"signature", identity.signature}};
	return body.dump();
}

std::optional<DeviceAuthIdentity> TryUsbIdentityFromProbe(const UsbAuthProbe& probe)
{
	const std::string live = device_identity::NormalizeUsbDeviceId(probe.liveHardwareSerial);
	if (probe.tokenPath.empty() || live.empty())
		return std::nullopt;

	const std::string claimed = device_identity::NormalizeUsbDeviceId(probe.claimedDeviceId);
	if (!claimed.empty() && claimed != live)
		return std::nullopt;

	UsbAuthToken token;
	if (!TryLoadUsbAuthToken(probe.tokenPath, live, token))
		return std::nullopt;
	return FromToken(token);
}

std::optional<DeviceAuthIdentity> SelectPcAuthIdentity(
	const std::string& programDataTokenPathUtf8,
	const std::string& exeDirTokenPathUtf8,
	const std::string& liveComputerUuid)
{
	const auto live = device_identity::NormalizeComputerUuid(liveComputerUuid);
	if (!live)
		return std::nullopt;

	const std::string candidates[] = {
		programDataTokenPathUtf8,
		exeDirTokenPathUtf8
	};
	for (std::size_t i = 0; i < 2; ++i) {
		if (candidates[i].empty())
			continue;
		UsbAuthToken token;
		if (TryLoadComputerAuthToken(candidates[i], *live, token))
			return FromToken(token);
	}
	return std::nullopt;
}

UsbAuthSelection SelectUsbAuthIdentityFromProbes(
	const std::vector<UsbAuthProbe>& probes)
{
	UsbAuthSelection result;
	for (std::size_t i = 0; i < probes.size(); ++i) {
		if (const auto usb = TryUsbIdentityFromProbe(probes[i])) {
			result.identity = usb;
			result.winningProbeIndex = i;
			return result;
		}
	}
	return result;
}

std::optional<DeviceAuthIdentity> SelectDeviceAuthIdentity(
	const std::vector<UsbAuthProbe>& usbProbes,
	const std::string& programDataTokenPathUtf8,
	const std::string& exeDirTokenPathUtf8,
	const std::string& liveComputerUuid)
{
	const UsbAuthSelection usbSelection = SelectUsbAuthIdentityFromProbes(usbProbes);
	if (usbSelection.identity)
		return usbSelection.identity;
	return SelectPcAuthIdentity(
		programDataTokenPathUtf8, exeDirTokenPathUtf8, liveComputerUuid);
}
