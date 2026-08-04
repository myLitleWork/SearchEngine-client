#include "WordBatchProcessor.h"

#include <System.IOUtils.hpp>
#include <System.Win.ComObj.hpp>
#include <Psapi.h>
#include <objbase.h>
#include <tlhelp32.h>
#include <windows.h>

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace
{
    constexpr int DOCX_WORKER_SUCCESS = 0;
    constexpr int DOCX_WORKER_BAD_ARGUMENTS = 20;
    constexpr int DOCX_WORKER_COM_ERROR = 21;
    constexpr int DOCX_WORKER_CONVERSION_ERROR = 22;
    constexpr int DOCX_WORKER_TIMEOUT_EXIT_CODE = 23;

    class ScopedHandle
    {
    public:
        explicit ScopedHandle(HANDLE value = nullptr) : value_(value) {}
        ~ScopedHandle() { reset(); }

        HANDLE get() const { return value_; }

        HANDLE release()
        {
            HANDLE value = value_;
            value_ = nullptr;
            return value;
        }

        void reset(HANDLE value = nullptr)
        {
            if (value_ && value_ != INVALID_HANDLE_VALUE)
                CloseHandle(value_);
            value_ = value;
        }

        ScopedHandle(const ScopedHandle&) = delete;
        ScopedHandle& operator=(const ScopedHandle&) = delete;

    private:
        HANDLE value_;
    };

    String FlattenStatusMessage(String message)
    {
        for (int i = 1; i <= message.Length(); ++i) {
            if (message[i] == L'\r' || message[i] == L'\n')
                message[i] = L' ';
        }
        return message;
    }

    void WriteUtf16File(const String& fileName, const String& text)
    {
        ScopedHandle file(CreateFileW(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (file.get() == INVALID_HANDLE_VALUE)
            throw Exception(L"Не удалось создать временный файл: " + SysErrorMessage(GetLastError()));

        const wchar_t bom = 0xFEFF;
        DWORD written = 0;
        if (!WriteFile(file.get(), &bom, sizeof(bom), &written, nullptr) || written != sizeof(bom))
            throw Exception(L"Не удалось записать BOM временного файла");

        const DWORD byteCount = static_cast<DWORD>(text.Length() * sizeof(wchar_t));
        if (byteCount != 0 &&
            (!WriteFile(file.get(), text.c_str(), byteCount, &written, nullptr) || written != byteCount)) {
            throw Exception(L"Не удалось записать временный файл");
        }
    }

    void WriteUtf16FileAtomic(const String& fileName, const String& text)
    {
        const String tempName = fileName + L".tmp";
        WriteUtf16File(tempName, text);
        if (!MoveFileExW(tempName.c_str(), fileName.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(tempName.c_str());
            throw Exception(L"Не удалось опубликовать файл: " + SysErrorMessage(GetLastError()));
        }
    }

    String ReadUtf16Text(const String& fileName)
    {
        std::unique_ptr<TStringList> lines(new TStringList());
        lines->LoadFromFile(fileName);
        return lines->Text;
    }

    void WriteWorkerStatus(const String& statusPath, DWORD wordPid,
                           bool ownedWordProcess, const String& state,
                           const String& message = L"")
    {
        String status = L"PID=" + String(static_cast<unsigned>(wordPid)) + L"\r\n";
        status += L"OWNED=" + String(ownedWordProcess ? 1 : 0) + L"\r\n";
        status += L"STATE=" + state + L"\r\n";
        status += L"MESSAGE=" + FlattenStatusMessage(message) + L"\r\n";
        WriteUtf16FileAtomic(statusPath, status);
    }

    String ReadStatusValue(const String& statusPath, const String& key)
    {
        if (!TFile::Exists(statusPath))
            return L"";

        try {
            std::unique_ptr<TStringList> lines(new TStringList());
            lines->LoadFromFile(statusPath);
            const String prefix = key + L"=";
            for (int i = 0; i < lines->Count; ++i) {
                const String line = lines->Strings[i];
                if (line.SubString(1, prefix.Length()) == prefix)
                    return line.SubString(prefix.Length() + 1, line.Length());
            }
        }
        catch (...) {
        }
        return L"";
    }

    std::set<DWORD> SnapshotWordProcessIds()
    {
        std::set<DWORD> result;
        ScopedHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (snapshot.get() == INVALID_HANDLE_VALUE)
            return result;

        PROCESSENTRY32W entry = {};
        entry.dwSize = sizeof(entry);
        if (!Process32FirstW(snapshot.get(), &entry))
            return result;

        do {
            if (_wcsicmp(entry.szExeFile, L"WINWORD.EXE") == 0)
                result.insert(entry.th32ProcessID);
        } while (Process32NextW(snapshot.get(), &entry));

        return result;
    }

    DWORD FindNewWordProcessId(const std::set<DWORD>& processIdsBefore)
    {
        const std::set<DWORD> processIdsAfter = SnapshotWordProcessIds();
        DWORD newProcessId = 0;
        for (DWORD processId : processIdsAfter) {
            if (processIdsBefore.find(processId) != processIdsBefore.end())
                continue;
            if (newProcessId != 0)
                return 0;
            newProcessId = processId;
        }
        return newProcessId;
    }

    void CloseDocumentOnly(Variant& document)
    {
        try {
            if (!document.IsEmpty())
                document.OleProcedure("Close", Variant(false));
        }
        catch (...) {
        }
        document = Unassigned();
    }

    void CloseWordAutomation(Variant& document, Variant& wordApp)
    {
        CloseDocumentOnly(document);

        try {
            if (!wordApp.IsEmpty())
                wordApp.OleProcedure("Quit", Variant(false));
        }
        catch (...) {
        }
        wordApp = Unassigned();
    }

    std::wstring QuoteCommandLineArgument(const String& value)
    {
        const std::wstring input(value.c_str());
        std::wstring result;
        result.push_back(L'"');

        std::size_t backslashes = 0;
        for (wchar_t ch : input) {
            if (ch == L'\\') {
                ++backslashes;
                continue;
            }

            if (ch == L'"') {
                result.append(backslashes * 2 + 1, L'\\');
                result.push_back(L'"');
                backslashes = 0;
                continue;
            }

            result.append(backslashes, L'\\');
            backslashes = 0;
            result.push_back(ch);
        }

        result.append(backslashes * 2, L'\\');
        result.push_back(L'"');
        return result;
    }

    String CreateTempDirectory()
    {
        wchar_t tempDirectory[MAX_PATH + 1] = {};
        if (GetTempPathW(MAX_PATH, tempDirectory) == 0)
            throw Exception(L"Не удалось получить TEMP: " + SysErrorMessage(GetLastError()));

        const String sessionDir = String(tempDirectory) + L"SCXDOCX-" +
            String(static_cast<unsigned>(GetCurrentProcessId())) + L"-" +
            String(static_cast<unsigned>(GetTickCount())) + L"\\";

        if (!CreateDirectoryW(sessionDir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
            throw Exception(L"Не удалось создать каталог сессии DOCX: " + SysErrorMessage(GetLastError()));

        return sessionDir;
    }

    String CreatePartialOutputPath(const String& outDocx)
    {
        const std::wstring suffix = L".part-" +
            std::to_wstring(static_cast<unsigned long>(GetCurrentProcessId())) + L"-" +
            std::to_wstring(static_cast<unsigned long>(GetTickCount())) + L".docx";
        return outDocx + String(suffix.c_str());
    }

    void TerminateOwnedWordFromStatus(const String& statusPath)
    {
        if (ReadStatusValue(statusPath, L"OWNED") != L"1")
            return;

        const DWORD processId = static_cast<DWORD>(
            StrToIntDef(ReadStatusValue(statusPath, L"PID"), 0));
        if (processId == 0)
            return;

        ScopedHandle process(OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE | SYNCHRONIZE,
            FALSE, processId));
        if (!process.get())
            return;

        wchar_t processName[MAX_PATH] = {};
        if (GetModuleBaseNameW(process.get(), nullptr, processName, MAX_PATH) == 0)
            return;
        if (_wcsicmp(processName, L"WINWORD.EXE") != 0)
            return;

        if (TerminateProcess(process.get(), DOCX_WORKER_TIMEOUT_EXIT_CODE))
            WaitForSingleObject(process.get(), 5000);
    }

    unsigned ElapsedMilliseconds(ULONGLONG startedAt)
    {
        const ULONGLONG elapsed = GetTickCount64() - startedAt;
        return elapsed > std::numeric_limits<unsigned>::max()
            ? std::numeric_limits<unsigned>::max()
            : static_cast<unsigned>(elapsed);
    }

    void DeleteFileIfExists(const String& path)
    {
        if (!path.IsEmpty() && TFile::Exists(path))
            DeleteFileW(path.c_str());
    }

    void EnsureWordStarted(Variant& wordApp, DWORD& wordProcessId, bool& ownedWordProcess,
                           const String& statusPath)
    {
        if (!wordApp.IsEmpty())
            return;

        const std::set<DWORD> wordProcessesBefore = SnapshotWordProcessIds();
        WriteWorkerStatus(statusPath, 0, false, L"STARTING_WORD");
        wordApp = CreateOleObject("Word.Application");
        wordApp.OlePropertySet("Visible", false);
        wordApp.OlePropertySet("DisplayAlerts", 0);
        wordApp.OlePropertySet("ScreenUpdating", false);

        wordProcessId = FindNewWordProcessId(wordProcessesBefore);
        ownedWordProcess = wordProcessId != 0 &&
            wordProcessesBefore.find(wordProcessId) == wordProcessesBefore.end();
        WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"READY");
    }

    void ProcessOneDocument(Variant& wordApp,
                            const String& inputPath,
                            const String& macroName,
                            const String& outputPath,
                            const String& macroDoc)
    {
        if (!TFile::Exists(inputPath))
            throw Exception(L"Временный файл с текстом не найден");
        if (!TFile::Exists(macroDoc))
            throw Exception(L"Шаблон макроса не найден: " + macroDoc);

        const String plainText = ReadUtf16Text(inputPath);

#ifdef _DEBUG
        wchar_t testDelayText[32] = {};
        if (GetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_MS",
                                    testDelayText, 32) != 0) {
            const int testDelayMs = StrToIntDef(String(testDelayText), 0);
            if (testDelayMs > 0)
                Sleep(static_cast<DWORD>(testDelayMs));
        }
#endif

        Variant document;
        try {
            Variant documents = wordApp.OlePropertyGet("Documents");
            document = documents.OleFunction("Open",
                WideString(macroDoc.c_str()), Variant(false), Variant(false), Variant(false));

            Variant range = document.OlePropertyGet("Content");
            range.OleFunction("InsertAfter", WideString(plainText.c_str()));
            wordApp.OleProcedure("Run", WideString(macroName.c_str()));
            document.OleProcedure("SaveAs2", WideString(outputPath.c_str()), 16);
            document.OleProcedure("Close", Variant(false));
            document = Unassigned();
        }
        catch (...) {
            CloseDocumentOnly(document);
            throw;
        }
    }

    void WriteJobResponse(const String& responsePath, unsigned seq,
                          bool ok, const String& message)
    {
        String text = L"SEQ=" + String(seq) + L"\r\n";
        text += L"STATUS=" + String(ok ? L"OK" : L"ERROR") + L"\r\n";
        text += L"MESSAGE=" + FlattenStatusMessage(message) + L"\r\n";
        WriteUtf16FileAtomic(responsePath, text);
    }
}

bool IsDocxWorkerInvocation()
{
    return ParamCount() >= 1 && SameText(ParamStr(1), L"--docx-worker");
}

#ifdef _DEBUG
bool IsDocxTimeoutSelfTestInvocation()
{
    return ParamCount() >= 1 && SameText(ParamStr(1), L"--docx-timeout-self-test");
}
#endif

int RunDocxWorkerFromCommandLine()
{
    // Долгоживущий режим:
    //   --docx-worker <sessionDir> <macroName> <macroDoc>
    if (ParamCount() != 4)
        return DOCX_WORKER_BAD_ARGUMENTS;

    const String sessionDir = IncludeTrailingPathDelimiter(ParamStr(2));
    const String macroName = ParamStr(3);
    const String macroDoc = ParamStr(4);
    const String statusPath = sessionDir + L"status";
    const String requestPath = sessionDir + L"job.req";
    const String responsePath = sessionDir + L"job.rsp";
    const String shutdownPath = sessionDir + L"shutdown";

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult)) {
        try {
            WriteWorkerStatus(statusPath, 0, false, L"ERROR",
                L"Не удалось инициализировать COM");
        }
        catch (...) {
        }
        return DOCX_WORKER_COM_ERROR;
    }

    Variant wordApp;
    DWORD wordProcessId = 0;
    bool ownedWordProcess = false;

    try {
        if (!TFile::Exists(macroDoc))
            throw Exception(L"Шаблон макроса не найден: " + macroDoc);

        WriteWorkerStatus(statusPath, 0, false, L"READY");

        while (!TFile::Exists(shutdownPath)) {
            if (!TFile::Exists(requestPath)) {
                Sleep(40);
                continue;
            }

            String inputPath;
            String outputPath;
            unsigned seq = 0;
            try {
                std::unique_ptr<TStringList> lines(new TStringList());
                lines->LoadFromFile(requestPath);
                for (int i = 0; i < lines->Count; ++i) {
                    const String line = lines->Strings[i];
                    if (line.SubString(1, 4) == L"SEQ=")
                        seq = static_cast<unsigned>(StrToIntDef(line.SubString(5, line.Length()), 0));
                    else if (line.SubString(1, 6) == L"INPUT=")
                        inputPath = line.SubString(7, line.Length());
                    else if (line.SubString(1, 7) == L"OUTPUT=")
                        outputPath = line.SubString(8, line.Length());
                }

                if (seq == 0 || inputPath.IsEmpty() || outputPath.IsEmpty())
                    throw Exception(L"Некорректный запрос DOCX worker");

                EnsureWordStarted(wordApp, wordProcessId, ownedWordProcess, statusPath);
                WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"BUSY");
                ProcessOneDocument(wordApp, inputPath, macroName, outputPath, macroDoc);
                WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"READY");
                WriteJobResponse(responsePath, seq, true, L"");
            }
            catch (const Exception& e) {
                try {
                    WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess,
                        L"READY", e.Message);
                    WriteJobResponse(responsePath, seq == 0 ? 1 : seq, false, e.Message);
                }
                catch (...) {
                }
            }
            catch (const std::exception& e) {
                try {
                    WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess,
                        L"READY", String(e.what()));
                    WriteJobResponse(responsePath, seq == 0 ? 1 : seq, false, String(e.what()));
                }
                catch (...) {
                }
            }
            catch (...) {
                try {
                    WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess,
                        L"READY", L"Неизвестная ошибка Word");
                    WriteJobResponse(responsePath, seq == 0 ? 1 : seq, false,
                        L"Неизвестная ошибка Word");
                }
                catch (...) {
                }
            }

            DeleteFileIfExists(requestPath);
        }

        Variant emptyDoc;
        CloseWordAutomation(emptyDoc, wordApp);
        WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"DONE");
        CoUninitialize();
        return DOCX_WORKER_SUCCESS;
    }
    catch (const Exception& e) {
        try {
            WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"ERROR", e.Message);
        }
        catch (...) {
        }
    }
    catch (...) {
        try {
            WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess,
                L"ERROR", L"Неизвестная ошибка Word");
        }
        catch (...) {
        }
    }

    {
        Variant emptyDoc;
        CloseWordAutomation(emptyDoc, wordApp);
    }
    CoUninitialize();
    return DOCX_WORKER_CONVERSION_ERROR;
}

