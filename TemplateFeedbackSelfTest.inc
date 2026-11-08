class TFeedbackQueuedReply : public TObject {
public:
    bool held = false;
    bool delivered = false;
    TRichEdit* editor = nullptr;
    void __fastcall Complete() { delivered = true; if (!held) editor->Text=L"Late telegram"; }
};
void RunTemplateFeedbackChecks(const String& output, int& checks)
{
    auto require = [&checks](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
        ++checks;
    };
    const String path = L"X:/Клиент/app/Templates.json";
    Request sample("254/", conv::to_string(L"Подписной №"), 0, true, false);
    sample.template_version = 2; sample.max_results = 5;
    const nh::json original = sample;
    const String receipt = template_feedback::describe(L"подп", L"254", sample, path, true, false, false);
    require(receipt.Pos(L"Шаблон успешно сохранён.") == 1, "success receipt heading");
    require(receipt.Pos(L"Название: «254»") > 0 && receipt.Pos(L"Группа: «подп»") > 0 && receipt.Pos(path) > 0, "receipt identity and complete Unicode path");
    require(receipt.Pos(L"Подписной номер") > 0 && receipt.Pos(L"«254/»") > 0 && receipt.Pos(L"входящие") > 0, "plain-language field query direction");
    require(receipt.Pos(L"поле должно содержать указанный текст") > 0 && receipt.Pos(L"не более 5 результатов от каждого сервера") > 0, "matching and saved result cap");
    require(receipt.Pos(L"все даты") > 0 && receipt.Pos(L"прежние фильтры колонок будут очищены") > 0, "no-period and no-filter explanation");
    require(receipt.Pos(L"max_results") == 0 && receipt.Pos(L"DData") == 0 && receipt.Pos(L"; колонок:") == 0, "no internal identifiers in ordinary receipt");
    require(nh::json(sample) == original, "formatting does not change saved parameters");
    const String updated = template_feedback::describe(L"подп",L"254",sample,path,true,true,true);
    require(updated.Pos(L"обновлён") > 0 && updated.Pos(L"Вы выбрали сохранение без фильтров") > 0, "overwrite and omitted filters clearly reported");
    sample.exc = true;
    require(template_feedback::describe(L"подп",L"254",sample,path,true,false,false).Pos(L"всё значение поля должно совпасть") > 0, "exact match explanation");
    sample.search_text = std::string(220,'x')+"QUERY_END";
    require(template_feedback::describe(L"подп",L"254",sample,path,true,false,false).Pos(L"QUERY_END") > 0, "complete long query shown");
    sample.search_text = "1=1";sample.field="SQL";sample.type=1;sample.max_results=300;sample.newest_first=true;sample.date_field="DataPodp";
    const wchar_t* periods[] = {L"сегодня",L"вчера",L"предыдущие 6 дней",L"предыдущие 29 дней",L"с первого числа текущего месяца"};
    for (int i=1;i<=5;++i) {
        sample.period=i;
        const String text=template_feedback::describe(L"Исходящие",L"Период",sample,path,true,false,false);
        require(text.Pos(periods[i-1]) > 0 && text.Pos(L"исходящие") > 0 && text.Pos(L"дата подписи") > 0, "saved period and date source");
        require(text.Pos(L"Даты периода будут вычисляться заново") > 0 && text.Pos(L"сначала новые") > 0 && text.Pos(L"300 результатов") > 0, "relative date and sorting explanation");
    }
    sample.column_filters={{"podp_num","254/"},{"from_to","город"}};
    const String filtered=template_feedback::describe(L"Входящие",L"Фильтр",sample,path,true,false,false,{{"from_to",L"Отправитель\r\nили получатель"}});
    require(filtered.Pos(L"Подписной номер: «254/»") > 0 && filtered.Pos(L"Отправитель  или получатель") > 0 && filtered.Pos(L"podp_num") == 0, "column captions instead of database names");
    require(filtered.Pos(L"среди уже полученных результатов") > 0, "local filters scope explained");
    const String selected=template_feedback::describe(L"Исходящие",L"Сегодня",sample,path,false,false,false,{},false);
    require(selected.Pos(L"Шаблон выбран.") == 1 && selected.Pos(L"готовый шаблон из комплекта") > 0 && selected.Pos(L"успешно сохранён") == 0, "unsaved builtin is not claimed saved");
    sample.sql=false;sample.search_text=conv::to_string(L"проверка связи");
    const String textMode=template_feedback::describe(L"Текст",L"Поиск",sample,path,true,false,false);
    require(textMode.Pos(L"в тексте документов") > 0 && textMode.Pos(L"проверка связи") > 0 && textMode.Pos(L"Какие документы: исходящие") == 0, "text mode does not claim SQL direction filter");
    sample.template_version=1;
    require(template_feedback::describe(L"Старые",L"Запрос",sample,path,false,false,false).Pos(L"лимит и фильтры таблицы остаются такими, как сейчас") > 0, "legacy scope explained");
    std::unique_ptr<TForm> form(new TForm(static_cast<TComponent*>(nullptr)));
    auto editor=new TRichEdit(form.get());editor->Parent=form.get();editor->SetBounds(0,0,500,350);
    editor->Text=L"Previous telegram";editor->SelectAll();editor->SelAttributes->Color=clRed;editor->SelAttributes->Style=TFontStyles()<<fsBold;
    auto timer=new TTimer(form.get());timer->Enabled=true;
    std::unique_ptr<TFeedbackQueuedReply> pending(new TFeedbackQueuedReply());
    pending->editor = editor;
    // Same held-state contract used by the queued telegram completion in formMain.cpp.
    TThread::ForceQueue(nullptr,pending->Complete);
    template_feedback::display(editor,timer,pending->held,receipt);
    CheckSynchronize(100);
    require(pending->held && !timer->Enabled, "feedback holds automatic text loading");
    require(pending->delivered && editor->Text.Pos(L"Шаблон успешно сохранён.") == 1 && editor->Text.Pos(L"Late telegram") == 0, "queued telegram cannot replace held notice");
    require(editor->SelStart == 0 && editor->SelLength == 0, "receipt opens at beginning");
    require(ColorToRGB(editor->SelAttributes->Color) == ColorToRGB(clWindowText) && !editor->SelAttributes->Style.Contains(fsBold), "old telegram highlighting removed");
    const auto bytes=conv::to_string(receipt.c_str());
    require(user_file_replace::WriteReplacing(std::wstring(output.c_str())+L".receipt.txt",bytes.data(),bytes.size()), "save synthetic receipt example");
}
