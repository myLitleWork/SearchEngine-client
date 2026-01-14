#include "WordServer.h"
#include "WordServerFunc.hpp"
#include <filesystem>
#include <thread>
#include <ComObj.hpp>
#include <Vcl.Dialogs.hpp>
#include <mutex>
#include <Psapi.h>  // Для GetModuleBaseName
#pragma comment(lib, "Psapi.lib")  // Подключаем библиотеку
#include <tlhelp32.h>  // Для работы с процессами

namespace fs = std::filesystem;

WordServer::WordServer() {
    RemoveTemporaryFolder();
    CheckWordApp();  // Убедимся, что сервер Word инициализирован
}

WordServer::~WordServer() {
    try {
	   //	CloseAllDocuments();
		CloseTrackedProcesses();
	  //  if (!wordApp.IsEmpty()) {
	 //       wordApp.OleProcedure("Quit");
	  //  }
        RemoveTemporaryFolder();
    } catch (const Exception &e) {
        ShowMessage("Ошибка при завершении работы с Word: " + e.Message);
    }
}

Variant& WordServer::App()
{
    CheckWordApp();     // убедиться, что Word жив
    return wordApp;     // возвращаем ссылку – без лишних копий
}

void WordServer::CheckWordApp() {
    std::lock_guard<std::mutex> lock(mtx);

    try {
        if (wordApp.IsEmpty()) {
            wordApp = CreateOleObject("Word.Application");
			wordApp.OlePropertySet("Visible", false);

			DWORD processId = GetWordProcessId();
				if (processId != 0) {
					trackedPIDs.push_back(processId);  // Сохраняем PID
			}

		} else {
            // Проверяем доступность текущего экземпляра
            wordApp.OlePropertyGet("Name");
        }
    } catch (const Exception &e) {
        // Пересоздаем сервер Word при любой ошибке
        try {
			wordApp = CreateOleObject("Word.Application");
			wordApp.OlePropertySet("Visible", false);

			DWORD processId = GetWordProcessId();
				if (processId != 0) {
					trackedPIDs.push_back(processId);  // Сохраняем PID
			}


        } catch (const Exception &inner) {
            throw Exception("Ошибка при инициализации Word: " + inner.Message);
        }
    }
}
DWORD GetLatestWordProcessId() {
    DWORD wordProcessId = 0;
    DWORD latestTimestamp = 0;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &processEntry)) {
        do {
			if (_wcsicmp(processEntry.szExeFile, L"WINWORD.EXE") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processEntry.th32ProcessID);
                if (hProcess) {
                    FILETIME creationTime, exitTime, kernelTime, userTime;
                    if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                        DWORD creationTimestamp = (DWORD)creationTime.dwLowDateTime;
                        if (creationTimestamp > latestTimestamp) {
                            latestTimestamp = creationTimestamp;
                            wordProcessId = processEntry.th32ProcessID;
                        }
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }

    CloseHandle(hSnapshot);
    return wordProcessId;
}


bool WordServer::OpenDocumentInternal(const UnicodeString& fileName) {
    try {
        // Проверяем существование файла
        if (!fs::exists(fileName.c_str())) {
            throw Exception(L"Файл не существует: " + fileName);
        }

        // Убеждаемся, что приложение Word запущено
        CheckWordApp();

        // Создаем временную папку WordServer
        UnicodeString tempDir = UnicodeString(fs::temp_directory_path().u16string().c_str());
        UnicodeString wordServerDir = tempDir + L"\\WordServer";

        if (!fs::exists(wordServerDir.c_str())) {
            fs::create_directory(wordServerDir.c_str());
        }

        // Формируем временное имя файла
        UnicodeString tempFileName = wordServerDir + L"\\" + UnicodeString(fs::path(fileName.c_str()).filename().u16string().c_str());

        // Копируем файл в временную папку, с перезаписью, если он уже существует
        fs::copy_file(fileName.c_str(), tempFileName.c_str(), fs::copy_options::overwrite_existing);

        // Логгируем копирование файла
		OutputDebugStringW((L"Файл скопирован во временную папку: " + tempFileName).c_str());

        // Открываем документ в Word
        Variant documents = wordApp.OlePropertyGet("Documents");
        activeDoc = documents.OleFunction(
            "Open",
            tempFileName,       // Путь к временной копии файла
            Variant(false),     // Подтверждение преобразований (ConfirmConversions)
            Variant(true),      // Режим только для чтения (ReadOnly)
            Variant(false)      // Не добавлять в список недавних файлов (AddToRecentFiles)
        );

        // Асинхронное удаление временного файла после использования
        std::thread([tempFileName]() {
            try {
				std::this_thread::sleep_for(std::chrono::seconds(1)); // Задержка перед удалением
                if (fs::exists(tempFileName.c_str())) {
                    fs::remove(tempFileName.c_str());
					OutputDebugStringW((L"Временный файл удален: " + tempFileName).c_str());
                }
            } catch (const std::exception& e) {
				OutputDebugStringW((L"Ошибка удаления временного файла: " + UnicodeString(e.what())).c_str());
            }
        }).detach();

        return true; // Успешное открытие документа
    } catch (const Exception &e) {
        // Логируем ошибку
		OutputDebugStringW((L"Ошибка при открытии документа: " + e.Message).c_str());
        ShowMessage("Ошибка при открытии документа: " + e.Message);
        return false;
    }
}


UnicodeString WordServer::GetTextFromDoc(const UnicodeString& fileName) {
    sheet_count = 0;
    try {
        if (!OpenDocumentInternal(fileName)) {
            return "";
        }

        std::lock_guard<std::mutex> lock(mtx);

        UnicodeString text = activeDoc.OlePropertyGet("Content").OlePropertyGet("Text");
		sheet_count = GetPrintedSheetCount();

		if(IsLastPageSpecial())
            --sheet_count;

        activeDoc.OleProcedure("Close", Variant(false));
        return text;
    } catch (const Exception &e) {
        ShowMessage("Ошибка при извлечении текста: " + e.Message);
        return "";
    }
}

void WordServer::CloseAllDocuments() {
    std::lock_guard<std::mutex> lock(mtx);

    try {
        if (!wordApp.IsEmpty()) {
            Variant documents = wordApp.OlePropertyGet("Documents");
            int count = documents.OlePropertyGet("Count");

            for (int i = count; i >= 1; --i) {
                Variant doc = documents.OlePropertyGet("Item", i);
                doc.OleProcedure("Close", Variant(false));
            }
        }
    } catch (const Exception &e) {
        ShowMessage("Ошибка при закрытии документов: " + e.Message);
    }
}
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == *(DWORD*)lParam && IsWindowVisible(hwnd)) {
        *(HWND*)lParam = hwnd;  // Сохраняем найденное окно
        return FALSE;           // Прекращаем перечисление
    }
    return TRUE;                // Продолжаем перечисление
}

