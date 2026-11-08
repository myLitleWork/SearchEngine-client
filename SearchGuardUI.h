#pragma once
#include "Net/SearchRequestControl.h"
#include <Vcl.Dialogs.hpp>
#include <memory>

enum class SearchGuardChoice { Continue, Limit, Cancel };

inline SearchGuardChoice showSearchGuard(const SearchRequestControl& state,
    SearchWatchdog::Notice notice, bool importing) {
    const auto count = state.rows.load();
    String detail;
    if (notice == SearchWatchdog::Notice::Large)
        detail = L"Результат содержит не менее " + UIntToStr(count) + L" записей. Загрузка большой таблицы может занять время.";
    else detail = importing ? L"Подготовка таблицы занимает больше 20 секунд." : L"Поиск или получение ответа занимает больше 20 секунд.";
    if (count > 0 && notice != SearchWatchdog::Notice::Large)
        detail += L"\nУже распознано записей: " + UIntToStr(count) + L".";
    if (!importing && count == 0)
        detail += L"\nТочное число записей пока неизвестно: сервер присылает готовый результат целиком.";
    detail += L"\n\nМожно сузить период или условия поиска и задать максимум ответов. Лимит 1000 будет применён при следующем нажатии «Поиск».";
    std::unique_ptr<TTaskDialog> dialog(new TTaskDialog(nullptr));
    dialog->Caption = L"Большая выборка";
    dialog->Title = L"Продолжить поиск?";
    dialog->Text = detail;
    dialog->CommonButtons = TTaskDialogCommonButtons();
    dialog->Flags = TTaskDialogFlags() << tfAllowDialogCancellation;
    auto keepWaiting = dialog->Buttons->Add();
    keepWaiting->Caption = L"Продолжить ждать";
    keepWaiting->ModalResult = mrYes;
    auto limit = dialog->Buttons->Add();
    limit->Caption = L"Отменить и поставить лимит 1000";
    limit->ModalResult = mrNo;
    auto cancel = dialog->Buttons->Add();
    cancel->Caption = L"Отменить поиск";
    cancel->ModalResult = mrCancel;
    if (!dialog->Execute()) return SearchGuardChoice::Cancel;
    if (dialog->ModalResult == mrYes) return SearchGuardChoice::Continue;
    return dialog->ModalResult == mrNo ? SearchGuardChoice::Limit : SearchGuardChoice::Cancel;
}