DocxConversionSession::DocxConversionSession(const String& macroName, const String& macroDoc)
    : macroName_(macroName)
    , macroDoc_(macroDoc)
{
}

DocxConversionSession::~DocxConversionSession()
{
    Close();
}

void DocxConversionSession::KillWorker()
{
    if (job_) {
        TerminateJobObject(job_, DOCX_WORKER_TIMEOUT_EXIT_CODE);
    }
    else if (process_) {
        TerminateProcess(process_, DOCX_WORKER_TIMEOUT_EXIT_CODE);
    }

    if (process_) {
        WaitForSingleObject(process_, 5000);
        CloseHandle(process_);
        process_ = nullptr;
    }

    if (job_) {
        CloseHandle(job_);
        job_ = nullptr;
    }

    if (!statusPath_.IsEmpty())
        TerminateOwnedWordFromStatus(statusPath_);
}

bool DocxConversionSession::IsWorkerAlive() const
{
    if (!process_)
        return false;

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(process_, &exitCode))
        return false;
    return exitCode == STILL_ACTIVE;
}

void DocxConversionSession::EnsureStarted()
{
    if (IsWorkerAlive())
        return;

    KillWorker();

    if (!TFile::Exists(macroDoc_))
        throw Exception(L"Шаблон макроса не найден: " + macroDoc_);

    if (sessionDir_.IsEmpty())
        sessionDir_ = CreateTempDirectory();
    else {
        ForceDirectories(sessionDir_);
    }

    statusPath_ = sessionDir_ + L"status";
    DeleteFileIfExists(statusPath_);
    DeleteFileIfExists(sessionDir_ + L"job.req");
    DeleteFileIfExists(sessionDir_ + L"job.rsp");
    DeleteFileIfExists(sessionDir_ + L"shutdown");

    const String executablePath = ParamStr(0);
    std::wstring commandLine = QuoteCommandLineArgument(executablePath);
    commandLine += L" --docx-worker ";
    commandLine += QuoteCommandLineArgument(sessionDir_);
    commandLine += L" ";
    commandLine += QuoteCommandLineArgument(macroName_);
    commandLine += L" ";
    commandLine += QuoteCommandLineArgument(macroDoc_);

    std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
    commandBuffer.push_back(L'\0');

    ScopedHandle job(CreateJobObjectW(nullptr, nullptr));
    if (!job.get())
        throw Exception(L"Не удалось создать Job Object: " + SysErrorMessage(GetLastError()));

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation,
                                 &jobInfo, sizeof(jobInfo))) {
        throw Exception(L"Не удалось настроить Job Object: " + SysErrorMessage(GetLastError()));
    }

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessW(executablePath.c_str(), commandBuffer.data(), nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, nullptr,
                        &startupInfo, &processInfo)) {
        throw Exception(L"Не удалось запустить DOCX worker: " + SysErrorMessage(GetLastError()));
    }

    ScopedHandle process(processInfo.hProcess);
    ScopedHandle thread(processInfo.hThread);

    if (!AssignProcessToJobObject(job.get(), process.get())) {
        const DWORD assignError = GetLastError();
        TerminateProcess(process.get(), DOCX_WORKER_CONVERSION_ERROR);
        WaitForSingleObject(process.get(), 5000);
        throw Exception(L"Не удалось изолировать DOCX worker: " + SysErrorMessage(assignError));
    }

    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) {
        const DWORD resumeError = GetLastError();
        TerminateJobObject(job.get(), DOCX_WORKER_CONVERSION_ERROR);
        WaitForSingleObject(process.get(), 5000);
        throw Exception(L"Не удалось запустить поток DOCX worker: " + SysErrorMessage(resumeError));
    }

    process_ = process.release();
    job_ = job.release();

    const ULONGLONG readyStarted = GetTickCount64();
    while (ElapsedMilliseconds(readyStarted) < 15000) {
        if (!IsWorkerAlive())
            throw Exception(L"DOCX worker завершился при старте");

        const String state = ReadStatusValue(statusPath_, L"STATE");
        if (state == L"READY" || state == L"BUSY")
            return;
        if (state == L"ERROR") {
            String message = ReadStatusValue(statusPath_, L"MESSAGE");
            if (message.IsEmpty())
                message = L"DOCX worker не смог инициализироваться";
            throw Exception(message);
        }
        Sleep(40);
    }

    KillWorker();
    throw Exception(L"DOCX worker не сообщил о готовности");
}