DWORD WordServer::GetWordProcessId() {
    DWORD wordProcessId = 0;
    return GetLatestWordProcessId();
	// Снимок всех процессов
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    // Перебираем процессы
	if (Process32First(hSnapshot, &processEntry)) {
		do {
			if (_wcsicmp(processEntry.szExeFile, L"WINWORD.EXE") == 0) {
				wordProcessId = processEntry.th32ProcessID;
                break;  // Найден процесс Word
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }
    CloseHandle(hSnapshot);
    return wordProcessId;
}

std::vector<DWORD> WordServer::GetTrackedProcesses() {
    return trackedPIDs;
}

#include <windows.h>
#include <tlhelp32.h>
#include <stdexcept>

void WordServer::CloseTrackedProcesses() {
	for (DWORD pid : trackedPIDs) {  // trackedPIDs содержит PID всех запущенных процессов Word
        try {
            // Попробуем открыть процесс
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pid);
            if (hProcess == nullptr) {
                // Если процесс не найден или доступ запрещен
                DWORD error = GetLastError();
                if (error == ERROR_ACCESS_DENIED) {
					OutputDebugStringW((L"Нет доступа к процессу Word с PID: " + std::to_wstring(pid)).c_str());
                } else if (error == ERROR_INVALID_PARAMETER) {
					OutputDebugStringW((L"Процесс Word с PID: " + std::to_wstring(pid) + L" не найден").c_str());
                }
                continue;
			}

            // Проверяем, действительно ли это процесс Word
            wchar_t processName[MAX_PATH] = {0};
			if (GetModuleBaseNameW(hProcess, nullptr, processName, MAX_PATH) > 0) {
                if (_wcsicmp(processName, L"WINWORD.EXE") != 0) {
                    // Это не Word, продолжаем
                    CloseHandle(hProcess);
					continue;
				}
            }

            // Пытаемся завершить процесс
            if (TerminateProcess(hProcess, 0)) {
				OutputDebugStringW((L"Процесс Word с PID: " + std::to_wstring(pid) + L" успешно завершен").c_str());
            } else {
				OutputDebugStringW((L"Не удалось завершить процесс Word с PID: " + std::to_wstring(pid)).c_str());
            }

			CloseHandle(hProcess);  // Закрываем дескриптор процесса
		} catch (const std::exception& e) {
			OutputDebugStringW((L"Исключение при завершении процесса Word с PID"));
		}
	}

    // Очищаем список PID, так как они больше не актуальны
    trackedPIDs.clear();
}

