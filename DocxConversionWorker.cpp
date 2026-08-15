#include "WordBatchProcessor.h"

#include <System.IOUtils.hpp>
#include <System.Win.ComObj.hpp>
#include <Psapi.h>
#include <objbase.h>
#include <tlhelp32.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cwchar>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <thread>
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

    class ScopedFindHandle
    {
    public:
        explicit ScopedFindHandle(HANDLE value) : value_(value) {}
        ~ScopedFindHandle()
        {
            if (value_ != INVALID_HANDLE_VALUE)
                FindClose(value_);
        }
        HANDLE get() const { return value_; }

        ScopedFindHandle(const ScopedFindHandle&) = delete;
        ScopedFindHandle& operator=(const ScopedFindHandle&) = delete;

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

    void AppendDocxLog(const String& role, const String& message) noexcept
    {
        try {
            const String logPath = GetDocxDiagnosticLogPath();
            ScopedHandle file(CreateFileW(logPath.c_str(), FILE_APPEND_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
            if (file.get() == INVALID_HANDLE_VALUE)
                return;

            SYSTEMTIME now = {};
            GetLocalTime(&now);
            wchar_t prefix[160] = {};
            std::swprintf(prefix, sizeof(prefix) / sizeof(prefix[0]),
                L"%04u-%02u-%02u %02u:%02u:%02u.%03u pid=%lu role=%ls ",
                now.wYear, now.wMonth, now.wDay,
                now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
                static_cast<unsigned long>(GetCurrentProcessId()), role.c_str());

            const String record = String(prefix) + FlattenStatusMessage(message) + L"\r\n";
            const UTF8String utf8(record);
            DWORD written = 0;
            WriteFile(file.get(), utf8.c_str(), static_cast<DWORD>(utf8.Length()),
                      &written, nullptr);
        }
        catch (...) {
        }
    }

    String Win32Failure(const String& operation, DWORD error)
    {
        return operation + L" failed, error=" + String(static_cast<unsigned>(error)) +
            L" (" + SysErrorMessage(error) + L")";
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

    String SerializeProcessIds(const std::set<DWORD>& processIds)
    {
        String result;
        for (DWORD processId : processIds)
            result += String(static_cast<unsigned>(processId)) + L"\r\n";
        return result;
    }

    std::set<DWORD> ReadProcessIds(const String& fileName)
    {
        std::set<DWORD> result;
        if (!TFile::Exists(fileName))
            return result;

        try {
            std::unique_ptr<TStringList> lines(new TStringList());
            lines->LoadFromFile(fileName);
            for (int i = 0; i < lines->Count; ++i) {
                const DWORD processId = static_cast<DWORD>(
                    StrToIntDef(Trim(lines->Strings[i]), 0));
                if (processId != 0)
                    result.insert(processId);
            }
        }
        catch (...) {
        }
        return result;
    }

    bool QueryProcessImagePath(HANDLE process, String& imagePath, DWORD& error)
    {
        std::vector<wchar_t> buffer(32768, L'\0');
        DWORD length = static_cast<DWORD>(buffer.size());
        if (!QueryFullProcessImageNameW(process, 0, buffer.data(), &length)) {
            error = GetLastError();
            return false;
        }
        imagePath = String(buffer.data(), static_cast<int>(length));
        error = ERROR_SUCCESS;
        return true;
    }

    bool IsSameProcessSession(DWORD processId)
    {
        DWORD currentSession = 0;
        DWORD processSession = 0;
        return ProcessIdToSessionId(GetCurrentProcessId(), &currentSession) &&
               ProcessIdToSessionId(processId, &processSession) &&
               currentSession == processSession;
    }

    struct VisibleWindowProbe
    {
        DWORD processId = 0;
        bool found = false;
    };

    BOOL CALLBACK FindVisibleProcessWindow(HWND window, LPARAM value)
    {
        auto* probe = reinterpret_cast<VisibleWindowProbe*>(value);
        DWORD processId = 0;
        GetWindowThreadProcessId(window, &processId);
        if (processId == probe->processId && IsWindowVisible(window)) {
            probe->found = true;
            return FALSE;
        }
        return TRUE;
    }

    bool HasVisibleProcessWindow(DWORD processId)
    {
        VisibleWindowProbe probe;
        probe.processId = processId;
        EnumWindows(FindVisibleProcessWindow, reinterpret_cast<LPARAM>(&probe));
        return probe.found;
    }

    DWORD GetWordProcessIdFromApplication(Variant& wordApp)
    {
        try {
            const Variant wordWindow = wordApp.OlePropertyGet("Hwnd");
            const LONG_PTR rawWindow = static_cast<LONG_PTR>(
                static_cast<__int64>(wordWindow));
            if (rawWindow == 0)
                return 0;

            DWORD processId = 0;
            GetWindowThreadProcessId(reinterpret_cast<HWND>(rawWindow), &processId);
            return processId;
        }
        catch (...) {
            return 0;
        }
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

    bool TerminateWordProcessSafely(DWORD processId, const String& reason)
    {
        if (processId == 0)
            return true;

        ScopedHandle process(OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
            FALSE, processId));
        if (!process.get()) {
            const DWORD error = GetLastError();
            const std::set<DWORD> runningWordProcesses = SnapshotWordProcessIds();
            if (error == ERROR_INVALID_PARAMETER ||
                runningWordProcesses.find(processId) == runningWordProcesses.end()) {
                AppendDocxLog(L"parent", L"Word pid=" +
                    String(static_cast<unsigned>(processId)) + L" already exited; " + reason);
                return true;
            }
            AppendDocxLog(L"parent", Win32Failure(
                L"OpenProcess Word pid=" + String(static_cast<unsigned>(processId)), error));
            return false;
        }

        if (!IsSameProcessSession(processId)) {
            AppendDocxLog(L"parent", L"refused Word termination for pid=" +
                String(static_cast<unsigned>(processId)) + L": different session; " + reason);
            return false;
        }

        String imagePath;
        DWORD queryError = ERROR_SUCCESS;
        if (!QueryProcessImagePath(process.get(), imagePath, queryError)) {
            AppendDocxLog(L"parent", Win32Failure(
                L"QueryFullProcessImageName Word pid=" +
                String(static_cast<unsigned>(processId)), queryError));
            return false;
        }
        if (!SameText(ExtractFileName(imagePath), L"WINWORD.EXE")) {
            AppendDocxLog(L"parent", L"refused termination for pid=" +
                String(static_cast<unsigned>(processId)) + L": image=" + imagePath);
            return false;
        }

        if (!TerminateProcess(process.get(), DOCX_WORKER_TIMEOUT_EXIT_CODE)) {
            AppendDocxLog(L"parent", Win32Failure(
                L"TerminateProcess Word pid=" + String(static_cast<unsigned>(processId)),
                GetLastError()));
            return false;
        }

        const DWORD waitResult = WaitForSingleObject(process.get(), 5000);
        if (waitResult != WAIT_OBJECT_0) {
            AppendDocxLog(L"parent", L"Word pid=" +
                String(static_cast<unsigned>(processId)) + L" did not exit, wait=" +
                String(static_cast<unsigned>(waitResult)) + L"; " + reason);
            return false;
        }

        AppendDocxLog(L"parent", L"terminated owned Word pid=" +
            String(static_cast<unsigned>(processId)) + L"; " + reason);
        return true;
    }

    std::vector<DWORD> FindConservativeWordCandidates(
        const std::set<DWORD>& processIdsBefore)
    {
        std::vector<DWORD> result;
        const std::set<DWORD> processIdsAfter = SnapshotWordProcessIds();
        for (DWORD processId : processIdsAfter) {
            if (processIdsBefore.find(processId) != processIdsBefore.end())
                continue;
            if (!IsSameProcessSession(processId) || HasVisibleProcessWindow(processId))
                continue;
            result.push_back(processId);
        }
        return result;
    }

    bool TerminateOwnedWordFromStatus(const String& statusPath, const String& reason)
    {
        const DWORD processId = static_cast<DWORD>(
            StrToIntDef(ReadStatusValue(statusPath, L"PID"), 0));
        const bool owned = ReadStatusValue(statusPath, L"OWNED") == L"1";
        if (owned && processId != 0)
            return TerminateWordProcessSafely(processId, reason);

        // A non-owned PID belongs to a Word instance that existed before this
        // automation session. Never terminate it.
        if (processId != 0)
            return true;

        const String state = ReadStatusValue(statusPath, L"STATE");
        const String baselinePath = ExtractFilePath(statusPath) + L"word.before";
        if ((state != L"STARTING_WORD" && state != L"CONFIGURING_WORD" &&
             state != L"BUSY") ||
            !TFile::Exists(baselinePath)) {
            return true;
        }

        const std::set<DWORD> processIdsBefore = ReadProcessIds(baselinePath);
        for (int attempt = 0; attempt < 25; ++attempt) {
            const std::vector<DWORD> candidates =
                FindConservativeWordCandidates(processIdsBefore);
            if (candidates.size() == 1) {
                AppendDocxLog(L"parent", L"using conservative startup fallback for Word pid=" +
                    String(static_cast<unsigned>(candidates.front())) + L"; " + reason);
                return TerminateWordProcessSafely(candidates.front(), reason +
                    L"; startup fallback");
            }
            if (candidates.size() > 1) {
                AppendDocxLog(L"parent", L"refused startup fallback: " +
                    String(static_cast<unsigned>(candidates.size())) +
                    L" new hidden WINWORD processes are ambiguous; " + reason);
                return false;
            }
            Sleep(100);
        }

        AppendDocxLog(L"parent", L"no owned Word process found after worker stop; " + reason);
        return true;
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

    void ApplyDebugDocxDelay(const String& stage)
    {
#ifdef _DEBUG
        wchar_t delayText[32] = {};
        if (GetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_MS",
                                    delayText, 32) == 0) {
            return;
        }

        wchar_t configuredStage[64] = {};
        String expectedStage = L"WORD_READY";
        if (GetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_STAGE",
                                    configuredStage, 64) != 0) {
            expectedStage = configuredStage;
        }
        if (!SameText(expectedStage, stage))
            return;

        const int delayMs = StrToIntDef(String(delayText), 0);
        if (delayMs > 0) {
            AppendDocxLog(L"worker", L"debug delay stage=" + stage +
                L" ms=" + String(delayMs));
            Sleep(static_cast<DWORD>(delayMs));
        }
#else
        (void)stage;
#endif
    }

    void EnsureWordStarted(Variant& wordApp, DWORD& wordProcessId, bool& ownedWordProcess,
                           const String& statusPath)
    {
        if (!wordApp.IsEmpty())
            return;

        const std::set<DWORD> wordProcessesBefore = SnapshotWordProcessIds();
        WriteUtf16FileAtomic(ExtractFilePath(statusPath) + L"word.before",
                             SerializeProcessIds(wordProcessesBefore));
        WriteWorkerStatus(statusPath, 0, false, L"STARTING_WORD");
        AppendDocxLog(L"worker", L"starting Word; baseline count=" +
            String(static_cast<unsigned>(wordProcessesBefore.size())));
        ApplyDebugDocxDelay(L"WORD_START");
        wordApp = CreateOleObject("Word.Application");

        wordProcessId = GetWordProcessIdFromApplication(wordApp);
        if (wordProcessId == 0)
            wordProcessId = FindNewWordProcessId(wordProcessesBefore);
        ownedWordProcess = wordProcessId != 0 &&
            wordProcessesBefore.find(wordProcessId) == wordProcessesBefore.end();
        WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"CONFIGURING_WORD");

        wordApp.OlePropertySet("Visible", false);
        wordApp.OlePropertySet("DisplayAlerts", 0);
        wordApp.OlePropertySet("ScreenUpdating", false);

        WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"READY");
        AppendDocxLog(L"worker", L"Word ready pid=" +
            String(static_cast<unsigned>(wordProcessId)) + L" owned=" +
            String(ownedWordProcess ? 1 : 0));
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

        ApplyDebugDocxDelay(L"WORD_READY");

        Variant document;
        try {
            Variant documents = wordApp.OlePropertyGet("Documents");
            document = documents.OleFunction("Open",
                WideString(macroDoc.c_str()), Variant(false), Variant(false), Variant(false));

            Variant range = document.OlePropertyGet("Content");
            range.OleFunction("InsertAfter", WideString(plainText.c_str()));
            ApplyDebugDocxDelay(L"RUN_MACRO");
            wordApp.OleProcedure("Run", WideString(macroName.c_str()));
            ApplyDebugDocxDelay(L"SAVE_DOCX");
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

String GetDocxDiagnosticLogPath()
{
    std::vector<wchar_t> localAppData(32768, L'\0');
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", localAppData.data(), static_cast<DWORD>(localAppData.size()));

    String basePath;
    if (length > 0 && length < localAppData.size())
        basePath = String(localAppData.data());
    else
        basePath = ExcludeTrailingPathDelimiter(TPath::GetTempPath());

    const String logDirectory = IncludeTrailingPathDelimiter(basePath) +
        L"SearchClient_asio\\Logs";
    try {
        if (ForceDirectories(logDirectory) || TDirectory::Exists(logDirectory))
            return IncludeTrailingPathDelimiter(logDirectory) + L"docx-worker.log";
    }
    catch (...) {
    }
    return IncludeTrailingPathDelimiter(TPath::GetTempPath()) +
        L"SearchClient_asio-docx-worker.log";
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

bool IsDocxCancelSelfTestInvocation()
{
    return ParamCount() >= 1 && SameText(ParamStr(1), L"--docx-cancel-self-test");
}

bool IsDocxRestartSelfTestInvocation()
{
    return ParamCount() >= 1 && SameText(ParamStr(1), L"--docx-restart-self-test");
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
        AppendDocxLog(L"worker", L"CoInitializeEx failed hresult=" +
            String(static_cast<int>(comResult)));
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

        AppendDocxLog(L"worker", L"worker ready, macro=" + macroName);
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

                AppendDocxLog(L"worker", L"job seq=" + String(seq) + L" started");
                EnsureWordStarted(wordApp, wordProcessId, ownedWordProcess, statusPath);
                WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"BUSY");
                ProcessOneDocument(wordApp, inputPath, macroName, outputPath, macroDoc);
                WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"READY");
                WriteJobResponse(responsePath, seq, true, L"");
                AppendDocxLog(L"worker", L"job seq=" + String(seq) + L" succeeded");
            }
            catch (const Exception& e) {
                AppendDocxLog(L"worker", L"job seq=" + String(seq) +
                    L" failed: " + e.Message);
                try {
                    WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess,
                        L"READY", e.Message);
                    WriteJobResponse(responsePath, seq == 0 ? 1 : seq, false, e.Message);
                }
                catch (...) {
                }
            }
            catch (const std::exception& e) {
                AppendDocxLog(L"worker", L"job seq=" + String(seq) +
                    L" failed: " + String(e.what()));
                try {
                    WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess,
                        L"READY", String(e.what()));
                    WriteJobResponse(responsePath, seq == 0 ? 1 : seq, false, String(e.what()));
                }
                catch (...) {
                }
            }
            catch (...) {
                AppendDocxLog(L"worker", L"job seq=" + String(seq) +
                    L" failed: unknown Word error");
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
        AppendDocxLog(L"worker", L"worker shutdown completed");
        CoUninitialize();
        return DOCX_WORKER_SUCCESS;
    }
    catch (const Exception& e) {
        AppendDocxLog(L"worker", L"worker fatal error: " + e.Message);
        try {
            WriteWorkerStatus(statusPath, wordProcessId, ownedWordProcess, L"ERROR", e.Message);
        }
        catch (...) {
        }
    }
    catch (...) {
        AppendDocxLog(L"worker", L"worker fatal unknown error");
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

void DocxConversionSession::KillWorker(const String& reason)
{
    if (!process_ && !job_ &&
        (statusPath_.IsEmpty() || !TFile::Exists(statusPath_))) {
        return;
    }

    const DWORD workerPid = process_ ? GetProcessId(process_) : 0;
    AppendDocxLog(L"parent", L"stopping worker pid=" +
        String(static_cast<unsigned>(workerPid)) + L"; " + reason);

    if (job_) {
        if (!TerminateJobObject(job_, DOCX_WORKER_TIMEOUT_EXIT_CODE))
            AppendDocxLog(L"parent", Win32Failure(L"TerminateJobObject", GetLastError()));
    }
    else if (process_) {
        if (!TerminateProcess(process_, DOCX_WORKER_TIMEOUT_EXIT_CODE))
            AppendDocxLog(L"parent", Win32Failure(L"TerminateProcess worker", GetLastError()));
    }

    if (process_) {
        DWORD waitResult = WaitForSingleObject(process_, 5000);
        if (waitResult == WAIT_TIMEOUT) {
            AppendDocxLog(L"parent", L"worker did not exit after job termination; forcing pid=" +
                String(static_cast<unsigned>(workerPid)));
            TerminateProcess(process_, DOCX_WORKER_TIMEOUT_EXIT_CODE);
            waitResult = WaitForSingleObject(process_, 5000);
        }
        if (waitResult != WAIT_OBJECT_0)
            AppendDocxLog(L"parent", L"worker wait failed, result=" +
                String(static_cast<unsigned>(waitResult)));
        CloseHandle(process_);
        process_ = nullptr;
    }

    if (job_) {
        CloseHandle(job_);
        job_ = nullptr;
    }

    if (!statusPath_.IsEmpty()) {
        TerminateOwnedWordFromStatus(statusPath_, reason);
        DeleteFileIfExists(statusPath_);
        DeleteFileIfExists(ExtractFilePath(statusPath_) + L"word.before");
    }
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

    KillWorker(L"restart stale worker");

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
    DeleteFileIfExists(sessionDir_ + L"word.before");

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
    AppendDocxLog(L"parent", L"worker created pid=" +
        String(static_cast<unsigned>(GetProcessId(process_))) + L" session=" + sessionDir_);

    const ULONGLONG readyStarted = GetTickCount64();
    while (ElapsedMilliseconds(readyStarted) < 15000) {
        if (!IsWorkerAlive()) {
            KillWorker(L"worker exited during startup");
            throw Exception(L"DOCX worker завершился при старте");
        }

        const String state = ReadStatusValue(statusPath_, L"STATE");
        if (state == L"READY" || state == L"BUSY")
            return;
        if (state == L"ERROR") {
            String message = ReadStatusValue(statusPath_, L"MESSAGE");
            if (message.IsEmpty())
                message = L"DOCX worker не смог инициализироваться";
            KillWorker(L"worker reported startup error");
            throw Exception(message);
        }
        Sleep(40);
    }

    KillWorker(L"worker readiness timeout");
    throw Exception(L"DOCX worker не сообщил о готовности");
}

DocxConversionResult DocxConversionSession::WaitForJobResponse(
    unsigned seq,
    const String& partialOutputPath,
    const String& outDocx,
    unsigned conversionTimeoutMs,
    unsigned wordStartTimeoutMs,
    ULONGLONG startedAt,
    const std::atomic_bool* cancelRequested)
{
    DocxConversionResult result;
    result.outputPath = outDocx;
    const String responsePath = sessionDir_ + L"job.rsp";
    const ULONGLONG jobSubmittedAt = GetTickCount64();
    bool conversionPhase = false;
    ULONGLONG conversionPhaseStartedAt = 0;

    while (true) {
        if (cancelRequested && cancelRequested->load(std::memory_order_acquire)) {
            KillWorker(L"job seq=" + String(seq) + L" cancelled");
            DeleteFileIfExists(partialOutputPath);
            DeleteFileIfExists(sessionDir_ + L"job.req");
            DeleteFileIfExists(responsePath);
            result.status = DocxConversionStatus::Cancelled;
            result.message = L"Сохранение DOCX отменено пользователем";
            result.elapsedMs = ElapsedMilliseconds(startedAt);
            AppendDocxLog(L"parent", L"job seq=" + String(seq) + L" cancelled");
            return result;
        }

        if (!IsWorkerAlive()) {
            result.message = ReadStatusValue(statusPath_, L"MESSAGE");
            if (result.message.IsEmpty())
                result.message = L"DOCX worker неожиданно завершился";
            result.elapsedMs = ElapsedMilliseconds(startedAt);
            KillWorker(L"job seq=" + String(seq) + L" worker exited");
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
                    AppendDocxLog(L"parent", L"job seq=" + String(seq) +
                        L" returned error: " + result.message);
                    KillWorker(L"job seq=" + String(seq) + L" conversion error");
                    return result;
                }

                if (!TFile::Exists(partialOutputPath)) {
                    result.message = L"Word не создал выходной DOCX";
                    result.elapsedMs = ElapsedMilliseconds(startedAt);
                    KillWorker(L"job seq=" + String(seq) + L" produced no output");
                    return result;
                }

                if (!MoveFileExW(partialOutputPath.c_str(), outDocx.c_str(),
                                 MOVEFILE_WRITE_THROUGH)) {
                    result.message = L"Не удалось завершить сохранение DOCX: " +
                        SysErrorMessage(GetLastError());
                    result.elapsedMs = ElapsedMilliseconds(startedAt);
                    DeleteFileIfExists(partialOutputPath);
                    KillWorker(L"job seq=" + String(seq) + L" output publish failed");
                    return result;
                }

                result.status = DocxConversionStatus::Success;
                result.message = L"DOCX создан";
                result.elapsedMs = ElapsedMilliseconds(startedAt);
                AppendDocxLog(L"parent", L"job seq=" + String(seq) +
                    L" succeeded elapsedMs=" + String(result.elapsedMs));
                return result;
            }
        }

        const String state = ReadStatusValue(statusPath_, L"STATE");
        if (!conversionPhase) {
            if (state == L"BUSY") {
                conversionPhase = true;
                conversionPhaseStartedAt = GetTickCount64();
                AppendDocxLog(L"parent", L"job seq=" + String(seq) +
                    L" entered conversion phase after wordStartMs=" +
                    String(ElapsedMilliseconds(jobSubmittedAt)));
            }
            else if (ElapsedMilliseconds(jobSubmittedAt) >= wordStartTimeoutMs) {
                KillWorker(L"job seq=" + String(seq) + L" word-start timeout");
                DeleteFileIfExists(partialOutputPath);
                DeleteFileIfExists(sessionDir_ + L"job.req");
                DeleteFileIfExists(responsePath);

                result.status = DocxConversionStatus::TimedOut;
                result.message = L"Word не запустился за " +
                    String(wordStartTimeoutMs / 1000) + L" секунд";
                result.elapsedMs = ElapsedMilliseconds(startedAt);
                AppendDocxLog(L"parent", L"job seq=" + String(seq) +
                    L" word-start timed out elapsedMs=" + String(result.elapsedMs) +
                    L" state=" + state);
                return result;
            }
        }
        else if (ElapsedMilliseconds(conversionPhaseStartedAt) >= conversionTimeoutMs) {
            KillWorker(L"job seq=" + String(seq) + L" conversion timeout");
            DeleteFileIfExists(partialOutputPath);
            DeleteFileIfExists(sessionDir_ + L"job.req");
            DeleteFileIfExists(responsePath);

            result.status = DocxConversionStatus::TimedOut;
            result.message = L"Макрос/SaveAs не завершился за " +
                String(conversionTimeoutMs / 1000) + L" секунд";
            result.elapsedMs = ElapsedMilliseconds(startedAt);
            AppendDocxLog(L"parent", L"job seq=" + String(seq) +
                L" conversion timed out elapsedMs=" + String(result.elapsedMs));
            return result;
        }

        Sleep(40);
    }
}

