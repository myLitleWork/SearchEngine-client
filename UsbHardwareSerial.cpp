//---------------------------------------------------------------------------

#pragma hdrstop

#include "UsbHardwareSerial.h"

#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemidl.h>

#include <algorithm>
#include <cctype>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")

//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

class ComInitializer
{
public:
	ComInitializer()
		: hr_(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
		, owns_(hr_ == S_OK || hr_ == S_FALSE)
	{
		if (hr_ == RPC_E_CHANGED_MODE)
		{
			hr_ = S_OK;
			owns_ = false;
		}
	}

	~ComInitializer()
	{
		if (owns_)
			CoUninitialize();
	}

	bool ok() const
	{
		return SUCCEEDED(hr_);
	}

private:
	HRESULT hr_;
	bool owns_;
};

class Bstr
{
public:
	explicit Bstr(const wchar_t* s)
		: bstr_(s ? SysAllocString(s) : nullptr)
	{
	}

	explicit Bstr(const std::wstring& s)
		: bstr_(SysAllocStringLen(s.c_str(), static_cast<UINT>(s.size())))
	{
	}

	~Bstr()
	{
		if (bstr_)
			SysFreeString(bstr_);
	}

	BSTR get() const { return bstr_; }
	operator BSTR() const { return bstr_; }

private:
	Bstr(const Bstr&);
	Bstr& operator=(const Bstr&);
	BSTR bstr_;
};

std::wstring Utf8ToWide(const std::string& s)
{
	if (s.empty())
		return std::wstring();
	const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
	if (n <= 0)
		return std::wstring(s.begin(), s.end());
	std::wstring out(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &out[0], n);
	return out;
}

std::string WideToUtf8(const std::wstring& ws)
{
	if (ws.empty())
		return std::string();
	const int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()),
		nullptr, 0, nullptr, nullptr);
	if (n <= 0)
		return std::string();
	std::string out(static_cast<size_t>(n), '\0');
	WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()),
		&out[0], n, nullptr, nullptr);
	return out;
}

std::string BstrToUtf8(BSTR bstr)
{
	if (!bstr)
		return std::string();
	return WideToUtf8(std::wstring(bstr, SysStringLen(bstr)));
}

std::string NormalizeDriveLetter(std::string drive)
{
	if (drive.empty())
		return std::string();

	while (!drive.empty() && (drive.back() == '\\' || drive.back() == '/'))
		drive.pop_back();

	if (drive.size() == 1)
		drive += ':';

	if (drive.size() >= 2 && drive[1] == ':')
	{
		drive[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(drive[0])));
		return drive.substr(0, 2);
	}

	return std::string();
}

bool ConnectWmi(IWbemLocator** outLoc, IWbemServices** outSvc)
{
	*outLoc = nullptr;
	*outSvc = nullptr;

	IWbemLocator* pLoc = nullptr;
	if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, reinterpret_cast<void**>(&pLoc))) || !pLoc)
		return false;

	IWbemServices* pSvc = nullptr;
	Bstr ns(L"ROOT\\CIMV2");
	const HRESULT hr = pLoc->ConnectServer(ns.get(), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
	if (FAILED(hr) || !pSvc)
	{
		pLoc->Release();
		return false;
	}

	if (FAILED(CoSetProxyBlanket(
			pSvc,
			RPC_C_AUTHN_WINNT,
			RPC_C_AUTHZ_NONE,
			nullptr,
			RPC_C_AUTHN_LEVEL_CALL,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr,
			EOAC_NONE)))
	{
		pSvc->Release();
		pLoc->Release();
		return false;
	}

	*outLoc = pLoc;
	*outSvc = pSvc;
	return true;
}

std::string FindDriveLetter(IWbemServices* pSvc, const std::string& deviceId)
{
	std::string letter;
	if (!pSvc || deviceId.empty())
		return letter;

	IEnumWbemClassObject* pEnum = nullptr;
	const std::wstring q =
		L"ASSOCIATORS OF {Win32_DiskDrive.DeviceID='" + Utf8ToWide(deviceId) +
		L"'} WHERE AssocClass = Win32_DiskDriveToDiskPartition";

	Bstr lang(L"WQL");
	Bstr query(q);
	if (FAILED(pSvc->ExecQuery(
			lang.get(),
			query.get(),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			nullptr,
			&pEnum)) || !pEnum)
		return letter;

	IWbemClassObject* pObj = nullptr;
	ULONG ret = 0;
	if (SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret)) && ret && pObj)
	{
		VARIANT vDevId;
		VariantInit(&vDevId);
		if (SUCCEEDED(pObj->Get(L"DeviceID", 0, &vDevId, nullptr, nullptr)) &&
			vDevId.vt == VT_BSTR && vDevId.bstrVal)
		{
			const std::wstring part(vDevId.bstrVal, SysStringLen(vDevId.bstrVal));
			const std::wstring q2 =
				L"ASSOCIATORS OF {Win32_DiskPartition.DeviceID='" + part +
				L"'} WHERE AssocClass = Win32_LogicalDiskToPartition";

			IEnumWbemClassObject* pEnum2 = nullptr;
			Bstr query2(q2);
			if (SUCCEEDED(pSvc->ExecQuery(
					lang.get(),
					query2.get(),
					WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
					nullptr,
					&pEnum2)) && pEnum2)
			{
				IWbemClassObject* pObj2 = nullptr;
				ULONG ret2 = 0;
				if (SUCCEEDED(pEnum2->Next(WBEM_INFINITE, 1, &pObj2, &ret2)) && ret2 && pObj2)
				{
					VARIANT vName;
					VariantInit(&vName);
					if (SUCCEEDED(pObj2->Get(L"DeviceID", 0, &vName, nullptr, nullptr)) &&
						vName.vt == VT_BSTR && vName.bstrVal)
					{
						letter = BstrToUtf8(vName.bstrVal);
					}
					VariantClear(&vName);
					pObj2->Release();
				}
				pEnum2->Release();
			}
		}
		VariantClear(&vDevId);
		pObj->Release();
	}
	pEnum->Release();
	return letter;
}

} // namespace

