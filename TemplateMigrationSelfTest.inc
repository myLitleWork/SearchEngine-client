void RunTemplateMigrationChecks(const String& output, int& checks)
{
    auto require=[&checks](bool ok,const char* why){if(!ok)throw std::runtime_error(why);++checks;};
    const std::wstring user=std::wstring(output.c_str())+L".migration-user.json";
    const std::wstring marker=std::wstring(output.c_str())+L".migration-state.json";
    const std::string zero="{\"defaultsVersion\":0}";
    const String incoming=L"Входящие", today=L"Сегодня";
    Request personal("MY PERSONAL QUERY","SQL",0,true,true);
    myRequest::loadTemplates({});myRequest::putTemplate(incoming,today,personal);myRequest::putTemplate(L"Личное",L"Мой запрос",personal);
    const auto before=myRequest::getTemplates();
    TemplateListOrder order;order.groups={conv::to_string(L"Личное"),conv::to_string(incoming.c_str())};
    order.templates[conv::to_string(incoming.c_str())]={conv::to_string(today.c_str())};myRequest::loadOrder(order);
    SaveTemplatesToPath(user);
    require(user_file_replace::WriteReplacing(marker,zero.data(),zero.size()),"prepare migration marker fixture");
    require(MigrateTemplateDefaults(user,marker),"first automatic migration runs");
    require(nh::json(myRequest::getTemplate(incoming,today))==nh::json(personal),"colliding personal preset is preserved exactly");
    require(nh::json(myRequest::getTemplate(L"Личное",L"Мой запрос"))==nh::json(personal),"legacy personal parameters preserved");
    require(myRequest::getTemplates().size()==3 && myRequest::getGroups(incoming).size()==5,"missing groups and periods added");
    require(myRequest::getOrder().groups==order.groups && myRequest::getOrder().templates==order.templates,"migration leaves manual ordering untouched");
    myRequest::removeTemplate(incoming,L"Вчера");SaveTemplatesToPath(user);
    std::string bytes;require(user_file_replace::ReadAll(user,bytes),"read migrated user fixture");
    const auto stable=bytes;
    LoadTemplatesFromPaths(user,L"");
    require(!MigrateTemplateDefaults(user,marker),"completed migration does not run every start");
    require(myRequest::getGroups(incoming).size()==4,"deleted preset is not resurrected after restart");
    require(user_file_replace::ReadAll(user,bytes)&&bytes==stable,"subsequent startup does not rewrite user file");
    const std::wstring bad=user+L".invalid";const std::string invalid="{broken";
    require(user_file_replace::WriteReplacing(bad,invalid.data(),invalid.size()),"prepare invalid user fixture");
    bool stopped=false;try{LoadTemplatesFromPaths(bad,L"");}catch(const Exception&){stopped=true;}
    require(stopped&&user_file_replace::ReadAll(bad,bytes)&&bytes==invalid,"unreadable user JSON stops startup migration without replacement");
    myRequest::loadTemplates(before);
    const std::wstring missing=std::wstring(output.c_str())+L".missing-migration-parent\\Templates.json";
    const std::wstring unusedMarker=missing+L".state";
    stopped=false;try{MigrateTemplateDefaults(missing,unusedMarker);}catch(const Exception&){stopped=true;}
    require(stopped&&nh::json(myRequest::getTemplates())==nh::json(before),"failed migration save restores personal map");
    require(!auth_paths::isRegularFile(unusedMarker),"failed save does not mark migration completed");
}

void RunUnifiedToolbarChecks(const String& path, int& checks)
{
    auto require=[&checks](bool ok,const char* why){if(!ok)throw std::runtime_error(why);++checks;};
    RegisterClass(__classid(TPanel));RegisterClass(__classid(TGroupBox));RegisterClass(__classid(TLabel));
    RegisterClass(__classid(TButton));RegisterClass(__classid(TEdit));RegisterClass(__classid(TCheckBox));
    RegisterClass(__classid(TComboBox));RegisterClass(__classid(TRadioGroup));RegisterClass(__classid(TGridPanel));
    for(int ppi : {96,120,144,168,192}) {
        std::unique_ptr<TForm> form(new TForm(static_cast<TComponent*>(nullptr)));
        form->ClientWidth=2400;form->ClientHeight=400;
        auto toolbar=new TPanel(form.get());toolbar->Parent=form.get();
        std::unique_ptr<TFileStream> text(new TFileStream(path,fmOpenRead|fmShareDenyNone));
        std::unique_ptr<TMemoryStream> binary(new TMemoryStream());ObjectTextToBinary(text.get(),binary.get());binary->Position=0;
        binary->ReadComponent(toolbar);
        auto templates=dynamic_cast<TGroupBox*>(toolbar->FindComponent(L"GroupBox1"));
        auto saving=dynamic_cast<TGroupBox*>(toolbar->FindComponent(L"grpSaveOptions"));
        auto folder=dynamic_cast<TPanel*>(toolbar->FindComponent(L"GroupBox3"));
        require(templates&&saving&&folder&&folder->Parent==saving,"one save group contains options and directory");
        auto controls=BuildTemplateOptionsUI(form.get(),templates,96,nullptr);
        toolbar->ScaleBy(ppi,96);
        require(toolbar->ControlCount==6,"no separate directory or period group");
        require(controls.period->Parent==templates&&controls.dateField->Parent==templates&&controls.newest->Parent==templates,"period controls share templates group");
        auto saveTemplate=dynamic_cast<TButton*>(toolbar->FindComponent(L"Button1"));
        require(saveTemplate&&saveTemplate->Parent==templates,"save template button remains in common group");
        require(saveTemplate->Left+saveTemplate->Width<=controls.period->Left,"template buttons do not overlap period controls");
        for(TControl* control : {static_cast<TControl*>(controls.period),static_cast<TControl*>(controls.dateField),static_cast<TControl*>(controls.newest)})
            require(control->Left>=0&&control->Top>=0&&control->Left+control->Width<=templates->ClientWidth&&control->Top+control->Height<=templates->ClientHeight,"period control fits at target scale");
        auto save=dynamic_cast<TButton*>(toolbar->FindComponent(L"btnSave"));
        auto cancel=dynamic_cast<TButton*>(toolbar->FindComponent(L"btnCancelSave"));
        auto directory=dynamic_cast<TEdit*>(toolbar->FindComponent(L"edtSaveDir"));
        require(save&&cancel&&directory&&save->Parent==folder&&cancel->Parent==folder&&directory->Parent==folder,"save operations and path preserved");
        require(save->Left+save->Width<=cancel->Left&&directory->Left+directory->Width<=folder->ClientWidth,"save controls fit without overlap");
    }
}
