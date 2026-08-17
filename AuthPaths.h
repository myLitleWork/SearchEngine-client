#pragma once

#include <Windows.h>

#include <string>
#include <vector>

namespace auth_paths
{
	inline std::wstring executableDirectory()
	{
		// GetModuleFileNameW keeps the path Unicode end-to-end. The previous
		// AnsiString conversion corrupted Cyrillic directory names.
		std::vector<wchar_t> buffer(32768, L'\0');
		const DWORD length = GetModuleFileNameW(
			nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0 || length >= buffer.size())
			return {};

		std::wstring modulePath(buffer.data(), length);
		const std::wstring::size_type separator = modulePath.find_last_of(L"\\/");
		if (separator == std::wstring::npos)
			return {};
		return modulePath.substr(0, separator);
	}

	inline std::wstring fileBesideExecutable(const wchar_t* fileName)
	{
		const std::wstring directory = executableDirectory();
		if (directory.empty() || fileName == nullptr || *fileName == L'\0')
			return {};
		return directory + L"\\" + fileName;
	}

	inline std::wstring programDataSearchEngineDirectory()
	{
		wchar_t buffer[MAX_PATH] = {};
		const DWORD length = GetEnvironmentVariableW(L"ProgramData", buffer, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
			return {};
		return std::wstring(buffer, length) + L"\\SearchEngine";
	}

	inline std::wstring programDataAuthTokenPath()
	{
		const std::wstring directory = programDataSearchEngineDirectory();
		if (directory.empty())
			return {};
		return directory + L"\\searchclient-auth-token.json";
	}

	inline std::wstring exeAuthTokenPath()
	{
		return fileBesideExecutable(L"searchclient-auth-token.json");
	}

	inline bool isRegularFile(const std::wstring& path)
	{
		if (path.empty())
			return false;
		const DWORD attributes = GetFileAttributesW(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	inline std::string toUtf8(const std::wstring& value)
	{
		if (value.empty())
			return {};
		const int length = WideCharToMultiByte(
			CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
			static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
		if (length <= 0)
			return {};
		std::string result(static_cast<std::size_t>(length), '\0');
		if (WideCharToMultiByte(
				CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
				static_cast<int>(value.size()), &result[0], length,
				nullptr, nullptr) != length)
			return {};
		return result;
	}
}
