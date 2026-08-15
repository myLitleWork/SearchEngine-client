#ifndef WORDBATCHPROCESSOR_H
#define WORDBATCHPROCESSOR_H
//-----------------------------------------------------------------
#include <System.Classes.hpp>      // TStringList
#include <System.SysUtils.hpp>     // Exception
#include <vcl.h>                   // WideString, ShowMessage
#include <atomic>
//-----------------------------------------------------------------
//  Внешняя функция перекодировки OEM‑866 → ANSI
extern void __fastcall F_SL_OemToAnsi(TStringList *sl);
//-----------------------------------------------------------------
/// Обёртка, открывающая Word один раз и закрывающая его в конце
#include "WordServer.h"      // вместо WordHost

enum class DocxConversionStatus
{
    Success,
    Failed,
    TimedOut,
    Cancelled
};

struct DocxConversionResult
{
    DocxConversionStatus status = DocxConversionStatus::Failed;
    String outputPath;
    String message;
    unsigned elapsedMs = 0;
};

/// Макрос + SaveAs после того, как worker перешёл в BUSY.
constexpr unsigned DOCX_CONVERSION_TIMEOUT_MS = 15000;
/// Cold start Word.Application до первого BUSY (отдельно от макроса).
constexpr unsigned DOCX_WORD_START_TIMEOUT_MS = 60000;

void ProcessPlainText_WS(WordServer& ws,
                         const String& plainText,
                         const String& macroName,
                         const String& outDocx,
                         const String& macroDoc = L"");

void ProcessRawFile_WS  (WordServer& ws,
                         const String& oemPath,
                         const String& macroName,
                         const String& outDocx,
                         const String& macroDoc = L"");

/// Долгоживущая DOCX-сессия: один worker-процесс и один Word на пакет.
/// При таймауте worker и его Word завершаются; следующий Convert() поднимет новые.
class DocxConversionSession
{
public:
    DocxConversionSession(const String& macroName, const String& macroDoc);
    ~DocxConversionSession();

    DocxConversionSession(const DocxConversionSession&) = delete;
    DocxConversionSession& operator=(const DocxConversionSession&) = delete;

    DocxConversionResult Convert(const String& plainText,
                                 const String& outDocx,
                                 unsigned timeoutMs = DOCX_CONVERSION_TIMEOUT_MS,
                                 const std::atomic_bool* cancelRequested = nullptr,
                                 unsigned wordStartTimeoutMs = DOCX_WORD_START_TIMEOUT_MS);

    void Close();

private:
    void EnsureStarted();
    void KillWorker(const String& reason);
    bool IsWorkerAlive() const;
    DocxConversionResult WaitForJobResponse(unsigned seq,
                                            const String& partialOutputPath,
                                            const String& outDocx,
                                            unsigned conversionTimeoutMs,
                                            unsigned wordStartTimeoutMs,
                                            ULONGLONG startedAt,
                                            const std::atomic_bool* cancelRequested);

    String macroName_;
    String macroDoc_;
    String sessionDir_;
    String statusPath_;
    HANDLE process_ = nullptr;
    HANDLE job_ = nullptr;
    unsigned nextSeq_ = 1;
};

DocxConversionResult ConvertPlainTextToDocxWithTimeout(
                         const String& plainText,
                         const String& macroName,
                         const String& outDocx,
                         const String& macroDoc,
                         unsigned timeoutMs = DOCX_CONVERSION_TIMEOUT_MS,
                         const std::atomic_bool* cancelRequested = nullptr,
                         unsigned wordStartTimeoutMs = DOCX_WORD_START_TIMEOUT_MS);

String GetDocxDiagnosticLogPath();

bool IsDocxWorkerInvocation();
int RunDocxWorkerFromCommandLine();

#ifdef _DEBUG
bool IsDocxTimeoutSelfTestInvocation();
int RunDocxTimeoutSelfTestFromCommandLine();
bool IsDocxCancelSelfTestInvocation();
int RunDocxCancelSelfTestFromCommandLine();
bool IsDocxRestartSelfTestInvocation();
int RunDocxRestartSelfTestFromCommandLine();
#endif

//-----------------------------------------------------------------
#endif // WORDBATCHPROCESSOR_H
