#pragma once

#include <Windows.h>

#include <atomic>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Narrow client-local helpers for save/ZIP/attachments:
// - keep relative paths under a chosen base (subdirs allowed, escape rejected);
// - publish new files via temp sibling + MoveFileExW without replace;
// - never treat equal size alone as proof of identical content.
// Prefixes intentionally differ from UserFileReplace (_SCX/_SCB).
namespace client_file_publish
{
	namespace fs = std::filesystem;

	inline std::string Narrow(const std::wstring& text)
	{
		if (text.empty())
			return {};
		const int size = WideCharToMultiByte(
			CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size <= 1)
			return {};
		std::string out(static_cast<std::size_t>(size - 1), '\0');
		WideCharToMultiByte(
			CP_UTF8, 0, text.c_str(), -1, &out[0], size, nullptr, nullptr);
		return out;
	}

	inline void SetError(std::wstring* error, const wchar_t* text, DWORD code = 0)
	{
		if (error == nullptr)
			return;
		*error = text;
		if (code != 0)
			*error += L" (" + std::to_wstring(static_cast<unsigned long long>(code)) + L")";
	}

	inline std::wstring PathToWide(const fs::path& path)
	{
		return path.wstring();
	}

	inline bool EqualsPathNoCase(const fs::path& left, const fs::path& right)
	{
		const std::wstring a = left.wstring();
		const std::wstring b = right.wstring();
		if (a.size() != b.size())
			return false;
		for (std::size_t i = 0; i < a.size(); ++i) {
			if (towlower(a[i]) != towlower(b[i]))
				return false;
		}
		return true;
	}

	inline bool IsUnderBase(const fs::path& baseAbs, const fs::path& candidateAbs)
	{
		auto baseIt = baseAbs.begin();
		auto candIt = candidateAbs.begin();
		for (; baseIt != baseAbs.end(); ++baseIt) {
			// A trailing separator becomes "." in C++Builder's normalized paths.
			// It names the same directory, rather than another path component.
			if (baseIt->empty() || *baseIt == ".")
				continue;
			if (candIt == candidateAbs.end())
				return false;
			if (!EqualsPathNoCase(*baseIt, *candIt))
				return false;
			++candIt;
		}
		return true;
	}

	inline fs::path LexicalAbsolute(const fs::path& path)
	{
		try
		{
			return fs::absolute(path).lexically_normal();
		}
		catch (...)
		{
			return path.lexically_normal(); // fallback on failure
		}
	}

	// relativeOrName may contain subdirectories. Absolute, rooted, empty, and ".." are rejected.
	inline bool ResolveUnderBase(
		const fs::path& baseDir,
		const fs::path& relativeOrName,
		fs::path& outAbsolute,
		std::wstring* error = nullptr)
	{
		if (relativeOrName.empty()) {
			SetError(error, L"Пустой относительный путь");
			return false;
		}
		if (relativeOrName.is_absolute() ||
			relativeOrName.has_root_name() ||
			relativeOrName.has_root_directory()) {
			SetError(error, L"Абсолютный или корневой путь запрещён");
			return false;
		}

		const fs::path normalized = relativeOrName.lexically_normal();
		if (normalized.empty() || normalized == "." || normalized == "..") {
			SetError(error, L"Недопустимый относительный путь");
			return false;
		}
		for (const auto& part : normalized) {
			if (part == "..") {
				SetError(error, L"Выход из выбранного каталога через .. запрещён");
				return false;
			}
			const std::wstring leaf = part.wstring();
			if (leaf.find(L':') != std::wstring::npos) {
				SetError(error, L"В имени файла запрещён символ ':'");
				return false;
			}
		}

		const fs::path baseAbs = LexicalAbsolute(baseDir);
		if (baseAbs.empty()) {
			SetError(error, L"Базовый каталог не задан");
			return false;
		}

		const fs::path fullAbs = LexicalAbsolute(baseAbs / normalized);
		if (!IsUnderBase(baseAbs, fullAbs)) {
			SetError(error, L"Путь выходит за пределы выбранного каталога");
			return false;
		}

		outAbsolute = fullAbs;
		return true;
	}

