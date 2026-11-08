// Included only by formMain.cpp in Debug. No main form, user DB or network.
#include "TemplateOrderSelfTest.inc"
#include "TemplateFeedbackSelfTest.inc"
#include "TemplateMigrationSelfTest.inc"
#include "UnlimitedSearchSelfTest.inc"
#include "SearchImportSelfTest.inc"
int RunTemplateSelfTest(const String& output)
{
    int checks = 0;
    nh::json report;
    auto require = [&checks](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
        ++checks;
    };
    try {
        Application->Initialize();
        nh::json legacy = {{"search_text","123"},{"field","SQL"},{"type",0},{"sql",true},{"exc",false}};
        Request old = legacy.get<Request>();
        require(old.template_version == 1 && old.period == 0 && old.max_results == 0, "legacy defaults");
        require(nh::json(old) == legacy, "legacy exact roundtrip");
        Request extended = old;
        extended.template_version = 2;
        extended.period = 3;
        extended.max_results = 300;
        extended.newest_first = true;
        extended.filters_saved = true;
        extended.column_filters["num"] = "> 1";
        Request restored = nh::json(extended).get<Request>();
        require(restored.column_filters == extended.column_filters && restored.period == 3 && restored.max_results == 300, "v2 roundtrip");
        nh::json invalid = legacy; invalid["period"] = 99; invalid["date_field"] = "unsafe";
        restored = invalid.get<Request>();
        require(restored.period == 0 && restored.date_field == "DData", "invalid optional values");
        std::unique_ptr<TForm> form(new TForm(static_cast<TComponent*>(nullptr)));
        auto field = new TComboBox(form.get());
        auto text = new TComboBox(form.get());
        auto sql = new TCheckBox(form.get());
        auto exact = new TCheckBox(form.get());
        auto direction = new TRadioGroup(form.get());
        field->Parent = form.get(); text->Parent = form.get();
        sql->Parent = form.get(); exact->Parent = form.get(); direction->Parent = form.get();
        direction->Items->Add(L"Вх."); direction->Items->Add(L"Исх.");
        myRequest::init(field,text,sql,exact,direction);
        myRequest::loadTemplates({});
        myRequest::putTemplate(L"Входящие", L"Сегодня", old);
        myRequest::addPeriodTemplates();
        require(myRequest::getTemplate(L"Входящие",L"Сегодня").search_text == "123", "preserve name collision");
        require(myRequest::getTemplate(L"Исходящие",L"Вчера").period == 2, "outgoing preset");
        require(myRequest::groupTempaltes(L"Входящие").front() == L"Сегодня", "period order");
        myRequest::setTemplate(L"Исходящие",L"Вчера");
        require(direction->ItemIndex == 1 && sql->Checked && text->Text == L"1=1", "VCL template application");
        myRequest::setTemplate(L"Входящие",L"Сегодня");
        require(text->Text == L"123" && field->Text == L"SQL", "legacy VCL application");

        std::unique_ptr<TFDConnection> db(new TFDConnection(nullptr));
        db->LoginPrompt = false;
        db->Params->Values[L"DriverID"] = L"SQLite";
        db->Params->Values[L"Database"] = L":memory:";
        db->Connected = true;
        std::unique_ptr<TFDQuery> query(new TFDQuery(nullptr));
        query->Connection = db.get();
        query->SQL->Text = L"CREATE TABLE ARCHIVE (`index` INTEGER, DData TEXT, DataPodp TEXT)";
        query->ExecSQL();
        const TDateTime anchors[] = {EncodeDate(2026,10,31),EncodeDate(2027,1,1),EncodeDate(2024,2,29),EncodeDate(2024,3,1),EncodeDate(2026,3,1)};
        for (auto anchor : anchors) {
            query->SQL->Text = L"DELETE FROM ARCHIVE"; query->ExecSQL();
            for (int offset=-40;offset<=2;++offset) {
                for (int format=0;format<3;++format) {
                    const String pattern = format == 0 ? L"dd.mm.yyyy" : format == 1 ? L"yyyy-mm-dd" : L"yyyymmdd";
                    query->SQL->Text = L"INSERT INTO ARCHIVE VALUES (:id,:d,:p)";
                    query->ParamByName(L"id")->AsInteger = 500-offset*3+format;
                    query->ParamByName(L"d")->AsString = FormatDateTime(pattern, anchor+offset);
                    query->ParamByName(L"p")->AsString = FormatDateTime(pattern, anchor+offset-1);
                    query->ExecSQL();
                }
            }
            for (int period=1;period<=5;++period) {
                TDateTime start=anchor,end=anchor;
                if (period==2) { start=anchor-1;end=start; }
                if (period==3) start=anchor-6;
                if (period==4) start=anchor-29;
                if (period==5) { unsigned short y,m,d;DecodeDate(anchor,y,m,d);start=EncodeDate(y,m,1); }
                auto condition = template_search::condition("1=1","DData",conv::to_string(FormatDateTime(L"yyyy-mm-dd",start).c_str()),conv::to_string(FormatDateTime(L"yyyy-mm-dd",end).c_str()),true,1000);
                query->SQL->Text = L"SELECT * FROM ARCHIVE WHERE " + US(condition);
                query->Open();query->FetchAll();
                require(query->RecordCount == (int(double(end)-double(start))+1)*3, "calendar range and date formats");
                require(query->FieldByName(L"DData")->AsString == FormatDateTime(L"yyyymmdd",end), "date precedes index in ordering");
                query->Close();
            }
        }
        // Exercise actual EhLib local filters on an isolated FireDAC dataset.
        query->SQL->Text = L"SELECT 1 AS num UNION ALL SELECT 2 AS num";
        query->Open();query->FetchAll();
        auto memory = new TMemTableEh(form.get());
        memory->LoadFromDataSet(query.get(), -1, lmCopy, false);
        auto source = new TDataSource(form.get()); source->DataSet = memory;
        auto grid = new TDBGridEh(form.get()); grid->Parent = form.get();grid->DataSource = source;
        grid->STFilter->Local = true;
        grid->Columns->Items[0]->STFilter->ExpressionStr = US(extended.column_filters.at("num"));
        grid->ApplyFilter();
        memory->First();
        require(memory->FieldByName(L"num")->AsInteger == 2 && memory->RecordCount == 1, "saved column filter applied");
        grid->Columns->Items[0]->STFilter->ExpressionStr = L"";grid->ApplyFilter();
        require(memory->RecordCount == 2, "clear omitted filters");
        grid->DataSource = nullptr;source->DataSet = nullptr;
        if (ParamCount() >= 3) {
            const std::wstring defaultsPath(ParamStr(3).c_str());
            std::string bytes;
            require(user_file_replace::ReadAll(defaultsPath, bytes), "read shipped defaults");
            const nh::json defaults = nh::json::parse(bytes);
            myRequest::loadTemplates({});
            myRequest::addPeriodTemplates();
            require(nh::json(myRequest::getTemplates()) == defaults, "shipped defaults match ten builtins");
            const std::wstring absentPath = std::wstring(output.c_str()) + L".absent";
            require(!auth_paths::isRegularFile(absentPath), "disposable fallback path absent");
            myRequest::loadTemplates({});
            LoadTemplatesFromPaths(absentPath, defaultsPath);
            require(nh::json(myRequest::getTemplates()) == defaults, "fresh install loads shipped defaults");
            const auto userBytes = nh::json{{"custom", {{"legacy", legacy}}}}.dump(2);
            const std::wstring userPath = std::wstring(output.c_str()) + L".user.json";
            require(user_file_replace::WriteReplacing(userPath, userBytes.data(), userBytes.size()), "write disposable legacy fixture");
            LoadTemplatesFromPaths(userPath, defaultsPath);
            require(nh::json(myRequest::getTemplates()) == nh::json::parse(userBytes), "user templates take priority without merging");
            require(user_file_replace::ReadAll(userPath, bytes) && bytes == userBytes, "user templates unchanged on load");
        }
        RunTemplateOrderChecks(output, checks);
        RunTemplateFeedbackChecks(output, checks);
        RunTemplateMigrationChecks(output, checks);
        if(ParamCount() >= 4) RunUnifiedToolbarChecks(ParamStr(4),checks);
        RunUnlimitedSearchChecks(checks);
        SearchImportTestAccess::Run(checks);
        RunSearchSocketChecks(checks);
        report = {{"passed",checks},{"status","passed"},{"database",":memory:"}};
    } catch (const Exception& e) { report = {{"passed",checks},{"error",conv::to_string(e.Message.c_str())}}; }
      catch (const std::exception& e) { report = {{"passed",checks},{"error",e.what()}}; }
    const auto bytes = report.dump(2);
    if (!user_file_replace::WriteReplacing(std::wstring(output.c_str()), bytes.data(), bytes.size())) return 2;
    return report.contains("error") ? 1 : 0;
}
