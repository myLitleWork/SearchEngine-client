// Included in the isolated Debug self-test; loopback and synthetic data only.
namespace {
class TSearchHeartbeat : public TObject {
public:
    int ticks = 0;
    void __fastcall Tick(TObject*) { ++ticks; }
};

void RunUnlimitedSearchChecks(int& checks) {
    auto require = [&checks](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
        ++checks;
    };
    int limit = -1;
    require(search_limit::parse(L"Без лимита",limit) && limit == 0, "unlimited label accepted");
    require(search_limit::parse(L" 1000 ",limit) && limit == 1000, "numeric limit accepted");
    for (const auto& bad : {L"0",L"-1",L"",L"300x",L"2147483648"})
        require(!search_limit::parse(bad,limit), "invalid limit rejected");
    const auto unlimited = template_search::condition("1=1","DData","2026-10-01","2026-10-31",true,0);
    require(unlimited.find("LIMIT") == std::string::npos && unlimited.find("BETWEEN") != std::string::npos,
        "unlimited keeps date predicate and order");
    require(template_search::condition("1=1","DData","","",false,300).find("LIMIT 300") != std::string::npos,
        "limited SQL preserved");
    SqlCondition::max = 0;
    SqlCondition::map[L"Подписной №"] = L"PodpNo";
    require(SqlCondition::getLikeVh(L"Подписной №",L"123").Pos(L"LIMIT") == 0,
        "unlimited named-field query");
    SqlCondition::max = 300;
    Request request("1=1","SQL",0,true,false);
    request.template_version=2; request.unlimited_results=true; request.period=4;
    const auto restored=nh::json(request).get<Request>();
    require(restored.unlimited_results && restored.period==4 && restored.max_results==0,
        "unlimited and period template roundtrip");
    require(template_feedback::describe(L"Входящие",L"Месяц",restored,L"X:/Templates.json",true,false,false,{},true)
        .Pos(L"без ограничения количества") > 0, "unlimited receipt");
    const auto now=SearchWatchdog::Clock::now();
    SearchWatchdog guard(now);
    SearchRequestControl state;
    require(guard.due(state,now+std::chrono::seconds(19))==SearchWatchdog::Notice::None,"no early guard");
    require(guard.due(state,now+std::chrono::seconds(20))==SearchWatchdog::Notice::Slow,"20 second guard");
    guard.continued(now+std::chrono::seconds(27));
    require(guard.due(state,now+std::chrono::seconds(46))==SearchWatchdog::Notice::None,"continue resets timer");
    require(guard.due(state,now+std::chrono::seconds(47))==SearchWatchdog::Notice::Slow,"repeat after 20 seconds");
    state.rows=99999;
    require(guard.due(state,now)==SearchWatchdog::Notice::None,"99999 below large threshold");
    state.rows=100000;
    require(guard.due(state,now)==SearchWatchdog::Notice::Large,"actual 100000 triggers gate");
    state.largeApproved=true;
    require(guard.due(state,now)==SearchWatchdog::Notice::None,"large approval retained");
    state.cancelled=true;
    require(guard.due(state,now+std::chrono::hours(1))==SearchWatchdog::Notice::None,"cancel suppresses prompts");

    Telega telegram; telegram.num="123"; telegram.podp_num="123/456";
    const auto record=nh::json(telegram).dump();
    std::string response="[";
    for (int i=0;i<1200;++i) { if(i) response+=","; response+=record; }
    response+="]";
    SearchRequestControl parsing;
    const auto hits=parseSearchResponse(response,"test-server",parsing);
    require(hits.size()==1200 && parsing.rows==1200,"more than 300 parsed without truncation");
    require(hits.back().num=="123" && hits.back().source_server_id=="test-server","fields and provenance retained");
    SearchRequestControl empty;
    require(parseSearchResponse("[]","s",empty).empty() && empty.rows==0,"empty result accepted");
    bool malformed=false;
    try { parseSearchResponse("[1]","s",empty); } catch (...) {malformed=true;}
    require(malformed,"non-document entry rejected");
    empty.cancelled=true;
    bool cancelled=false;
    try { parseSearchResponse(response,"s",empty); } catch (...) {cancelled=true;}
    require(cancelled && empty.rows==0,"parsing cancellation before first record");

    SearchRequestControl paused;
    paused.rows=99999;
    std::atomic_bool parseDone{false};
    bool parseCancelled=false;
    std::thread pausedParser([&] {
        try { parseSearchResponse("["+record+"]","s",paused); }
        catch (...) { parseCancelled=true; }
        parseDone=true;
    });
    const auto waitDeadline=SearchWatchdog::Clock::now()+std::chrono::seconds(3);
    while (paused.rows<100000 && !parseDone && SearchWatchdog::Clock::now()<waitDeadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const bool didPause=paused.rows==100000 && !parseDone;
    paused.cancelled=true;
    pausedParser.join();
    require(didPause && parseCancelled,"large result waits for decision and cancels");

    std::unique_ptr<TFDConnection> db(new TFDConnection(nullptr));
    db->LoginPrompt=false; db->Params->Values[L"DriverID"]=L"SQLite";
    db->Params->Values[L"Database"]=L":memory:"; db->Connected=true;
    db->ExecSQL(L"create table res (num integer)");
    db->ExecSQL(L"with recursive n(i) as (select 1 union all select i+1 from n where i<5000) insert into res select i from n");
    std::unique_ptr<TFDQuery> query(new TFDQuery(nullptr)); query->Connection=db.get();
    std::unique_ptr<TDataSetDriverEh> driver(new TDataSetDriverEh(nullptr)); driver->ProviderDataSet=query.get();
    std::unique_ptr<TMemTableEh> table(new TMemTableEh(nullptr)); table->DataDriver=driver.get();
    query->FetchOptions->Mode=fmAll; table->FetchAllOnOpen=true;
    int batches=0;
    {
        SearchGridLoader loader(query.get(),table.get());
        while (!loader.step()) {
            ++batches;
            if(batches>100) throw std::runtime_error("grid load not completing");
            Application->ProcessMessages();
        }
        require(table->RecordCount==5000,"all grid rows loaded");
    }
    require(!table->ControlsDisabled() && table->FetchAllOnOpen && query->FetchOptions->Mode==fmAll,
        "grid options and controls restored");
    table->Close(); query->Close();
    {
        SearchGridLoader loader(query.get(),table.get());
        loader.step();
    }
    require(!table->ControlsDisabled(),"cancelled grid preparation restores controls");
}

void RunSearchSocketChecks(int& checks) {
    auto require = [&checks](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
        ++checks;
    };
    // Exercise the real requestText transport with no main form or production server.
    for (int scenario=0;scenario<3;++scenario) {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io,tcp::endpoint(tcp::v4(),0));
        const auto port=acceptor.local_endpoint().port();
        std::atomic_bool received{false}, stop{false};
        std::thread server([&] {
            try {
                tcp::socket socket(io); acceptor.accept(socket);
                header request{};
                boost::asio::read(socket,boost::asio::buffer(&request,sizeof(request)));
                std::string body(static_cast<std::size_t>(request.size),'\0');
                boost::asio::read(socket,boost::asio::buffer(&body[0],body.size()));
                received=true;
                if(scenario==2) {
                    header answer{}; answer.command=request.command; answer.size=1000000;
                    boost::asio::write(socket,boost::asio::buffer(&answer,sizeof(answer)));
                    const std::string fragment(65536,'x');
                    boost::asio::write(socket,boost::asio::buffer(fragment));
                }
                const auto start=SearchWatchdog::Clock::now();
                while(!stop && (scenario!=0 || SearchWatchdog::Clock::now()-start<std::chrono::seconds(21)))
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if(scenario==0 && !stop) {
                    header answer{}; answer.command=request.command; answer.size=2;
                    boost::asio::write(socket,boost::asio::buffer(&answer,sizeof(answer)));
                    boost::asio::write(socket,boost::asio::buffer("[]",2));
                }
            } catch (...) { received=true; }
        });
        MyAsioClient client;
        std::string error,response;
        if(!client.tryConnect("127.0.0.1",std::to_string(port),error)) {
            stop=true; server.join(); throw std::runtime_error("loopback connect failed");
        }
        SearchRequestControl state;
        std::atomic_bool done{false}; bool success=false;
        std::thread request([&] {
            success=client.requestText("1=1 ORDER BY `index` DESC",COMMAND::GET_VH_TELEGI_FROM_SQL,
                response,nullptr,&error,nullptr,&state);
            done=true;
        });
        std::unique_ptr<TSearchHeartbeat> heartbeat(new TSearchHeartbeat());
        std::unique_ptr<TTimer> timer(new TTimer(nullptr)); timer->Interval=10;
        timer->OnTimer=heartbeat->Tick; timer->Enabled=true;
        const auto began=SearchWatchdog::Clock::now();
        SearchWatchdog guard(began); bool sawGuard=false;
        while(!done && SearchWatchdog::Clock::now()-began<std::chrono::seconds(25)) {
            Application->ProcessMessages();
            if(scenario==0 && guard.due(state)==SearchWatchdog::Notice::Slow) {
                sawGuard=true; guard.continued(); // User chooses Continue.
            }
            if(scenario>0 && received && SearchWatchdog::Clock::now()-began>std::chrono::milliseconds(100) &&
                (scenario==1 || state.bytes>=65536)) state.cancelled=true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        state.cancelled=true; stop=true;
        request.join(); server.join(); timer->Enabled=false;
        const auto elapsed=SearchWatchdog::Clock::now()-began;
        if(scenario==0) require(success && response=="[]" && sawGuard,"21 second response survives old 10 second idle timeout and soft guard");
        else require(!success && !client.isConnected() && elapsed<std::chrono::seconds(3),
            "cancel interrupts header or body read and closes incomplete transaction");
        require(heartbeat->ticks>0,"UI heartbeat during network request");
        client.disConnect();
    }
}
}
