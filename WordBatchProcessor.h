#ifndef WORDBATCHPROCESSOR_H
#define WORDBATCHPROCESSOR_H
//-----------------------------------------------------------------
#include <System.Classes.hpp>      // TStringList
#include <System.SysUtils.hpp>     // Exception
#include <vcl.h>                   // WideString, ShowMessage
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
    TimedOut
};

struct DocxConversionResult
{
    DocxConversionStatus status = DocxConversionStatus::Failed;
    String outputPath;
    String message;
    unsigned elapsedMs = 0;
};

constexpr unsigned DOCX_CONVERSION_TIMEOUT_MS = 60000;

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
                                 unsigned timeoutMs = DOCX_CONVERSION_TIMEOUT_MS);

    void Close();

private:
    void EnsureStarted();
    void KillWorker();
    bool IsWorkerAlive() const;
    DocxConversionResult WaitForJobResponse(unsigned seq,
                                            const String& partialOutputPath,
                                            const String& outDocx,
                                            unsigned timeoutMs,
                                            ULONGLONG startedAt);

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
                         unsigned timeoutMs = DOCX_CONVERSION_TIMEOUT_MS);

bool IsDocxWorkerInvocation();
int RunDocxWorkerFromCommandLine();

#ifdef _DEBUG
bool IsDocxTimeoutSelfTestInvocation();
int RunDocxTimeoutSelfTestFromCommandLine();
#endif

//-----------------------------------------------------------------
#endif // WORDBATCHPROCESSOR_H
