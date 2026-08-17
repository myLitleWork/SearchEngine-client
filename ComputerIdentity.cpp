//---------------------------------------------------------------------------

#pragma hdrstop

#include "ComputerIdentity.h"
#include "DeviceIdentity.h"

#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemidl.h>

#include <string>

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
	const int n = WideCharToMultiByte(
		CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()),
		nullptr, 0, nullptr, nullptr);
	if (n <= 0)
		return std::string();
	std::string out(static_cast<size_t>(n), '\0');
	WideCharToMultiByte(
		CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()),
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
	if (FAILED(CoCreateInstance(
			CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, reinterpret_cast<void**>(&pLoc))) ||
		!pLoc)
	{
		return false;
	}

	IWbemServices* pSvc = nullptr;
	Bstr ns(L"ROOT\\CIMV2");
	const HRESULT hr = pLoc->ConnectServer(
		ns.get(), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
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

} // namespace

std::optional<std::string> QueryComputerSystemProductUuid()
{
	ComInitializer com;
	if (!com.ok())
		return std::nullopt;

	IWbemLocator* pLoc = nullptr;
	IWbemServices* pSvc = nullptr;
	if (!ConnectWmi(&pLoc, &pSvc))
		return std::nullopt;

	IEnumWbemClassObject* pEnum = nullptr;
	Bstr lang(L"WQL");
	Bstr query(L"SELECT UUID FROM Win32_ComputerSystemProduct");
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
		return std::nullopt;
	}

	std::string raw;
	IWbemClassObject* pObj = nullptr;
	ULONG ret = 0;
	if (SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &ret)) && ret && pObj)
	{
		VARIANT vUuid;
		VariantInit(&vUuid);
		if (SUCCEEDED(pObj->Get(L"UUID", 0, &vUuid, nullptr, nullptr)) &&
			vUuid.vt == VT_BSTR && vUuid.bstrVal)
		{
			raw = BstrToUtf8(vUuid.bstrVal);
		}
		VariantClear(&vUuid);
		pObj->Release();
	}

	pEnum->Release();
	pSvc->Release();
	pLoc->Release();

	return device_identity::NormalizeComputerUuid(std::move(raw));
}
