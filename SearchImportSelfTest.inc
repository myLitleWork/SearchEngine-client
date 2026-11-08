// Blank native form: bypass the real constructor, DFM, files and auth startup.
struct SearchImportTestAccess {
    static void Run(int& checks) {
        auto require = [&checks](bool ok, const char* message) {
            if (!ok) throw std::runtime_error(message);
            ++checks;
        };
        std::unique_ptr<TfMain> form(new TfMain(nullptr,1));
        auto db=new TFDConnection(form.get()); db->LoginPrompt=false;
        db->Params->Values[L"DriverID"]=L"SQLite"; db->Params->Values[L"Database"]=L":memory:";
        db->Connected=true;
        db->ExecSQL(L"create table res (num integer, date date, date_podp date, podp_num text, tel_num integer, isp text, from_to text, rel real, dir text, type text, kr text, pril_name text, pril_count text, last_mesto text, gde_sht text, blank text, deleted integer, source_server_id text)");
        form->qryTelegi=new TFDQuery(form.get()); form->qryTelegi->Connection=db;
        form->memTbl=new TMemTableEh(form.get());
        form->ggSaveProgress=new TGauge(form.get());
        form->rchEdt=new TRichEdit(form.get());
        form->searchControl_=std::make_shared<SearchRequestControl>();
        std::vector<Telega> rows(2000);
        for (int i=0;i<int(rows.size());++i) {
            rows[i].num=std::to_string(i+1); rows[i].tel_num=std::to_string(i+1);
            rows[i].date="01.11.2026"; rows[i].date_podp="31.10.2026";
            rows[i].source_server_id="loopback";
        }
        int batches=0;
        while (!form->convTelegs(rows)) {
            ++batches; Application->ProcessMessages();
            if(batches>1000) throw std::runtime_error("row import not completing");
        }
        require(batches>=3 && form->searchSaveInserted_==2000,"real row importer yields between batches and saves all rows");
        require(!db->InTransaction,"row import commits transaction");
        std::unique_ptr<TFDQuery> check(new TFDQuery(nullptr)); check->Connection=db;
        check->SQL->Text=L"select count(*) from res"; check->Open();
        require(check->Fields->Fields[0]->AsInteger==2000,"all rows present in disposable SQLite");
        check->Close(); db->ExecSQL(L"delete from res");
        const bool completed=form->convTelegs(rows);
        const auto firstBatch=form->searchImportIndex_;
        require(!completed && firstBatch>0 && firstBatch<=500 && db->InTransaction,"batch leaves cancellable transaction");
        form->searchControl_->cancelled=true;
        require(form->convTelegs(rows),"row import acknowledges cancellation");
        check->Open();
        require(check->Fields->Fields[0]->AsInteger==0 && !db->InTransaction,"cancel rolls back partial row import");
        check->Close();
        require(!form->searchImportReport_ && form->searchSaveInserted_==0,"cancel resets import state");
    }
};
