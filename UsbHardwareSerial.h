//---------------------------------------------------------------------------

#ifndef UsbHardwareSerialH
#define UsbHardwareSerialH

#include <string>
#include <vector>

struct UsbDiskInfo
{
	std::string Serial;
	std::string DriveLetter;   // "E:"
	std::string DeviceID;      // Win32_DiskDrive.DeviceID
	std::string PnpDeviceId;
};

// Hardware/PNP serial USB-дисков через WMI (не volume serial).
class UsbHardwareSerial
{
public:
	static std::string ExtractSerialFromPnp(const std::wstring& pnpDeviceId);
	static std::string ExtractSerialFromPnp(const std::string& pnpDeviceId);

	static std::string ResolveDriveLetterFromDiskDeviceId(const std::string& deviceId);

	// "E:" / "E:\\" / "e" → hardware serial или пустая строка, если диска нет.
	static std::string GetSerialForDriveLetter(const std::string& driveLetter);

	static std::vector<UsbDiskInfo> EnumerateConnectedUsbDisks();
};

#endif