DocxConversionResult DocxConversionSession::WaitForJobResponse(
    unsigned seq,
    const String& partialOutputPath,
    const String& outDocx,
    unsigned timeoutMs,
    ULONGLONG startedAt)
{
    DocxConversionResult result;
    result.outputPath = outDocx;
    const String responsePath = sessionDir_ + L"job.rsp";

    while (ElapsedMilliseconds(startedAt) < timeoutMs) {
        if (!IsWorkerAlive()) {
            result.message = ReadStatusValue(statusPath_, L"MESSAGE");
            if (result.message.IsEmpty())
                result.message = L"DOCX worker неожиданно завершился";
            result.elapsedMs = ElapsedMilliseconds(startedAt);
            KillWorker();
            return result;
        }

        if (TFile::Exists(responsePath)) {
            const unsigned responseSeq = static_cast<unsigned>(
                StrToIntDef(ReadStatusValue(responsePath, L"SEQ"), 0));
            if (responseSeq == seq) {
                const String status = ReadStatusValue(responsePath, L"STATUS");
                const String responseMessage = ReadStatusValue(responsePath, L"MESSAGE");
                DeleteFileIfExists(responsePath);

                if (status != L"OK") {
                    result.message = responseMessage;
                    if (result.message.IsEmpty())
                        result.message = ReadStatusValue(statusPath_, L"MESSAGE");
                    if (result.message.IsEmpty())
                        result.message = L"Ошибка конвертации DOCX";
                    result.elapsedMs = ElapsedMilliseconds(startedAt);
                    return result;
                }

                if (!TFile::Exists(partialOutputPath)) {
                    result.message = L"Word не создал выходной DOCX";
                    result.elapsedMs = ElapsedMilliseconds(startedAt);
                    return result;
                }

                if (!MoveFileExW(partialOutputPath.c_str(), outDocx.c_str(),
                                 MOVEFILE_WRITE_THROUGH)) {
                    result.message = L"Не удалось завершить сохранение DOCX: " +
                        SysErrorMessage(GetLastError());
                    result.elapsedMs = ElapsedMilliseconds(startedAt);
                    DeleteFileIfExists(partialOutputPath);
                    return result;
                }

                result.status = DocxConversionStatus::Success;
                result.message = L"DOCX создан";
                result.elapsedMs = ElapsedMilliseconds(startedAt);
                return result;
            }
        }

        Sleep(40);
    }

    KillWorker();
    DeleteFileIfExists(partialOutputPath);
    DeleteFileIfExists(sessionDir_ + L"job.req");
    DeleteFileIfExists(responsePath);

    result.status = DocxConversionStatus::TimedOut;
    result.message = L"Word/макрос не завершился за " +
        String(timeoutMs / 1000) + L" секунд";
    result.elapsedMs = ElapsedMilliseconds(startedAt);
    return result;
}

