//---------------------------------------------------------------------------

#pragma hdrstop

#include "UsbAuthWatcher.h"
#include "UsbHardwareSerial.h"

#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemidl.h>

#include <atomic>
#include <thread>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")

//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

class Bstr
{
public:
	explicit Bstr(const wchar_t* s)
		: bstr_(s ? SysAllocString(s) : nullptr)
	{
	}

	~Bstr()
	{
		if (bstr_)
			SysFreeString(bstr_);
	}

	BSTR get() const { return bstr_; }

private:
	Bstr(const Bstr&);
	Bstr& operator=(const Bstr&);
	BSTR bstr_;
};

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
	if (FAILED(pLoc->ConnectServer(ns.get(), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc)) || !pSvc)
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

std::string SerialFromDiskInstance(IWbemClassObject* pInst)
{
	VARIANT vPnp;
	VariantInit(&vPnp);
	std::string serial = "(UNKNOWN)";
	if (SUCCEEDED(pInst->Get(L"PNPDeviceID", 0, &vPnp, nullptr, nullptr)) &&
		vPnp.vt == VT_BSTR && vPnp.bstrVal)
	{
		serial = UsbHardwareSerial::ExtractSerialFromPnp(
			std::wstring(vPnp.bstrVal, SysStringLen(vPnp.bstrVal)));
	}
	VariantClear(&vPnp);
	return serial;
}

std::string DeviceIdFromDiskInstance(IWbemClassObject* pInst)
{
	VARIANT vId;
	VariantInit(&vId);
	std::string id;
	if (SUCCEEDED(pInst->Get(L"DeviceID", 0, &vId, nullptr, nullptr)) &&
		vId.vt == VT_BSTR && vId.bstrVal)
	{
		id = BstrToUtf8(vId.bstrVal);
	}
	VariantClear(&vId);
	return id;
}

} // namespace

class UsbAuthWatcher::Impl
{
public:
	std::atomic_bool running{false};
	std::thread worker;
	InsertFn onInsert;
	RemoveFn onRemove;

	bool start(InsertFn insertCb, RemoveFn removeCb)
	{
		if (running.load())
			return false;
		onInsert = std::move(insertCb);
		onRemove = std::move(removeCb);
		running = true;
		worker = std::thread([this]() { run(); });
		return true;
	}

	void stop()
	{
		running = false;
		if (worker.joinable())
			worker.join();
	}

	void run()
	{
		HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool ownsCom = (hrCom == S_OK || hrCom == S_FALSE);
		if (FAILED(hrCom) && hrCom != RPC_E_CHANGED_MODE)
		{
			running = false;
			return;
		}

		IWbemLocator* pLoc = nullptr;
		IWbemServices* pSvc = nullptr;
		if (!ConnectWmi(&pLoc, &pSvc))
		{
			if (ownsCom)
				CoUninitialize();
			running = false;
			return;
		}

		Bstr lang(L"WQL");
		Bstr qInsert(
			L"SELECT * FROM __InstanceCreationEvent WITHIN 2 "
			L"WHERE TargetInstance ISA 'Win32_DiskDrive' "
			L"AND TargetInstance.InterfaceType = 'USB'");
		Bstr qRemove(
			L"SELECT * FROM __InstanceDeletionEvent WITHIN 2 "
			L"WHERE TargetInstance ISA 'Win32_DiskDrive' "
			L"AND TargetInstance.InterfaceType = 'USB'");

		IEnumWbemClassObject* pEnumInsert = nullptr;
		IEnumWbemClassObject* pEnumRemove = nullptr;
		pSvc->ExecNotificationQuery(
			lang.get(), qInsert.get(),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			nullptr, &pEnumInsert);
		pSvc->ExecNotificationQuery(
			lang.get(), qRemove.get(),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			nullptr, &pEnumRemove);

		while (running.load())
		{
			if (pEnumInsert)
			{
				IWbemClassObject* pObj = nullptr;
				ULONG ret = 0;
				if (SUCCEEDED(pEnumInsert->Next(500, 1, &pObj, &ret)) && ret && pObj)
				{
					VARIANT vtInst;
					VariantInit(&vtInst);
					if (SUCCEEDED(pObj->Get(L"TargetInstance", 0, &vtInst, nullptr, nullptr)) &&
						vtInst.vt == VT_UNKNOWN && vtInst.punkVal)
					{
						IWbemClassObject* pInst = nullptr;
						if (SUCCEEDED(vtInst.punkVal->QueryInterface(
								IID_IWbemClassObject, reinterpret_cast<void**>(&pInst))) && pInst)
						{
							const std::string serial = SerialFromDiskInstance(pInst);
							const std::string deviceId = DeviceIdFromDiskInstance(pInst);
							std::string letter;
							if (!deviceId.empty())
								letter = UsbHardwareSerial::ResolveDriveLetterFromDiskDeviceId(deviceId);
							if (onInsert)
								onInsert(serial, letter);
							pInst->Release();
						}
					}
					VariantClear(&vtInst);
					pObj->Release();
				}
			}

			if (pEnumRemove)
			{
				IWbemClassObject* pObjR = nullptr;
				ULONG retR = 0;
				if (SUCCEEDED(pEnumRemove->Next(500, 1, &pObjR, &retR)) && retR && pObjR)
				{
					VARIANT vtInst;
					VariantInit(&vtInst);
					if (SUCCEEDED(pObjR->Get(L"TargetInstance", 0, &vtInst, nullptr, nullptr)) &&
						vtInst.vt == VT_UNKNOWN && vtInst.punkVal)
					{
						IWbemClassObject* pInst = nullptr;
						if (SUCCEEDED(vtInst.punkVal->QueryInterface(
								IID_IWbemClassObject, reinterpret_cast<void**>(&pInst))) && pInst)
						{
							const std::string serial = SerialFromDiskInstance(pInst);
							if (onRemove)
								onRemove(serial);
							pInst->Release();
						}
					}
					VariantClear(&vtInst);
					pObjR->Release();
				}
			}
		}

		if (pEnumInsert)
			pEnumInsert->Release();
		if (pEnumRemove)
			pEnumRemove->Release();
		pSvc->Release();
		pLoc->Release();
		if (ownsCom)
			CoUninitialize();
	}
};

UsbAuthWatcher::UsbAuthWatcher()
	: pImpl_(new Impl())
{
}

UsbAuthWatcher::~UsbAuthWatcher()
{
	stop();
	delete pImpl_;
	pImpl_ = nullptr;
}

bool UsbAuthWatcher::start(InsertFn onInsert, RemoveFn onRemove)
{
	return pImpl_->start(std::move(onInsert), std::move(onRemove));
}

void UsbAuthWatcher::stop()
{
	if (pImpl_)
		pImpl_->stop();
}

bool UsbAuthWatcher::running() const
{
	return pImpl_ && pImpl_->running.load();
}
