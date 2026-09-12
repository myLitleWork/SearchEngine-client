//---------------------------------------------------------------------------

#ifndef mySomeFuncH
#define mySomeFuncH

#include <map>
#include <vector>
#include <atomic>
#include <System.Zip.hpp>
#include <System.IOUtils.hpp>
#include "ClientFilePublish.h"

inline String CreateUniqueExtractTempDir()
{
	for (int attempt = 0; attempt < 256; ++attempt) {
		static std::atomic<unsigned int> zipTempSeq{1};
		const unsigned int sequence = zipTempSeq.fetch_add(1);
		const String tempDir = TPath::GetTempPath() +
			L"ZipExtractTemp-" +
			UIntToStr(static_cast<unsigned>(GetCurrentProcessId())) +
			L"-" + UIntToStr(sequence) + L"\\";
		if (TDirectory::Exists(tempDir))
			continue;
		TDirectory::CreateDirectory(tempDir);
		if (TDirectory::Exists(tempDir))
			return tempDir;
	}
	return String();
}

void setUniqFileName(String& full_file_dir)
{

	if (!FileExists(full_file_dir))
		return;

	auto file_dir 	= ExtractFileDir(full_file_dir);
	auto ext 		= ExtractFileExt(full_file_dir);
	auto name	    = TPath::GetFileNameWithoutExtension(full_file_dir);


	int try_count = 0;

	String new_file_name;

	do
	{
		++try_count;

		if (try_count > 30000)
			return;

		auto postfix = "(" + IntToStr(try_count) + ")";

		new_file_name = file_dir + "\\" + name + postfix + ext;
	}
	while (FileExists(new_file_name) && try_count <= 30000);

	full_file_dir = new_file_name;
}


inline client_file_publish::ZipExtractStats ExtZip(
	String zipFile,
	String targetDir,
	const std::map<String, String>& fileMappings)
{
	using client_file_publish::ZipExtractStats;
	using client_file_publish::ResolveUnderBase;
	using client_file_publish::EnsureUniqueFilePath;
	using client_file_publish::MoveNewFile;
	namespace fs = std::filesystem;

	ZipExtractStats stats;
	stats.expected = static_cast<int>(fileMappings.size());
	if (stats.expected <= 0)
		return stats;

	if (!FileExists(zipFile) || !TZipFile::IsValid(zipFile)) {
		stats.failed = stats.expected;
		stats.errors.push_back(L"Архив повреждён или имеет неверный формат");
		return stats;
	}

	TZipFile *z = new TZipFile();
	try {
		z->Open(zipFile, zmRead);

		const String tempDir = CreateUniqueExtractTempDir();
		if (tempDir.IsEmpty()) {
			stats.failed = stats.expected;
			stats.errors.push_back(L"Не удалось создать временный каталог распаковки");
			z->Close();
		} else {

		const fs::path baseDir(targetDir.c_str());
		const fs::path tempBase(tempDir.c_str());

		for (const auto& pair : fileMappings)
		{
			String archivePath = pair.first;
			archivePath = StringReplace(archivePath, "\\", "/", TReplaceFlags() << rfReplaceAll);

			std::wstring pathError;
			fs::path archiveRel(StringReplace(pair.first, "/", "\\", TReplaceFlags() << rfReplaceAll).c_str());
			fs::path archiveAbs;
			if (!ResolveUnderBase(tempBase, archiveRel, archiveAbs, &pathError)) {
				++stats.failed;
				stats.errors.push_back(
					L"Небезопасный путь в архиве \"" + std::wstring(archivePath.c_str()) +
					L"\": " + pathError);
				continue;
			}

			fs::path targetAbs;
			fs::path targetRel(pair.second.c_str());
			if (!ResolveUnderBase(baseDir, targetRel, targetAbs, &pathError)) {
				++stats.failed;
				stats.errors.push_back(
					L"Небезопасный целевой путь \"" + std::wstring(pair.second.c_str()) +
					L"\": " + pathError);
				continue;
			}
			if (!EnsureUniqueFilePath(targetAbs)) {
				++stats.failed;
				stats.errors.push_back(
					L"Не удалось подобрать свободное имя для \"" +
					std::wstring(pair.second.c_str()) + L"\"");
				continue;
			}

			try
			{
				if (z->IndexOf(archivePath) == -1)
				{
					++stats.failed;
					stats.errors.push_back(
						L"В архиве отсутствует обязательная запись «" +
						std::wstring(archivePath.c_str()) + L"»");
					continue;
				}

				z->Extract(archivePath, tempDir);

				String extractedFilePath = tempDir + archivePath;
				extractedFilePath = StringReplace(
					extractedFilePath, "/", "\\", TReplaceFlags() << rfReplaceAll);

				if (!FileExists(extractedFilePath)) {
					++stats.failed;
					stats.errors.push_back(
						L"После извлечения не найден файл \"" +
						std::wstring(extractedFilePath.c_str()) + L"\"");
					continue;
				}

				if (!MoveNewFile(fs::path(extractedFilePath.c_str()), targetAbs, &pathError)) {
					++stats.failed;
					stats.errors.push_back(
						L"\"" + std::wstring(archivePath.c_str()) + L"\": " + pathError);
					continue;
				}
				++stats.extracted;
			}
			catch (const Exception& e)
			{
				++stats.failed;
				stats.errors.push_back(
					L"Ошибка извлечения \"" + std::wstring(archivePath.c_str()) +
					L"\": " + std::wstring(e.Message.c_str()));
			}
		}

		TDirectory::Delete(tempDir, true);
		z->Close();
		} // unique tempDir created
	}
	__finally {
		delete z;
	}

	return stats;
}

inline client_file_publish::ZipExtractStats copyFile(
	const std::vector<String>& prils,
	const String& archive_zip_dir,
	const String& out_dir)
{
	using client_file_publish::ZipExtractStats;

	std::map<String, String> fileMappings;
	for (int i = 0; i < static_cast<int>(prils.size()); i++)
	{
		const String targetFilePath = prils[i].Trim();
		if (targetFilePath.IsEmpty() ||
			targetFilePath == L"." ||
			targetFilePath == L".." ||
			ExtractFileName(targetFilePath) != targetFilePath ||
			targetFilePath.Pos(L"/") > 0 ||
			targetFilePath.Pos(L"\\") > 0 ||
			targetFilePath.Pos(L":") > 0)
			continue;

		const String archiveFilePath =
			L"MESSAGE\\aps\\make_msg\\tmpvl00" + IntToStr(i + 1) + L".srg";
		fileMappings[archiveFilePath] = targetFilePath;
	}

	if (fileMappings.empty()) {
		ZipExtractStats stats;
		stats.expected = static_cast<int>(prils.size());
		if (stats.expected <= 0)
			stats.expected = 1;
		stats.failed = stats.expected;
		stats.errors.push_back(L"Нет приложений для извлечения");
		return stats;
	}

	return ExtZip(archive_zip_dir, out_dir, fileMappings);
}

//---------------------------------------------------------------------------
#endif
