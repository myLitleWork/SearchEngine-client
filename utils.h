#ifndef utilsH
#define utilsH

#include <Data.DB.hpp>
#include <string>
#include <Winapi.Windows.hpp>   // OemToAnsi, MultiByteToWideChar
#include <codecvt>
#include <vector>
#include <locale>
#include <Vcl.StdCtrls.hpp>
#include <System.AnsiStrings.hpp>
#include <System.hpp>

namespace conv
{
    inline std::string to_string(const std::wstring& wstr)
    {
        // потокобезопасный вариант с локальным конвертером
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.to_bytes(wstr);
    }

    inline std::wstring to_wstring(const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.from_bytes(str);
    }
}

// Net/JSON diagnostics are UTF-8 in std::string. UTF8String(c_str()) / String(c_str())
// treat those bytes as the system ANSI code page (CP1251) and produce mojibake in VCL.
// Some older client-side err literals are still CP1251 — fall back to CP_ACP if UTF-8 fails.
inline UnicodeString Utf8ToVcl(const std::string& s)
{
	if (s.empty())
		return UnicodeString();
	const int len = static_cast<int>(s.size());
	UINT codePage = CP_UTF8;
	DWORD flags = WC_ERR_INVALID_CHARS;
	int n = MultiByteToWideChar(codePage, flags, s.data(), len, nullptr, 0);
	if (n <= 0) {
		codePage = CP_ACP;
		flags = 0;
		n = MultiByteToWideChar(codePage, flags, s.data(), len, nullptr, 0);
		if (n <= 0)
			return UnicodeString();
	}
	std::wstring wide(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(codePage, flags, s.data(), len, &wide[0], n);
	return UnicodeString(wide.c_str());
}

inline const wchar_t* deletedFileMessage()
{
	return L"Файл удалён с диска. Доступен только след в индексе поиска.";
}

inline bool isDeletedRow(TDataSet* ds)
{
	if (!ds || !ds->Active)
		return false;
	TField* f = ds->FindField("deleted");
	if (!f || f->IsNull)
		return false;
	if (f->DataType == ftBoolean)
		return f->AsBoolean;
	return f->AsInteger != 0;
}

namespace my_utils
{
    inline AnsiString toAnsi(const std::string& s)
    {
        return AnsiString(s.c_str());
    }

    inline void F_SL_OemToAnsi(TStringList* reg)
    {
        for (int j = 0; j < reg->Count; j++)
        {
            AnsiString s = reg->Strings[j];

            // создаём временный буфер
            std::string tmp = s.c_str();
            std::vector<char> buf(tmp.begin(), tmp.end());
			buf.push_back('\0');

            // перевод из OEM866 → ANSI
			OemToAnsi(buf.data(), buf.data());

            // обновляем строку и приводим к верхнему регистру
            s = AnsiString(buf.data()).UpperCase();
            reg->Strings[j] = s;
        }
    }
}

#endif

