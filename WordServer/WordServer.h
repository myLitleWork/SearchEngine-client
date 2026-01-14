#ifndef WordServerH
#define WordServerH

#include <System.hpp>
#include <Variants.hpp>
#include <Classes.hpp>
#include <mutex>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>  // Для работы с процессами

class WordServer {
private:

	inline static WordServer* instance = nullptr;
    Variant wordApp;  // Описание приложения Word
	Variant activeDoc;  // Активный документ
	int sheet_count = 0;
	std::mutex mtx;
	bool last_special;
	std::vector<DWORD> trackedPIDs;  // Список PID запущенных процессов Word

    // Приватные методы для работы с документом Word
    void CheckWordApp();  // Проверка доступности приложения Word
	bool OpenDocumentInternal(const UnicodeString& fileName);  // Внутренний метод для открытия документа
	static bool IsCharRepresentableInOEM(wchar_t ch);

	WordServer();
	void CloseAllDocuments();
	int GetPrintedSheetCount();
	void RemoveTemporaryFolder();
	void CloseTrackedProcesses();
	DWORD GetWordProcessId();

	// Запрещаем копирование и перемещение экземпляра синглтона
	WordServer(const WordServer&) = delete;  // Запрещаем копирование
    WordServer& operator=(const WordServer&) = delete;  // Запрещаем присваивание
    WordServer(WordServer&&) = delete;  // Запрещаем перемещение
	WordServer& operator=(WordServer&&) = delete;

public:
	  // Конструктор
	~WordServer(); // Деструктор

    Variant& App();

	std::vector<DWORD> GetTrackedProcesses();

    static WordServer* getInstance() {
		if (instance == nullptr) {
            instance = new WordServer();
        }
        return instance;
	}


    static void Cleanup() {
        delete instance;
        instance = nullptr;
    }

    // Открытие документа для чтения
    bool OpenDocument(const UnicodeString& fileName);

    // Извлечение текста из документа
    UnicodeString GetTextFromDoc(const UnicodeString& fileName);

	// Получение количества напечатанных листов
	int GetPrintedSheetCount2() const {

		return sheet_count;
	}

	// Проверка, является ли последняя страница особенной
	bool IsLastPageSpecial();


	static UnicodeString ReplaceSpecialCharacters(const UnicodeString &text);
	static void ReplaceSpecialCharactersInList(TStringList *list);

};

#endif

