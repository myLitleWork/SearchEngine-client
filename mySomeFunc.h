//---------------------------------------------------------------------------

#ifndef mySomeFuncH
#define mySomeFuncH

#include <map>
#include <vector>
#include <System.Zip.hpp>
#include <System.IOUtils.hpp>

// Проверка, существует ли файл в указанной директории

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


void ExtZip(String zipFile, String targetDir, const std::map<String, String>& fileMappings)
{
	TZipFile *z = new TZipFile();
	z->Open(zipFile, zmRead);

	// Создаем временную директорию для извлечения файлов
	String tempDir = TPath::GetTempPath() + "ZipExtractTemp\\";
    if (!TDirectory::Exists(tempDir))
	{
		TDirectory::CreateDirectory(tempDir);
	}

    // Извлечение файлов, указанных в fileMappings
    for (const auto& pair : fileMappings)
    {
		String archivePath = pair.first;

        // Заменяем обратные слэши на прямые для совпадения с путями в архиве
        archivePath = StringReplace(archivePath, "\\", "/", TReplaceFlags() << rfReplaceAll);

        try
        {
			// Проверяем, существует ли файл в архиве
            if (z->IndexOf(archivePath) == -1)
			{
                std::cerr << "Файл " << archivePath.c_str() << " не найден в архиве." << std::endl;
                continue;
            }

            // Извлекаем файл в временную директорию, сохраняется внутренняя структура директорий
            z->Extract(archivePath, tempDir);

            // Полный путь к извлеченному файлу, включая внутренние директории из архива
            String extractedFilePath = tempDir + archivePath;

            // Заменяем прямые слэши на обратные для корректной работы с путями Windows
            extractedFilePath = StringReplace(extractedFilePath, "/", "\\", TReplaceFlags() << rfReplaceAll);

            // Целевой путь с новым именем файла
			String targetPath = targetDir + "\\" + pair.second;

			setUniqFileName(targetPath);

            // Создаем директорию назначения, если она не существует
            String targetDirPath = TPath::GetDirectoryName(targetPath);
			if (!TDirectory::Exists(targetDirPath))
            {
                TDirectory::CreateDirectory(targetDirPath);
            }

            // Перемещаем и переименовываем файл
			TFile::Move(extractedFilePath, targetPath);
        }
        catch (const Exception& e)
        {
            std::cerr << "Ошибка при извлечении файла " << archivePath.c_str() << ": " << e.Message.c_str() << std::endl;
        }
	}

    // Удаляем временную директорию после завершения
    TDirectory::Delete(tempDir, true);

	z->Close();
	delete z;
}

void copyFile(const std::vector<String>& prils,  const String& archive_zip_dir, const String& out_dir)
{


			std::map<String, String> fileMappings;

			for (int i = 0; i < prils.size(); i++)
			{
				String archiveFilePath = "MESSAGE\\aps\\make_msg\\tmpvl00" + IntToStr(i + 1) + ".srg";
				String targetFilePath = prils[i];
				fileMappings[archiveFilePath] = targetFilePath;
			}

			ExtZip(archive_zip_dir, out_dir, fileMappings);

}

//---------------------------------------------------------------------------
#endif
