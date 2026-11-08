#pragma once
#include <Vcl.StdCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
struct TemplateOptionsUI {
    TComboBox* period = nullptr;
    TComboBox* dateField = nullptr;
    TCheckBox* newest = nullptr;
};
inline TemplateOptionsUI BuildTemplateOptionsUI(TComponent* owner, TGroupBox* group,
    int ppi, TNotifyEvent onChange) {
    if (ppi <= 0) ppi = 96;
    TemplateOptionsUI controls;
    auto label = new TLabel(owner);
    label->Parent = group;
    label->Caption = L"Период AutoPad:";
    label->Font->Style = TFontStyles();
    label->Font->Height = -MulDiv(12,ppi,96);
    label->SetBounds(MulDiv(164,ppi,96),MulDiv(17,ppi,96),MulDiv(204,ppi,96),MulDiv(15,ppi,96));
    controls.period = new TComboBox(owner);
    controls.dateField = new TComboBox(owner);
    TComboBox* combos[] = {controls.period, controls.dateField};
    for (int i=0;i<2;++i) {
        combos[i]->Parent = group;
        combos[i]->Style = csDropDownList;
        combos[i]->ParentFont = false;
        combos[i]->Font->Style = TFontStyles();
        combos[i]->Font->Height = -MulDiv(12,ppi,96);
        combos[i]->SetBounds(MulDiv(164,ppi,96),MulDiv(33+i*27,ppi,96),MulDiv(204,ppi,96),MulDiv(23,ppi,96));
    }
    const wchar_t* periods[] = {L"Без периода",L"Сегодня",L"Вчера",L"Последние 7 дней",L"Последние 30 дней",L"Текущий месяц"};
    for (auto item : periods) controls.period->Items->Add(item);
    controls.dateField->Items->Add(L"Дата вх./исх.");
    controls.dateField->Items->Add(L"Дата подписи");
    controls.period->ItemIndex = 0;
    controls.dateField->ItemIndex = 0;
    controls.period->OnChange = onChange;
    controls.newest = new TCheckBox(owner);
    controls.newest->Parent = group;
    controls.newest->ParentFont = false;
    controls.newest->Font->Style = TFontStyles();
    controls.newest->Font->Height = -MulDiv(12,ppi,96);
    controls.newest->Caption = L"Сначала новые по дате";
    controls.newest->SetBounds(MulDiv(164,ppi,96),MulDiv(87,ppi,96),MulDiv(204,ppi,96),MulDiv(21,ppi,96));
    group->Hint = L"Период по датам карточек AutoPad. Лимит действует на каждый сервер; фильтры колонок — на полученную выборку.";
    group->ShowHint = true;
    return controls;
}
