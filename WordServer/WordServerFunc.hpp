#include "WordServer.h"
#include <filesystem>  // Для работы с путями
#include <fstream>     // Для работы с файлами
#include <thread>      // Для асинхронного удаления
#include <mutex>       // Для защиты ресурсов
#include <chrono>
#include <ComObj.hpp>
#include <Vcl.Dialogs.hpp>
#include <System.hpp> // Для работы с Exception
#include <System.SysUtils.hpp>
#include <System.Variants.hpp>


namespace fs = std::filesystem;

void WordServer::RemoveTemporaryFolder() {
    try {
        // Определяем путь к временной папке WordServer
        UnicodeString tempDir = UnicodeString(fs::temp_directory_path().u16string().c_str());
        UnicodeString wordServerDir = tempDir + L"\\WordServer";

        // Удаляем папку WordServer, если она существует
		if (fs::exists(wordServerDir.c_str())) {
			fs::remove_all(wordServerDir.c_str());  // Рекурсивное удаление папки и её содержимого
		}
	} catch (const std::exception& e) {
        // Логгируем ошибки при удалении временной папки
		OutputDebugString((L"Ошибка удаления временной папки WordServer: " + UnicodeString(e.what())).c_str());
    }
}


int WordServer::GetPrintedSheetCount() {
    try {
        // Получаем общее количество страниц в документе
        const int wdStatisticPages = 2;
        int totalPageCount = activeDoc.OleFunction("ComputeStatistics", wdStatisticPages);
        return totalPageCount;
    }
    catch (const Exception &e) {
        ShowMessage("Ошибка при определении количества страниц: " + e.Message);
        return 0;
    }
}

bool WordServer::IsLastPageSpecial() {
    try {
        // Получаем общее количество страниц
        const int wdStatisticPages = 2;
		int totalPageCount = activeDoc.OleFunction("ComputeStatistics", wdStatisticPages);

        // Если в документе только одна страница, считаем ее обычной
        if (totalPageCount <= 1) {
            return false; // Не считаем последнюю страницу особой
        }

        // Получаем диапазон последней страницы
        Variant lastPageRange = activeDoc.OleFunction("GoTo", 1, 1, totalPageCount);
        int lastPageStart = lastPageRange.OlePropertyGet("Start");
        int lastPageEnd = activeDoc.OlePropertyGet("Content").OlePropertyGet("End");

        // Создаем диапазон для последней страницы
        Variant lastPageContentRange = activeDoc.OleFunction("Range", lastPageStart, lastPageEnd);

        // Получаем текст последней страницы
        UnicodeString lastPageText = lastPageContentRange.OlePropertyGet("Text");

        // Нормализуем переносы строк
        lastPageText = StringReplace(lastPageText, "\r\n", "\n", TReplaceFlags() << rfReplaceAll);
        lastPageText = StringReplace(lastPageText, "\r", "\n", TReplaceFlags() << rfReplaceAll);

        // Разбиваем текст на строки
        TStringList* lines = new TStringList();
        try {
            lines->Text = lastPageText;

            int totalLines = lines->Count;

            if (totalLines == 0) {
                delete lines;
                return true; // Считаем последнюю страницу особой
            }

            int firstNonEmptyLine = -1;
            for (int i = 0; i < totalLines; ++i) {
                UnicodeString line = lines->Strings[i].Trim();
                if (!line.IsEmpty()) {
                    firstNonEmptyLine = i + 1;
                    break;
                }
            }

            if (firstNonEmptyLine == -1) {
                delete lines;
                return true; // Считаем страницу пустой и особой
            }

            double linePositionPercentage = (double)firstNonEmptyLine / (double)totalLines;

            delete lines;
            return linePositionPercentage >= 0.7; // Если текст начинается ниже 75% страницы, считаем ее особой

        } catch (...) {
            delete lines;
            throw;
        }
    }
    catch (const Exception &e) {
        ShowMessage("Ошибка при проверке последней страницы: " + e.Message);
        return false;
    }
}

void WordServer::ReplaceSpecialCharactersInList(TStringList *list)
{

	list->Text = ReplaceSpecialCharacters(list->Text);

}


UnicodeString WordServer::ReplaceSpecialCharacters(const UnicodeString &text)
{
	UnicodeString result;
	int len = text.Length();

    for (int i = 1; i <= len; i++) // Индексация UnicodeString начинается с 1
    {
        wchar_t ch = text[i];

        switch (ch)
        {
            case 0x0009: // Табуляция
                result += L"   "; // Заменяем на три пробела
                break;

            case 0x00A0: // Неразрывный пробел
            case 0x2007: // Фигурный пробел
            case 0x202F: // Узкий неразрывный пробел
                result += L' '; // Заменяем на обычный пробел
                break;

            case 0x00AD: // Мягкий перенос
                // Удаляем символ (ничего не добавляем)
                break;

            case 0x2011: // Неразрывный дефис
            case 0x2013: // Среднее тире
            case 0x2014: // Длинное тире
                result += L'-'; // Заменяем на дефис
                break;

            case 0x2028: // Разделитель строк
            case 0x2029: // Разделитель абзацев
            case 0x000B: // Вертикальная табуляция
				result += L' '; // Заменяем на перевод строки
                break;

            case L'«': // Левая треугольная кавычка
            case L'»': // Правая треугольная кавычка
				result += L'"'; // Заменяем на двойную кавычку
                break;

            case L'‘': // Левые одинарные кавычки
            case L'’': // Правые одинарные кавычки
				result += L'\''; // Заменяем на стандартные одинарные кавычки
                break;

			case L'“': // Левые двойные кавычки
            case L'”': // Правые двойные кавычки
				result += L'"'; // Заменяем на стандартные двойные кавычки
                break;

            default:
                // Проверяем, представим ли символ в кодировке OEM (CP866)
				if (!IsCharRepresentableInOEM(ch))
                {
                    result += L' '; // Если нельзя представить, заменяем на пробел
                }
                else
                {
                    result += ch; // Добавляем символ как есть
                }
                break;
        }
    }

    return result;
}




bool WordServer::IsCharRepresentableInOEM(wchar_t ch)
{
	// Исключение для символов кавычек, они всегда считаются представимыми
	if (ch == L'«' || ch == L'»' || ch == L'‘' || ch == L'’' || ch == L'“' || ch == L'”')
	{
		return true;
	}

	char oemChar;
	BOOL usedDefaultChar = FALSE;

	int res = WideCharToMultiByte(
		866,               // Кодовая страница CP866
		WC_NO_BEST_FIT_CHARS,
		&ch,
		1,                 // Один символ
        &oemChar,
		1,                 // Результат - один символ
		NULL,
        &usedDefaultChar
    );

    return res != 0 && !usedDefaultChar;
}