DocxConversionResult DocxConversionSession::Convert(
    const String& plainText,
    const String& outDocx,
    unsigned timeoutMs)
{
    const ULONGLONG startedAt = GetTickCount64();
    DocxConversionResult result;
    result.outputPath = outDocx;

    String inputPath;
    String partialOutputPath;
    try {
        EnsureStarted();

        inputPath = sessionDir_ + L"input-" + String(nextSeq_) + L".txt";
        partialOutputPath = CreatePartialOutputPath(outDocx);
        const String requestPath = sessionDir_ + L"job.req";
        const String responsePath = sessionDir_ + L"job.rsp";

        DeleteFileIfExists(responsePath);
        DeleteFileIfExists(partialOutputPath);
        WriteUtf16File(inputPath, plainText);

        const unsigned seq = nextSeq_++;
        String request = L"SEQ=" + String(seq) + L"\r\n";
        request += L"INPUT=" + inputPath + L"\r\n";
        request += L"OUTPUT=" + partialOutputPath + L"\r\n";
        WriteUtf16FileAtomic(requestPath, request);

        result = WaitForJobResponse(seq, partialOutputPath, outDocx, timeoutMs, startedAt);
        DeleteFileIfExists(inputPath);
        if (result.status != DocxConversionStatus::Success)
            DeleteFileIfExists(partialOutputPath);
        return result;
    }
    catch (const Exception& e) {
        result.message = e.Message;
    }
    catch (const std::exception& e) {
        result.message = String(e.what());
    }
    catch (...) {
        result.message = L"Неизвестная ошибка DOCX-сессии";
    }

    DeleteFileIfExists(inputPath);
    DeleteFileIfExists(partialOutputPath);
    result.elapsedMs = ElapsedMilliseconds(startedAt);
    return result;
}

