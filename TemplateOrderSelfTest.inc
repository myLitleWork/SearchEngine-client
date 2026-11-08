// Debug-only VCL event tests; persistence is confined to the supplied audit path.
class TTemplateDragTestCommit : public TComponent {
public:
    TListBox* groups = nullptr;
    String group;
    std::wstring path;
    int writes = 0;
    __fastcall TTemplateDragTestCommit(TComponent* owner) : TComponent(owner) {}
    void __fastcall Moved(TObject* sender) {
        PersistTemplateListOrder(static_cast<TListBox*>(sender), sender == groups, group, path);
        ++writes;
    }
};

void RunTemplateOrderChecks(const String& output, int& checks)
{
    auto require = [&checks](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
        ++checks;
    };
    const String incoming = L"Входящие", outgoing = L"Исходящие";
    const std::vector<String> expected{L"Сегодня",L"Вчера",L"Последние 7 дней",L"Последние 30 дней",L"Текущий месяц"};
    myRequest::loadOrder({}); myRequest::loadTemplates({}); myRequest::addPeriodTemplates();
    const auto untouchedData = nh::json(myRequest::getTemplates());
    require(myRequest::groupTempaltes(incoming) == expected && myRequest::groupTempaltes(outgoing) == expected, "both default period sequences");
    std::unique_ptr<TForm> form(new TForm(static_cast<TComponent*>(nullptr)));
    auto groups = new TListBox(form.get()); groups->Parent = form.get(); groups->SetBounds(0,0,230,240);
    auto names = new TListBox(form.get()); names->Parent = form.get(); names->SetBounds(240,0,230,240);
    groups->Sorted = true; names->Sorted = true; // Regress the old DFM setting.
    auto groupDrag = new TTemplateListDrag(form.get(), groups);
    auto nameDrag = new TTemplateListDrag(form.get(), names);
    auto commit = new TTemplateDragTestCommit(form.get());
    commit->groups = groups; commit->group = incoming;
    commit->path = std::wstring(output.c_str()) + L".order.json";
    groupDrag->OnMoved = commit->Moved; nameDrag->OnMoved = commit->Moved;
    require(!groups->Sorted && !names->Sorted && groups->DragMode == dmAutomatic && names->DragMode == dmAutomatic, "native lists allow manual ordering");
    for (auto item : myRequest::groupsNames()) groups->Items->Add(item);
    for (auto item : myRequest::groupTempaltes(incoming)) names->Items->Add(item);
    for (int i=0;i<5;++i) require(names->Items->Strings[i] == expected[i], "VCL preserves insertion period order");
    auto drag = [&require](TListBox* list, int from, int to) {
        list->ItemIndex = from;
        TDragObject* object = nullptr;
        list->OnStartDrag(list, object);
        const TRect rect = list->ItemRect(to);
        bool accept = false;
        list->OnDragOver(list,list,5,(rect.Top+rect.Bottom)/2,dsDragMove,accept);
        require(accept, "native drag accepts same list");
        list->OnDragDrop(list,list,5,(rect.Top+rect.Bottom)/2);
        list->OnEndDrag(list,list,5,(rect.Top+rect.Bottom)/2);
    };
    drag(groups,0,1);
    require(myRequest::groupsNames().front() == outgoing && groups->ItemIndex == 1 && groups->Items->Strings[1] == incoming, "group move and selected identity");
    require(myRequest::getOrder().templates.empty(), "group sort leaves template lists automatic");
    require(LoadTemplateOrderFromPath(commit->path) && myRequest::groupsNames().front() == outgoing, "group order survives reload");
    require(myRequest::groupTempaltes(incoming) == expected && myRequest::groupTempaltes(outgoing) == expected, "untouched groups keep defaults after reload");
    drag(names,0,3);
    auto custom = expected; custom.erase(custom.begin()); custom.insert(custom.begin()+3, expected[0]);
    require(myRequest::groupTempaltes(incoming) == custom && names->ItemIndex == 3 && names->Items->Strings[3] == expected[0], "template move and selected identity");
    require(myRequest::groupTempaltes(outgoing) == expected, "manual sort affects only one group");
    require(LoadTemplateOrderFromPath(commit->path) && myRequest::groupTempaltes(incoming) == custom && myRequest::groupsNames().front() == outgoing, "independent manual lists survive reload");
    require(nh::json(myRequest::getTemplates()) == untouchedData, "drag never changes template parameters or legacy format");
    int writes = commit->writes;
    drag(names,1,1);
    require(commit->writes == writes, "same-position drop does not save");
    names->ItemIndex = 0; TDragObject* object = nullptr; names->OnStartDrag(names,object);
    bool accept = true; names->OnDragOver(names,groups,5,5,dsDragMove,accept);
    require(!accept, "cross-list drag rejected");
    names->OnDragDrop(names,groups,5,5);names->OnEndDrag(names,nullptr,5,5);
    require(commit->writes == writes && myRequest::groupTempaltes(incoming) == custom, "rejected drag does not mutate or save");
    names->OnStartDrag(names,object); names->OnEndDrag(names,nullptr,0,0);
    require(commit->writes == writes, "cancel drag does not save");
    // Empty space below the last row appends, without losing the item.
    names->ItemIndex = 0;names->OnStartDrag(names,object);
    names->OnDragDrop(names,names,5,names->ClientHeight-2);names->OnEndDrag(names,names,5,names->ClientHeight-2);
    const auto moved = custom.front();custom.erase(custom.begin());custom.push_back(moved);
    require(myRequest::groupTempaltes(incoming) == custom && names->Items->Strings[4] == moved, "drop on empty space appends");
    std::string savedBytes;
    require(user_file_replace::ReadAll(commit->path,savedBytes), "manual file exists");
    const auto savedPath = commit->path;
    commit->path = std::wstring(output.c_str()) + L".missing-parent\\order.json";
    bool failed = false;
    try { drag(names,0,2); } catch (const Exception&) { failed = true; }
    require(failed && names->Items->Strings[0] == custom[0] && names->ItemIndex == 0 && myRequest::groupTempaltes(incoming) == custom, "failed save rolls back UI and memory");
    commit->path = savedPath;
    std::string bytes;
    require(user_file_replace::ReadAll(savedPath,bytes) && bytes == savedBytes, "failed save leaves disk order intact");
    // New items in a manual list append; automatic lists still rank by period.
    Request extra("1=1","SQL",0,true,false);extra.template_version=2;extra.period=1;
    myRequest::putTemplate(incoming,L"New today",extra);
    auto enlarged = myRequest::groupTempaltes(incoming);
    require(enlarged.back() == L"New today" && std::equal(custom.begin(),custom.end(),enlarged.begin()), "new item appends to manual list");
    extra.period=3;myRequest::putTemplate(outgoing,L"Renamed week",extra);
    auto automatic = myRequest::groupTempaltes(outgoing);
    require(automatic.front() == expected[0] && automatic[1] == expected[1] && std::find(automatic.begin(),automatic.end(),String(L"Renamed week")) < std::find(automatic.begin(),automatic.end(),expected[3]), "automatic list sorts renamed template by period");
    myRequest::removeTemplate(incoming,custom[0]);
    auto afterDelete = myRequest::groupTempaltes(incoming);
    require(std::find(afterDelete.begin(),afterDelete.end(),custom[0]) == afterDelete.end(), "stale order does not resurrect deleted template");
    TemplateListOrder stale;stale.groups={"missing","missing"};
    stale.templates[conv::to_string(outgoing.c_str())]={"missing","missing"};myRequest::loadOrder(stale);
    require(myRequest::groupsNames().size() == 2 && myRequest::groupTempaltes(outgoing) == automatic, "unknown and duplicate order entries are ignored");
    const std::wstring badPath = std::wstring(output.c_str())+L".bad-order.json";
    const std::string bad="{broken";
    require(user_file_replace::WriteReplacing(badPath,bad.data(),bad.size()), "write corrupt order fixture");
    require(!LoadTemplateOrderFromPath(badPath) && myRequest::getOrder().groups.empty() && myRequest::getOrder().templates.empty(), "corrupt sidecar falls back to defaults");
    require(user_file_replace::ReadAll(badPath,bytes) && bytes == bad, "corrupt sidecar is not overwritten on load");
    auto transientList = new TListBox(form.get());
    transientList->Parent = form.get();
    auto transientDrag = new TTemplateListDrag(form.get(),transientList);
    delete transientList;
    delete transientDrag;
    require(true, "controller survives list destruction before its owner");
}