DocxConversionResult DocxConversionSession::Convert(
    const String& plainText,
    const String& outDocx,
    unsigned timeoutMs,
    const std::atomic_bool* cancelRequested,
    unsigned wordStartTimeoutMs)
{
    const ULONGLONG startedAt = GetTickCount64();
    DocxConversionResult result;
    result.outputPath = outDocx;

    String inputPath;
    String partialOutputPath;
    try {
        if (cancelRequested && cancelRequested->load(std::memory_order_acquire)) {
            result.status = DocxConversionStatus::Cancelled;
            result.message = L"Сохранение DOCX отменено пользователем";
            return result;
        }

        EnsureStarted();

        if (cancelRequested && cancelRequested->load(std::memory_order_acquire)) {
            KillWorker(L"cancelled before DOCX job submission");
            result.status = DocxConversionStatus::Cancelled;
            result.message = L"Сохранение DOCX отменено пользователем";
            result.elapsedMs = ElapsedMilliseconds(startedAt);
            return result;
        }

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
        AppendDocxLog(L"parent", L"job seq=" + String(seq) +
            L" submitted conversionTimeoutMs=" + String(timeoutMs) +
            L" wordStartTimeoutMs=" + String(wordStartTimeoutMs));

        result = WaitForJobResponse(seq, partialOutputPath, outDocx, timeoutMs,
                                    wordStartTimeoutMs, startedAt, cancelRequested);
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

    if (cancelRequested && cancelRequested->load(std::memory_order_acquire)) {
        result.status = DocxConversionStatus::Cancelled;
        result.message = L"Сохранение DOCX отменено пользователем";
    }
    KillWorker(L"DOCX session exception: " + result.message);
    DeleteFileIfExists(inputPath);
    DeleteFileIfExists(partialOutputPath);
    if (!sessionDir_.IsEmpty()) {
        DeleteFileIfExists(sessionDir_ + L"job.req");
        DeleteFileIfExists(sessionDir_ + L"job.rsp");
    }
    result.elapsedMs = ElapsedMilliseconds(startedAt);
    AppendDocxLog(L"parent", L"conversion failed: " + result.message);
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
        KillWorker(L"graceful shutdown timeout");
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
            TerminateOwnedWordFromStatus(statusPath_, L"normal session close");
    }

    if (!sessionDir_.IsEmpty()) {
        try {
            TDirectory::Delete(sessionDir_, true);
        }
        catch (const Exception& e) {
            AppendDocxLog(L"parent", L"failed to delete session directory: " + e.Message);
        }
        catch (...) {
            AppendDocxLog(L"parent", L"failed to delete session directory: unknown error");
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
                         unsigned timeoutMs,
                         const std::atomic_bool* cancelRequested,
                         unsigned wordStartTimeoutMs)
{
    DocxConversionSession session(macroName, macroDoc);
    return session.Convert(plainText, outDocx, timeoutMs, cancelRequested,
                           wordStartTimeoutMs);
}

#ifdef _DEBUG
namespace
{
    bool IsSupportedTestStage(const String& stage)
    {
        return SameText(stage, L"WORD_START") ||
               SameText(stage, L"WORD_READY") ||
               SameText(stage, L"RUN_MACRO") ||
               SameText(stage, L"SAVE_DOCX");
    }

    class ScopedDocxTestDelay
    {
    public:
        ScopedDocxTestDelay(unsigned delayMs, const String& stage)
        {
            SetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_MS",
                String(delayMs).c_str());
            SetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_STAGE",
                stage.c_str());
        }

        ~ScopedDocxTestDelay()
        {
            SetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_MS", nullptr);
            SetEnvironmentVariableW(L"SEARCHCLIENT_DOCX_TEST_DELAY_STAGE", nullptr);
        }

        ScopedDocxTestDelay(const ScopedDocxTestDelay&) = delete;
        ScopedDocxTestDelay& operator=(const ScopedDocxTestDelay&) = delete;
    };

    std::set<std::wstring> SnapshotCurrentProcessDocxDirectories()
    {
        std::set<std::wstring> result;
        const String tempPath = IncludeTrailingPathDelimiter(TPath::GetTempPath());
        const String pattern = tempPath + L"SCXDOCX-" +
            String(static_cast<unsigned>(GetCurrentProcessId())) + L"-*";

        WIN32_FIND_DATAW findData = {};
        ScopedFindHandle search(FindFirstFileW(pattern.c_str(), &findData));
        if (search.get() == INVALID_HANDLE_VALUE)
            return result;

        do {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                wcscmp(findData.cFileName, L".") != 0 &&
                wcscmp(findData.cFileName, L"..") != 0) {
                result.insert(std::wstring((tempPath + findData.cFileName).c_str()));
            }
        } while (FindNextFileW(search.get(), &findData));
        return result;
    }

    std::vector<String> FindPartialOutputs(const String& outputPath)
    {
        std::vector<String> result;
        const String pattern = outputPath + L".part-*.docx";
        WIN32_FIND_DATAW findData = {};
        ScopedFindHandle search(FindFirstFileW(pattern.c_str(), &findData));
        if (search.get() == INVALID_HANDLE_VALUE)
            return result;

        const String directory = ExtractFilePath(outputPath);
        do {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                result.push_back(directory + findData.cFileName);
        } while (FindNextFileW(search.get(), &findData));
        return result;
    }

    int ValidateDocxSelfTestCleanup(const std::set<DWORD>& wordBefore,
                                    const std::set<std::wstring>& tempBefore,
                                    const std::vector<String>& outputPaths,
                                    int errorBase)
    {
        Sleep(1500);
        const std::set<DWORD> wordAfter = SnapshotWordProcessIds();
        for (DWORD processId : wordBefore) {
            if (wordAfter.find(processId) == wordAfter.end())
                return errorBase + 1; // a pre-existing Word process was terminated
        }

        // With a pre-existing Word instance, Office can legitimately create an
        // additional WINWORD process while the test is running.  It is not safe
        // for a validator to terminate a process that the worker did not record
        // as its own.  The clean-baseline tests remain strict and catch leaks.
        if (wordBefore.empty() && !wordAfter.empty())
            return errorBase + 2;

        const std::set<std::wstring> tempAfter = SnapshotCurrentProcessDocxDirectories();
        for (const std::wstring& directory : tempAfter) {
            if (tempBefore.find(directory) == tempBefore.end())
                return errorBase + 3;
        }

        for (const String& outputPath : outputPaths) {
            if (TFile::Exists(outputPath))
                return errorBase + 4;
            const std::vector<String> partialOutputs = FindPartialOutputs(outputPath);
            if (!partialOutputs.empty()) {
                for (const String& partialOutput : partialOutputs)
                    DeleteFileIfExists(partialOutput);
                return errorBase + 5;
            }
        }
        return 0;
    }
}