void DocxConversionSession::Close()
{
    if (!sessionDir_.IsEmpty() && IsWorkerAlive()) {
        try {
            WriteUtf16FileAtomic(sessionDir_ + L"shutdown", L"1\r\n");
        }
        catch (...) {
        }

        if (process_)
            WaitForSingleObject(process_, 8000);
    }

    if (IsWorkerAlive())
        KillWorker();
    else {
        if (process_) {
            CloseHandle(process_);
            process_ = nullptr;
        }
        if (job_) {
            CloseHandle(job_);
            job_ = nullptr;
        }
        if (!statusPath_.IsEmpty())
            TerminateOwnedWordFromStatus(statusPath_);
    }

    if (!sessionDir_.IsEmpty()) {
        try {
            TDirectory::Delete(sessionDir_, true);
        }
        catch (...) {
        }
        sessionDir_ = L"";
        statusPath_ = L"";
    }
}

DocxConversionResult ConvertPlainTextToDocxWithTimeout(
                         const String& plainText,
                         const String& macroName,
                         const String& outDocx,
                         const String& macroDoc,
                         unsigned timeoutMs)
{
    DocxConversionSession session(macroName, macroDoc);
    return session.Convert(plainText, outDocx, timeoutMs);
}

#ifdef _DEBUG
int RunDocxTimeoutSelfTestFromCommandLine()
{
    if (ParamCount() != 6)
        return 30;

    const String inputPath = ParamStr(2);
    const String macroDoc = ParamStr(3);
    const String outputPath = ParamStr(4);
    const unsigned timeoutMs = static_cast<unsigned>(StrToIntDef(ParamStr(5), 0));
    const unsigned delayMs = static_cast<unsigned>(StrToIntDef(ParamStr(6), 0));
    if (!TFile::Exists(inputPath) || !TFile::Exists(macroDoc) ||
        timeoutMs == 0 || delayMs <= timeoutMs) {
        return 31;
    }

    SetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_MS",
        String(delayMs).c_str());
    const DocxConversionResult result = ConvertPlainTextToDocxWithTimeout(
        ReadUtf16Text(inputPath), L"Обработка_ШТ_2022",
        outputPath, macroDoc, timeoutMs);
    SetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_MS", nullptr);

    if (result.status != DocxConversionStatus::TimedOut)
        return 32;
    if (TFile::Exists(outputPath))
        return 33;
    return 0;
}
#endif
