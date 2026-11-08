#pragma once
#include <System.SysUtils.hpp>

namespace search_limit {
inline String unlimitedText() { return L"Без лимита"; }
// Zero means no LIMIT only after this parser has explicitly accepted the label.
inline bool parse(const String& text, int& value) {
    if (SameText(Trim(text), unlimitedText())) { value = 0; return true; }
    return TryStrToInt(Trim(text), value) && value > 0;
}
}
