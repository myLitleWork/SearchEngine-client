#pragma once

#include "mySomeFunc.h"
#include "UserFileReplace.h"
#include "ClientFilePublish.h"

#include <System.IOUtils.hpp>
#include <System.Zip.hpp>

#include <filesystem>
#include <string>
#include <vector>

// Archive ZIP of an outgoing telegram (PRD), as written by GoodByeIAO:
// MESSAGE/aps/make_msg/tmpvl00{N}.srg — concatenation, not %03d.
// N=0 is the telegram body; N>=1 are PrilName[N-1] attachments.
namespace outgoing_archive_zip
{
	inline String PayloadName(int index)
	{
		return L"MESSAGE/aps/make_msg/tmpvl00" + IntToStr(index) + L".srg";
	}

	inline std::string ZipFileNameUtf8(int telegramId)
	{
		return std::to_string(telegramId) + ".zip";
	}

	inline void SplitPrilNames(const String& prilNames, std::vector<String>& tokens)
	{
		tokens.clear();
		const int len = prilNames.Length();
		int start = 1;
		while (start <= len) {
			int pos = 0;
			for (int i = start; i <= len; ++i) {
				if (prilNames[i] == L';') {
					pos = i;
					break;
				}
			}
			if (pos == 0) {
				tokens.push_back(prilNames.SubString(start, len - start + 1));
				break;
			}
			tokens.push_back(prilNames.SubString(start, pos - start));
			start = pos + 1;
		}
	}

	inline bool IsValidLeafName(const String& name)
	{
		if (name.IsEmpty() || name == L"." || name == L"..")
			return false;
		if (name.Pos(L"/") > 0 || name.Pos(L"\\") > 0 || name.Pos(L":") > 0)
			return false;
		if (ExtractFileName(name) != name)
			return false;

		try {
			const std::filesystem::path path(name.c_str());
			if (path.empty() ||
				path.is_absolute() ||
				path.has_root_path() ||
				path.has_root_name() ||
				path.has_root_directory() ||
				path.filename() != path)
				return false;
		}
		catch (...) {
			return false;
		}
		return true;
	}

	struct UniqueTempDir
	{
		String path;

		UniqueTempDir()
			: path(CreateUniqueExtractTempDir())
		{
		}

		~UniqueTempDir()
		{
			if (path.IsEmpty())
				return;
			try {
				TDirectory::Delete(path, true);
			}
			catch (...) {
			}
		}

		UniqueTempDir(const UniqueTempDir&) = delete;
		UniqueTempDir& operator=(const UniqueTempDir&) = delete;

		bool ok() const
		{
			return !path.IsEmpty();
		}
	};

	inline bool ExtractAttachmentToPath(
		const String& zipFile,
		int payloadIndex,
		const String& destFile,
		std::wstring* error)
	{
		namespace fs = std::filesystem;
		using client_file_publish::ResolveUnderBase;
		using client_file_publish::MoveNewFile;

		if (payloadIndex < 1) {
			client_file_publish::SetError(error, L"Некорректный индекс приложения");
			return false;
		}
		if (destFile.IsEmpty()) {
			client_file_publish::SetError(error, L"Путь файла не задан");
			return false;
		}
		if (!FileExists(zipFile) || !TZipFile::IsValid(zipFile)) {
			client_file_publish::SetError(
				error, L"Архив повреждён или имеет неверный формат");
			return false;
		}

		UniqueTempDir temp;
		if (!temp.ok()) {
			client_file_publish::SetError(
				error, L"Не удалось создать временный каталог");
			return false;
		}

		const String archivePath = PayloadName(payloadIndex);
		const fs::path tempBase(temp.path.c_str());
		fs::path archiveAbs;
		std::wstring pathError;
		fs::path archiveRel(
			StringReplace(archivePath, L"/", L"\\",
				TReplaceFlags() << rfReplaceAll).c_str());
		if (!ResolveUnderBase(tempBase, archiveRel, archiveAbs, &pathError)) {
			if (error)
				*error = L"Небезопасный путь в архиве: " + pathError;
			return false;
		}

		TZipFile* zip = new TZipFile();
		try {
			zip->Open(zipFile, zmRead);
			if (zip->IndexOf(archivePath) < 0) {
				const String message =
					L"В архиве отсутствует обязательная запись «" +
					archivePath + L"»";
				client_file_publish::SetError(error, message.c_str());
				zip->Close();
				delete zip;
				return false;
			}
			zip->Extract(archivePath, temp.path);
			zip->Close();
		}
		catch (const Exception& e) {
			try { zip->Close(); } catch (...) {}
			delete zip;
			if (error)
				*error = std::wstring(e.Message.c_str());
			return false;
		}
		catch (...) {
			try { zip->Close(); } catch (...) {}
			delete zip;
			client_file_publish::SetError(error, L"Не удалось извлечь файл из архива");
			return false;
		}
		delete zip;

		const String extractedFilePath = String(archiveAbs.c_str());
		if (!FileExists(extractedFilePath)) {
			client_file_publish::SetError(
				error, L"После извлечения файл не найден");
			return false;
		}

		const std::wstring destW(destFile.c_str());
		if (user_file_replace::IsDirectory(destW)) {
			client_file_publish::SetError(
				error, L"По конечному пути уже существует каталог");
			return false;
		}

		if (user_file_replace::IsRegularFile(destW)) {
			return user_file_replace::PublishExistingTemporary(
				std::wstring(extractedFilePath.c_str()), destW, error);
		}

		if (!MoveNewFile(archiveAbs, fs::path(destW), &pathError)) {
			if (error)
				*error = pathError;
			return false;
		}
		return true;
	}
}