	inline bool IsRegularFileW(const fs::path& path)
	{
		const DWORD attributes = GetFileAttributesW(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	inline bool IsDirectoryW(const fs::path& path)
	{
		const DWORD attributes = GetFileAttributesW(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	// Prefer "(n) name.ext" — same user-visible scheme as SaveFileVh.
	inline bool EnsureUniqueFilePath(fs::path& path, int maxTries = 30)
	{
		if (!IsRegularFileW(path) && !IsDirectoryW(path))
			return true;

		const fs::path parent = path.parent_path();
		const fs::path fileName = path.filename();
		for (int tryLimit = 1; tryLimit <= maxTries; ++tryLimit) {
			const fs::path candidate =
				parent / (L"(" + std::to_wstring(tryLimit) + L") " + fileName.wstring());
			if (!IsRegularFileW(candidate) && !IsDirectoryW(candidate)) {
				path = candidate;
				return true;
			}
		}
		return false;
	}

	inline std::wstring NextTemporaryLeaf()
	{
		static std::atomic<unsigned int> counter{1};
		const unsigned int sequence = counter.fetch_add(1);
		std::wostringstream out;
		out << L"_SCF-"
			<< std::uppercase << std::hex << std::setfill(L'0') << std::setw(8)
			<< static_cast<unsigned>(GetCurrentProcessId())
			<< L'-' << std::setw(8) << sequence
			<< L".tmp";
		return out.str();
	}

	struct HandleGuard
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
		~HandleGuard()
		{
			if (handle != INVALID_HANDLE_VALUE)
				CloseHandle(handle);
		}
		HANDLE Release()
		{
			HANDLE value = handle;
			handle = INVALID_HANDLE_VALUE;
			return value;
		}
	};

	struct TemporaryFileGuard
	{
		std::wstring path;
		bool keep = false;
		~TemporaryFileGuard()
		{
			if (!keep && !path.empty())
				DeleteFileW(path.c_str());
		}
		void Release()
		{
			keep = true;
			path.clear();
		}
	};

	inline bool WriteAll(HANDLE handle, const void* data, std::size_t size, std::wstring* error)
	{
		const auto* bytes = static_cast<const unsigned char*>(data);
		std::size_t offset = 0;
		while (offset < size) {
			const DWORD chunk = static_cast<DWORD>(
				(size - offset) > 0x400000 ? 0x400000 : (size - offset));
			DWORD written = 0;
			if (!WriteFile(handle, bytes + offset, chunk, &written, nullptr) || written != chunk) {
				SetError(error, L"Не удалось записать временный файл", GetLastError());
				return false;
			}
			offset += written;
		}
		return true;
	}

	inline bool CreateUniqueTemporary(
		const fs::path& parent,
		std::wstring& temporaryPath,
		HANDLE& createdHandle,
		std::wstring* error)
	{
		for (int attempt = 0; attempt < 256; ++attempt) {
			const std::wstring candidate = (parent / NextTemporaryLeaf()).wstring();
			HANDLE handle = CreateFileW(
				candidate.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
				CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE) {
				temporaryPath = candidate;
				createdHandle = handle;
				return true;
			}
			const DWORD errorCode = GetLastError();
			if (errorCode == ERROR_FILE_EXISTS || errorCode == ERROR_ALREADY_EXISTS)
				continue;
			SetError(error, L"Не удалось создать временный файл", errorCode);
			return false;
		}
		SetError(error, L"Не удалось выбрать уникальное временное имя");
		return false;
	}

	// Create-new publish: never replaces an existing final path.
	inline bool WriteNewFile(
		const fs::path& absolutePath,
		const void* data,
		std::size_t size,
		std::wstring* error = nullptr)
	{
		if (absolutePath.empty()) {
			SetError(error, L"Путь файла не задан");
			return false;
		}
		if (IsDirectoryW(absolutePath)) {
			SetError(error, L"По конечному пути уже существует каталог");
			return false;
		}
		if (IsRegularFileW(absolutePath)) {
			SetError(error, L"Файл уже существует; замена запрещена");
			return false;
		}

		const fs::path parent = absolutePath.parent_path();
		if (parent.empty()) {
			SetError(error, L"Не удалось определить каталог назначения");
			return false;
		}

		std::error_code ec;
		fs::create_directories(parent, ec);
		if (ec && !IsDirectoryW(parent)) {
			SetError(error, L"Не удалось создать каталог назначения");
			return false;
		}

		std::wstring temporaryPath;
		HandleGuard file;
		if (!CreateUniqueTemporary(parent, temporaryPath, file.handle, error))
			return false;

		TemporaryFileGuard temporaryGuard;
		temporaryGuard.path = temporaryPath;

		if (!WriteAll(file.handle, data, size, error))
			return false;
		if (!FlushFileBuffers(file.handle)) {
			SetError(error, L"Не удалось сохранить временный файл", GetLastError());
			return false;
		}

		LARGE_INTEGER writtenSize{};
		if (!GetFileSizeEx(file.handle, &writtenSize) ||
			writtenSize.QuadPart != static_cast<LONGLONG>(size)) {
			SetError(error, L"Размер временного файла не совпал с записанными данными");
			return false;
		}

		const HANDLE rawHandle = file.Release();
		if (!CloseHandle(rawHandle)) {
			SetError(error, L"Не удалось закрыть временный файл", GetLastError());
			return false;
		}

		if (!MoveFileExW(
				temporaryPath.c_str(), absolutePath.c_str(), MOVEFILE_WRITE_THROUGH)) {
			SetError(error, L"Не удалось опубликовать новый файл", GetLastError());
			return false;
		}
		temporaryGuard.Release();
		return true;
	}

	inline bool MoveNewFile(const fs::path& fromPath, const fs::path& toPath, std::wstring* error = nullptr)
	{
		if (IsRegularFileW(toPath) || IsDirectoryW(toPath)) {
			SetError(error, L"Файл уже существует; замена запрещена");
			return false;
		}
		const fs::path parent = toPath.parent_path();
		std::error_code ec;
		fs::create_directories(parent, ec);
		if (ec && !IsDirectoryW(parent)) {
			SetError(error, L"Не удалось создать каталог назначения");
			return false;
		}
		if (!MoveFileExW(fromPath.c_str(), toPath.c_str(), MOVEFILE_WRITE_THROUGH)) {
			SetError(error, L"Не удалось переместить извлечённый файл", GetLastError());
			return false;
		}
		return true;
	}

	struct ZipExtractStats
	{
		int expected = 0;
		int extracted = 0;
		int failed = 0;
		std::vector<std::wstring> errors;

		bool fullyOk() const
		{
			return failed == 0 && extracted == expected && expected >= 0;
		}
	};
}
