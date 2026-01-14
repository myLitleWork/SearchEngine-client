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

//-----------------------------------------------------------------
#endif // WORDBATCHPROCESSOR_H