int RunDocxTimeoutSelfTestFromCommandLine()
{
    if (ParamCount() != 6 && ParamCount() != 7)
        return 30;

    const String inputPath = ParamStr(2);
    const String macroDoc = ParamStr(3);
    const String outputPath = ParamStr(4);
    const unsigned timeoutMs = static_cast<unsigned>(StrToIntDef(ParamStr(5), 0));
    const unsigned delayMs = static_cast<unsigned>(StrToIntDef(ParamStr(6), 0));
    const String stage = ParamCount() == 7 ? ParamStr(7) : L"WORD_READY";
    if (!TFile::Exists(inputPath) || !TFile::Exists(macroDoc) ||
        TFile::Exists(outputPath) || timeoutMs == 0 || delayMs <= timeoutMs ||
        !IsSupportedTestStage(stage)) {
        return 31;
    }

    const bool wordStartStage = SameText(stage, L"WORD_START");
    const unsigned conversionTimeoutMs = wordStartStage
        ? DOCX_CONVERSION_TIMEOUT_MS
        : timeoutMs;
    const unsigned wordStartTimeoutMs = wordStartStage
        ? timeoutMs
        : DOCX_WORD_START_TIMEOUT_MS;

    const std::set<DWORD> wordBefore = SnapshotWordProcessIds();
    const std::set<std::wstring> tempBefore = SnapshotCurrentProcessDocxDirectories();
    DocxConversionResult result;
    {
        ScopedDocxTestDelay delay(delayMs, stage);
        result = ConvertPlainTextToDocxWithTimeout(
            ReadUtf16Text(inputPath), L"Обработка_ШТ_2022",
            outputPath, macroDoc, conversionTimeoutMs, nullptr, wordStartTimeoutMs);
    }

    if (result.status != DocxConversionStatus::TimedOut)
        return 32;
    return ValidateDocxSelfTestCleanup(
        wordBefore, tempBefore, std::vector<String>{outputPath}, 32);
}