std::string UsbHardwareSerial::ExtractSerialFromPnp(const std::wstring& pnpDeviceId)
{
	if (pnpDeviceId.empty())
		return "(UNKNOWN)";

	const size_t pos = pnpDeviceId.find_last_of(L'\\');
	std::wstring tail = (pos == std::wstring::npos) ? pnpDeviceId : pnpDeviceId.substr(pos + 1);

	const size_t amp = tail.find(L'&');
	if (amp != std::wstring::npos)
		tail = tail.substr(0, amp);

	std::string s = WideToUtf8(tail);
	s.erase(std::remove_if(s.begin(), s.end(),
			[](unsigned char c) {
				return c == '\0' || c == ' ' || c == '\r' || c == '\n';
			}),
		s.end());
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return static_cast<char>(std::toupper(c)); });

	if (s.empty())
		return "(UNKNOWN)";
	return s;
}

std::string UsbHardwareSerial::ExtractSerialFromPnp(const std::string& pnpDeviceId)
{
	return ExtractSerialFromPnp(Utf8ToWide(pnpDeviceId));
}

std::string UsbHardwareSerial::ResolveDriveLetterFromDiskDeviceId(const std::string& deviceId)
{
	ComInitializer com;
	if (!com.ok())
		return std::string();

	IWbemLocator* pLoc = nullptr;
	IWbemServices* pSvc = nullptr;
	if (!ConnectWmi(&pLoc, &pSvc))
		return std::string();

	const std::string letter = FindDriveLetter(pSvc, deviceId);
	pSvc->Release();
	pLoc->Release();
	return letter;
}

std::vector<UsbDiskInfo> UsbHardwareSerial::EnumerateConnectedUsbDisks()
{
	std::vector<UsbDiskInfo> result;

	ComInitializer com;
	if (!com.ok())
		return result;

	IWbemLocator* pLoc = nullptr;
	IWbemServices* pSvc = nullptr;
	if (!ConnectWmi(&pLoc, &pSvc))
		return result;

	IEnumWbemClassObject* pEnum = nullptr;
	Bstr lang(L"WQL");
	Bstr query(L"SELECT DeviceID, PNPDeviceID FROM Win32_DiskDrive WHERE InterfaceType = 'USB'");
	const HRESULT hr = pSvc->ExecQuery(
		lang.get(),
		query.get(),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		nullptr,
		&pEnum);

	if (FAILED(hr) || !pEnum)
	{
		pSvc->Release();
		pLoc->Release();
		return result;
	}

	for (;;)
	{
		IWbemClassObject* pObj = nullptr;
		ULONG ret = 0;
		if (FAILED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret)) || !ret || !pObj)
			break;

		UsbDiskInfo info;

		VARIANT vDeviceId;
		VARIANT vPnpId;
		VariantInit(&vDeviceId);
		VariantInit(&vPnpId);

		if (SUCCEEDED(pObj->Get(L"DeviceID", 0, &vDeviceId, nullptr, nullptr)) &&
			vDeviceId.vt == VT_BSTR && vDeviceId.bstrVal)
		{
			info.DeviceID = BstrToUtf8(vDeviceId.bstrVal);
		}

		if (SUCCEEDED(pObj->Get(L"PNPDeviceID", 0, &vPnpId, nullptr, nullptr)) &&
			vPnpId.vt == VT_BSTR && vPnpId.bstrVal)
		{
			info.PnpDeviceId = BstrToUtf8(vPnpId.bstrVal);
			info.Serial = ExtractSerialFromPnp(
				std::wstring(vPnpId.bstrVal, SysStringLen(vPnpId.bstrVal)));
		}
		else
		{
			info.Serial = "(UNKNOWN)";
		}

		if (!info.DeviceID.empty())
			info.DriveLetter = FindDriveLetter(pSvc, info.DeviceID);

		result.push_back(info);

		VariantClear(&vDeviceId);
		VariantClear(&vPnpId);
		pObj->Release();
	}

	pEnum->Release();
	pSvc->Release();
	pLoc->Release();
	return result;
}

std::string UsbHardwareSerial::GetSerialForDriveLetter(const std::string& driveLetter)
{
	const std::string want = NormalizeDriveLetter(driveLetter);
	if (want.empty())
		return std::string();

	const std::vector<UsbDiskInfo> disks = EnumerateConnectedUsbDisks();
	for (size_t i = 0; i < disks.size(); ++i)
	{
		if (NormalizeDriveLetter(disks[i].DriveLetter) == want)
			return disks[i].Serial;
	}
	return std::string();
}
