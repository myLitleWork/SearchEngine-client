#include "WordBatchProcessor.h"
#include <System.Win.ComObj.hpp>
#include <System.IOUtils.hpp>

static Variant GetDocs(WordServer& ws)
{
    return ws.App().OlePropertyGet("Documents");
}

// ───────────────────────────────────────────────────────────
void ProcessPlainText_WS(WordServer& ws,
                         const String& plainText,
                         const String& macroName,
                         const String& outDocx,
                         const String& macroDoc)
{
    Variant docs = GetDocs(ws);                    // app.Documents
    Variant doc  = macroDoc.Length()
                 ? docs.OleFunction("Open", WideString(macroDoc.c_str()))
                 : docs.OleFunction("Add");

    Variant rng = doc.OlePropertyGet("Content");
    rng.OleFunction("InsertAfter", WideString(plainText.c_str()));

    ws.App().OleProcedure("Run", WideString(macroName.c_str()));  // макрос

    doc.OleProcedure("SaveAs2", WideString(outDocx.c_str()), 16); // wdFormatDefault
    doc.OleProcedure("Close", false);
}

// ───────────────────────────────────────────────────────────
void ProcessRawFile_WS(WordServer& ws,
                       const String& oemPath,
                       const String& macroName,
                       const String& outDocx,
                       const String& macroDoc)
{
    if (!TFile::Exists(oemPath))
        throw Exception("Файл не найден: " + oemPath);

    std::unique_ptr<TStringList> sl(new TStringList);
    sl->LoadFromFile(oemPath);          // OEM-866 байты
    F_SL_OemToAnsi(sl.get());           // перекодировка

    String text;
    for (int i = 0; i < sl->Count; ++i)
        text += sl->Strings[i] + "\r\n";

    ProcessPlainText_WS(ws, text, macroName, outDocx, macroDoc);
}