int RunDocxCancelSelfTestFromCommandLine()
{
    if (ParamCount() != 7 && ParamCount() != 8)
        return 40;

    const String inputPath = ParamStr(2);
    const String macroDoc = ParamStr(3);
    const String outputPath = ParamStr(4);
    const unsigned timeoutMs = static_cast<unsigned>(StrToIntDef(ParamStr(5), 0));
    const unsigned delayMs = static_cast<unsigned>(StrToIntDef(ParamStr(6), 0));
    const unsigned cancelAfterMs = static_cast<unsigned>(StrToIntDef(ParamStr(7), 0));
    const String stage = ParamCount() == 8 ? ParamStr(8) : L"WORD_READY";
    if (!TFile::Exists(inputPath) || !TFile::Exists(macroDoc) ||
        TFile::Exists(outputPath) || timeoutMs <= cancelAfterMs ||
        delayMs <= cancelAfterMs || cancelAfterMs == 0 ||
        !IsSupportedTestStage(stage)) {
        return 41;
    }

    const bool wordStartStage = SameText(stage, L"WORD_START");
    const unsigned conversionTimeoutMs = wordStartStage
        ? DOCX_CONVERSION_TIMEOUT_MS
        : timeoutMs;
    const unsigned wordStartTimeoutMs = wordStartStage
        ? timeoutMs
        : DOCX_WORD_START_TIMEOUT_MS;

    const std::set<DWORD> wordBefore = SnapshotWordProcessIds();
    const std::set<std::wstring> tempBefore = SnapshotCurrentProcessDocxDirectories();
    std::atomic_bool cancelRequested{false};
    DocxConversionResult result;
    {
        ScopedDocxTestDelay delay(delayMs, stage);
        std::thread cancelThread([&cancelRequested, cancelAfterMs]() {
            Sleep(cancelAfterMs);
            cancelRequested.store(true, std::memory_order_release);
        });
        DocxConversionSession session(L"Обработка_ШТ_2022", macroDoc);
        result = session.Convert(ReadUtf16Text(inputPath), outputPath,
                                 conversionTimeoutMs, &cancelRequested,
                                 wordStartTimeoutMs);
        cancelThread.join();
        session.Close();
    }

    if (result.status != DocxConversionStatus::Cancelled)
        return 42;
    return ValidateDocxSelfTestCleanup(
        wordBefore, tempBefore, std::vector<String>{outputPath}, 42);
}

