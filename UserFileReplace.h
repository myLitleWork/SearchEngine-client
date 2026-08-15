#pragma once

#include <Windows.h>

#include <atomic>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Compact local equivalent of Otpravka SafeIo::WriteUserDataReplacing:
// write a unique sibling, flush/close/verify, then publish with one
// same-volume rename (MoveFileExW) or ReplaceFileW. The previous file is
// not removed until the new content is fully checked. This is not a port
// of SafeIo; leftover names use SearchClient prefixes, not _SPF/_SPB.
namespace user_file_replace
{
	inline void SetError(std::wstring* error, const wchar_t* text, DWORD code = 0)
	{
		if (error == nullptr)
			return;
		*error = text;
		if (code != 0)
			*error += L" (" + std::to_wstring(static_cast<unsigned long long>(code)) + L")";
	}

	inline std::wstring ParentDirectory(const std::wstring& path)
	{
		const std::wstring::size_type separator = path.find_last_of(L"\\/");
		if (separator == std::wstring::npos)
			return {};
		return path.substr(0, separator);
	}

	inline bool IsDirectory(const std::wstring& path)
	{
		const DWORD attributes = GetFileAttributesW(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	inline bool IsRegularFile(const std::wstring& path)
	{
		const DWORD attributes = GetFileAttributesW(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	inline std::wstring NextTemporaryLeaf(const wchar_t* prefix, const wchar_t* suffix)
	{
		static std::atomic<unsigned int> counter{1};
		const unsigned int sequence = counter.fetch_add(1);
		std::wostringstream out;
		out << prefix
			<< std::uppercase << std::hex << std::setfill(L'0') << std::setw(8)
			<< static_cast<unsigned>(GetCurrentProcessId())
			<< L'-' << std::setw(8) << sequence
			<< suffix;
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

	inline bool ReadAll(const std::wstring& path, std::string& bytes, std::wstring* error = nullptr)
	{
		bytes.clear();
		HandleGuard file;
		file.handle = CreateFileW(
			path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (file.handle == INVALID_HANDLE_VALUE) {
			SetError(error, L"Не удалось открыть файл для чтения", GetLastError());
			return false;
		}

		LARGE_INTEGER size{};
		if (!GetFileSizeEx(file.handle, &size) || size.QuadPart < 0) {
			SetError(error, L"Не удалось определить размер файла", GetLastError());
			return false;
		}

		bytes.resize(static_cast<size_t>(size.QuadPart));
		std::size_t offset = 0;
		while (offset < bytes.size()) {
			const DWORD chunk = static_cast<DWORD>(
				(bytes.size() - offset) > 0x400000 ? 0x400000 : (bytes.size() - offset));
			DWORD read = 0;
			if (!ReadFile(file.handle, &bytes[offset], chunk, &read, nullptr)) {
				SetError(error, L"Не удалось прочитать файл", GetLastError());
				bytes.clear();
				return false;
			}
			if (read == 0)
				break;
			offset += read;
		}
		if (offset != bytes.size()) {
			SetError(error, L"Неполное чтение файла");
			bytes.clear();
			return false;
		}
		return true;
	}

	inline bool CreateUniqueTemporary(
		const std::wstring& parent,
		std::wstring& temporaryPath,
		HANDLE& createdHandle,
		std::wstring* error)
	{
		for (int attempt = 0; attempt < 256; ++attempt) {
			const std::wstring candidate = parent + L"\\" +
				NextTemporaryLeaf(L"_SCX-", L".tmp");
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

	inline bool PublishTemporary(
		const std::wstring& temporaryPath,
		const std::wstring& finalPath,
		TemporaryFileGuard& temporaryGuard,
		std::wstring* error)
	{
		if (IsDirectory(finalPath)) {
			SetError(error, L"По конечному пути уже существует каталог");
			return false;
		}

		if (!IsRegularFile(finalPath)) {
			if (!MoveFileExW(
					temporaryPath.c_str(), finalPath.c_str(), MOVEFILE_WRITE_THROUGH)) {
				SetError(error, L"Не удалось опубликовать новый файл", GetLastError());
				return false;
			}
			temporaryGuard.Release();
			return true;
		}

		const std::wstring parent = ParentDirectory(finalPath);
		for (int attempt = 0; attempt < 256; ++attempt) {
			const std::wstring backupPath = parent + L"\\" +
				NextTemporaryLeaf(L"_SCB-", L".bak");
			if (IsRegularFile(backupPath) || IsDirectory(backupPath))
				continue;

			if (ReplaceFileW(
					finalPath.c_str(), temporaryPath.c_str(), backupPath.c_str(),
					0, nullptr, nullptr)) {
				temporaryGuard.Release();
				if (!DeleteFileW(backupPath.c_str())) {
					const DWORD cleanupError = GetLastError();
					OutputDebugStringW((
						L"SearchClient: файл заменён, но recovery-backup " +
						backupPath + L" не удалён. Windows " +
						std::to_wstring(static_cast<unsigned long long>(cleanupError)) +
						L".\n").c_str());
				}
				return true;
			}

			DWORD errorCode = GetLastError();
			if (errorCode == ERROR_UNABLE_TO_MOVE_REPLACEMENT_2) {
				const bool targetExists = IsRegularFile(finalPath);
				if (!targetExists && MoveFileExW(
						backupPath.c_str(), finalPath.c_str(), MOVEFILE_WRITE_THROUGH)) {
					SetError(error,
						L"Атомарная замена не завершилась; старая версия восстановлена",
						errorCode);
					return false;
				}
				temporaryGuard.Release();
				SetError(error,
					L"Атомарная замена не завершилась; сохранены старая и новая копии",
					errorCode);
				return false;
			}
			if (errorCode == ERROR_FILE_EXISTS || errorCode == ERROR_ALREADY_EXISTS)
				continue;

			if ((errorCode == ERROR_FILE_NOT_FOUND || errorCode == ERROR_PATH_NOT_FOUND) &&
				!IsRegularFile(finalPath)) {
				if (MoveFileExW(
						temporaryPath.c_str(), finalPath.c_str(), MOVEFILE_WRITE_THROUGH)) {
					temporaryGuard.Release();
					return true;
				}
				errorCode = GetLastError();
			}

			SetError(error, L"Не удалось атомарно заменить файл", errorCode);
			return false;
		}

		SetError(error, L"Не удалось выбрать уникальное имя recovery-backup");
		return false;
	}

	inline bool WriteReplacing(
		const std::wstring& finalPath,
		const void* data,
		std::size_t size,
		std::wstring* error = nullptr)
	{
		if (finalPath.empty()) {
			SetError(error, L"Путь файла не задан");
			return false;
		}

		const std::wstring parent = ParentDirectory(finalPath);
		if (parent.empty() || !IsDirectory(parent)) {
			SetError(error, L"Не удалось определить каталог назначения");
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

		return PublishTemporary(temporaryPath, finalPath, temporaryGuard, error);
	}

	// Publish a sibling already written and closed (for example a SQLite file).
	// Same create-new / ReplaceFileW rules as WriteReplacing. On failure the
	// temporary file is deleted unless replacement left both copies.
	inline bool PublishExistingTemporary(
		const std::wstring& temporaryPath,
		const std::wstring& finalPath,
		std::wstring* error = nullptr)
	{
		if (temporaryPath.empty() || finalPath.empty()) {
			SetError(error, L"Путь файла не задан");
			return false;
		}
		if (!IsRegularFile(temporaryPath)) {
			SetError(error, L"Временный файл для публикации не найден");
			return false;
		}

		TemporaryFileGuard temporaryGuard;
		temporaryGuard.path = temporaryPath;
		return PublishTemporary(temporaryPath, finalPath, temporaryGuard, error);
	}
}
