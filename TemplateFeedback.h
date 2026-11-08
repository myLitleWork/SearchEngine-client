#pragma once
#include "my_json/Request.h"
#include "utils.h"
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <map>
#include <richedit.h>

namespace template_feedback {
inline String searchField(const String& name) {
    if (name == L"Подписной №") return L"Подписной номер";
    if (name == L"Вх/исх №") return L"Входящий или исходящий номер";
    if (name == L"Кр. содерж.") return L"Краткое содержание";
    if (name == L"Подп./исп.") return L"Подписывающее лицо или исполнитель";
    if (name == L"Тел. №") return L"Телеграфный номер";
    return name;
}
inline String columnName(const std::string& key, const std::map<std::string,String>& captions) {
    const auto found = captions.find(key);
    if (found != captions.end() && !found->second.IsEmpty()) {
        String name = StringReplace(found->second, L"\r", L" ", TReplaceFlags() << rfReplaceAll);
        return StringReplace(name, L"\n", L" ", TReplaceFlags() << rfReplaceAll);
    }
    const std::map<std::string,String> known = {
        {"num",L"Входящий или исходящий номер"},{"podp_num",L"Подписной номер"},
        {"date",L"Дата вх./исх."},{"date_podp",L"Дата подписи"},{"tel_num",L"Телеграфный номер"},
        {"kr",L"Краткое содержание"},{"from_to",L"Куда / откуда"},{"isp",L"Подписывающее лицо / исполнитель"}
    };
    const auto label = known.find(key);
    return label == known.end() ? L"Колонка «" + Utf8ToVcl(key) + L"»" : label->second;
}

inline String describe(const String& group, const String& name, const Request& value,
    const String& path, bool saved, bool replaced, bool filtersOmitted,
    const std::map<std::string,String>& captions = {}, bool userFileExists = true) {
    String text = saved ? String(L"Шаблон успешно сохранён.") : String(L"Шаблон выбран. Его параметры применены к поиску.");
    text += L"\r\n\r\nНазвание: «" + name + L"»\r\nГруппа: «" + group + L"»";
    if (saved && replaced) text += L"\r\nПрежний шаблон с этим названием в этой группе обновлён.";
    text += saved ? L"\r\n\r\nКуда сохранён:\r\n" :
        (userFileExists ? L"\r\n\r\nФайл с вашими шаблонами:\r\n" : L"\r\n\r\nЭто готовый шаблон из комплекта программы. При сохранении он попадёт в файл:\r\n");
    text += path;
    text += saved ? L"\r\n\r\nСохранённые параметры:" : L"\r\n\r\nПараметры шаблона:";
    if (value.sql) {
        text += L"\r\n• Где искать: в карточках AutoPad.";
        text += L"\r\n• Какие документы: " + String(value.type == 0 ? L"входящие." : L"исходящие.");
        if (value.field == "SQL") {
            if (Trim(Utf8ToVcl(value.search_text)) == L"1=1" || value.search_text.empty())
                text += L"\r\n• Что искать: все документы, подходящие под остальные сохранённые условия.";
            else text += L"\r\n• Дополнительное условие поиска:\r\n" + Utf8ToVcl(value.search_text);
        } else {
            text += L"\r\n• В каком поле: " + searchField(Utf8ToVcl(value.field)) + L".";
            text += L"\r\n• Что искать: «" + Utf8ToVcl(value.search_text) + L"».";
            text += value.exc ? L"\r\n• Совпадение: всё значение поля должно совпасть с запросом." :
                L"\r\n• Совпадение: поле должно содержать указанный текст.";
        }
    } else {
        text += L"\r\n• Где искать: в тексте документов доступных источников.";
        text += L"\r\n• Что искать: «" + Utf8ToVcl(value.search_text) + L"».";
    }
    if (value.template_version >= 2) {
        if (value.sql) {
            const wchar_t* periods[] = {L"все даты, ограничение по дате не задано",L"сегодня",L"вчера",L"сегодня и предыдущие 6 дней",L"сегодня и предыдущие 29 дней",L"с первого числа текущего месяца по сегодня"};
            const int period = value.period >= 0 && value.period <= 5 ? value.period : 0;
            text += L"\r\n• За какой период: " + String(periods[period]) + L".";
            text += L"\r\n• Поле даты: " + String(value.date_field == "DataPodp" ? L"дата подписи документа в AutoPad." : L"дата регистрации входящего / исходящего в AutoPad.");
            if (value.unlimited_results) text += L"\r\n• Сколько показать: все подходящие документы, без ограничения количества. При долгом поиске можно продолжить ожидание или отменить его.";
            else if (value.max_results > 0) text += L"\r\n• Сколько показать: не более " + IntToStr(value.max_results) + L" результатов от каждого сервера.";
            text += L"\r\n• Порядок результатов: " + String(value.newest_first ? L"сначала новые по выбранной дате." : L"стандартная сортировка программы.");
            if (period > 0) text += L"\r\nДаты периода будут вычисляться заново при каждом поиске по часам этого компьютера.";
        }
        if (value.column_filters.empty()) text += L"\r\n• Фильтры колонок таблицы: не заданы. При выборе шаблона прежние фильтры колонок будут очищены.";
        else {
            text += L"\r\n• Сохранены фильтры колонок таблицы:";
            for (const auto& filter : value.column_filters)
                text += L"\r\n  — " + columnName(filter.first,captions) + L": «" + Utf8ToVcl(filter.second) + L"».";
            text += L"\r\nЭти фильтры отбирают строки среди уже полученных результатов.";
        }
    } else text += L"\r\nШаблон старого формата: лимит и фильтры таблицы остаются такими, как сейчас.";
    if (filtersOmitted) text += L"\r\n\r\nВы выбрали сохранение без фильтров: период и фильтры колонок в этот шаблон не записаны.";
    text += L"\r\n\r\nОстались текущими и не записаны в шаблон: сервер / группа серверов, OEM-866, папка сохранения документов, шрифт и набор колонок.";
    if (saved) text += L"\r\n\r\nПри следующем запуске программы шаблон загрузится автоматически из указанного файла.";
    text += L"\r\n\r\nКак использовать: выберите группу и шаблон. Один щелчок применяет параметры, двойной — также запускает поиск.";
    text += L"\r\nЧтобы снова увидеть текст телеграммы, щёлкните по строке результатов.";
    return text;
}

inline void hold(TTimer* timer, bool& held) {
    held = true;
    if (timer) timer->Enabled = false;
}
inline void display(TRichEdit* editor, TTimer* timer, bool& held, const String& message) {
    hold(timer, held);
    editor->Lines->BeginUpdate();
    try {
        editor->Lines->Text = message;
        editor->SelectAll();
        editor->SelAttributes->Color = clWindowText;
        editor->SelAttributes->Style = TFontStyles();
        editor->SelStart = 0;
        editor->SelLength = 0;
    } __finally { editor->Lines->EndUpdate(); }
    SendMessage(editor->Handle, EM_SCROLLCARET, 0, 0);
}
}