int RunDocxRestartSelfTestFromCommandLine()
{
    if (ParamCount() != 6 && ParamCount() != 7)
        return 50;

    const String inputPath = ParamStr(2);
    const String macroDoc = ParamStr(3);
    const String firstOutputPath = ParamStr(4);
    const String secondOutputPath = ChangeFileExt(firstOutputPath, L".second.docx");
    const unsigned timeoutMs = static_cast<unsigned>(StrToIntDef(ParamStr(5), 0));
    const unsigned delayMs = static_cast<unsigned>(StrToIntDef(ParamStr(6), 0));
    const String stage = ParamCount() == 7 ? ParamStr(7) : L"WORD_READY";
    if (!TFile::Exists(inputPath) || !TFile::Exists(macroDoc) ||
        TFile::Exists(firstOutputPath) || TFile::Exists(secondOutputPath) ||
        timeoutMs == 0 || delayMs <= timeoutMs || !IsSupportedTestStage(stage)) {
        return 51;
    }

    const bool wordStartStage = SameText(stage, L"WORD_START");
    const unsigned conversionTimeoutMs = wordStartStage
        ? DOCX_CONVERSION_TIMEOUT_MS
        : timeoutMs;
    const unsigned wordStartTimeoutMs = wordStartStage
        ? timeoutMs
        : DOCX_WORD_START_TIMEOUT_MS;

    const std::set<DWORD> wordBefore = SnapshotWordProcessIds();
    const std::set<std::wstring> tempBefore = SnapshotCurrentProcessDocxDirectories();
    DocxConversionResult firstResult;
    DocxConversionResult secondResult;
    {
        ScopedDocxTestDelay delay(delayMs, stage);
        DocxConversionSession session(L"Обработка_ШТ_2022", macroDoc);
        firstResult = session.Convert(
            ReadUtf16Text(inputPath), firstOutputPath, conversionTimeoutMs,
            nullptr, wordStartTimeoutMs);
        secondResult = session.Convert(
            ReadUtf16Text(inputPath), secondOutputPath, conversionTimeoutMs,
            nullptr, wordStartTimeoutMs);
        session.Close();
    }

    if (firstResult.status != DocxConversionStatus::TimedOut)
        return 52;
    if (secondResult.status != DocxConversionStatus::TimedOut)
        return 53;
    return ValidateDocxSelfTestCleanup(
        wordBefore, tempBefore,
        std::vector<String>{firstOutputPath, secondOutputPath}, 53);
}
#endif
