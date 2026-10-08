#pragma once

#include "SqliteDb.h"
#include "ServerCatalogSeed.h"
#include "../UserFileReplace.h"
#include <windows.h>

namespace ServerCatalogTest
{
struct Fixture
{
    std::unique_ptr<TFDConnection> con;
    explicit Fixture(const String& path = L":memory:") : con(new TFDConnection(nullptr))
    {
        ClientDb::Open(con.get(), path, ClientDb::OpenMode::CreateIfMissing);
    }
    void exec(const String& sql)
    {
        std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
        q->Connection = con.get();
        q->SQL->Text = sql;
        q->ExecSQL();
    }
    String scalar(const String& sql)
    {
        std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
        q->Connection = con.get();
        q->SQL->Text = sql;
        q->Open();
        return q->Fields->Fields[0]->AsString;
    }
    void reset()
    {
        exec(L"DROP TABLE IF EXISTS servers");
        exec(L"DROP TABLE IF EXISTS server_ports");
        exec(L"DROP TABLE IF EXISTS ports");
        exec(L"DROP TABLE IF EXISTS settings");
        exec(L"CREATE TABLE settings (key VARCHAR PRIMARY KEY, value VARCHAR)");
        exec(L"CREATE TABLE servers (server_id VARCHAR PRIMARY KEY, display_name VARCHAR, "
            L"host VARCHAR, enabled INTEGER, sort_order INTEGER)");
        exec(L"CREATE TABLE server_ports (server_id VARCHAR, god VARCHAR, port INTEGER, "
            L"enabled INTEGER, server_year INTEGER, PRIMARY KEY (server_id, god))");
        exec(L"CREATE TABLE ports (port INTEGER, god VARCHAR, PRIMARY KEY (port, god))");
    }
    void seed(int year)
    {
        ClientDb::SeedServerCatalog(con.get(), L"localhost", year);
    }
};
inline void require(bool ok, const String& message)
{
    if (!ok) throw Exception(message);
}
}

// Included only by formMain.cpp in Debug builds; no production databases used.
int RunServerCatalogSeedSelfTest()
{
    using namespace ServerCatalogTest;
    try {
        Fixture db;
        const int years[] = {2026, 2027, 2030, 2099};
        for (int i = 0; i < 4; ++i) {
            const int year = years[i];
            db.reset();
            db.seed(year);
            require(db.scalar(L"SELECT host FROM servers") == L"localhost", L"default host");
            require(db.scalar(L"SELECT god FROM server_ports") == IntToStr(year), L"calendar year");
            require(db.scalar(L"SELECT server_year FROM server_ports") == IntToStr(year), L"service year");
            require(ServerGroups::ReadSetting(db.con.get(), L"default_server_group") == IntToStr(year), L"initial group");
            require(db.scalar(L"SELECT port FROM server_ports") == IntToStr(15000 + year % 10), L"year port");
            ClientDb::SeedServerCatalog(db.con.get(), L"other-host", year + 1);
            require(db.scalar(L"SELECT COUNT(*) FROM server_ports") == L"1", L"no rollover/duplicates");
            require(db.scalar(L"SELECT god FROM server_ports") == IntToStr(year), L"preserve configured year");
            require(db.scalar(L"SELECT host FROM servers") == L"localhost", L"preserve configured host");
        }
        db.reset();
        db.exec(L"INSERT INTO servers VALUES ('default', 'Main', 'remote', 1, 0)");
        db.seed(2026);
        require(db.scalar(L"SELECT port FROM server_ports") == L"15006", L"repair old empty default");
        require(db.scalar(L"SELECT host FROM servers") == L"remote", L"preserve old default host");
        for (int i = 0; i < 2; ++i) {
            db.reset();
            if (i == 1)
                db.exec(L"INSERT INTO servers VALUES ('default', 'Main', 'remote', 1, 0)");
            db.exec(L"INSERT INTO ports VALUES (23456, '2024'), (23457, '2025')");
            db.seed(2026);
            require(db.scalar(L"SELECT COUNT(*) FROM server_ports") == L"2", L"legacy migration count");
            require(db.scalar(L"SELECT port FROM server_ports WHERE god = '2024'") == L"23456", L"legacy custom port");
            require(db.scalar(L"SELECT COUNT(*) FROM server_ports WHERE god = '2026'") == L"0", L"no guessed legacy endpoint");
        }
        db.reset();
        db.exec(L"INSERT INTO servers VALUES ('default', 'Main', 'remote', 1, 0)");
        db.exec(L"INSERT INTO server_ports (server_id, god, port, enabled) VALUES ('default', '2026', 32123, 0)");
        db.exec(L"INSERT INTO ports VALUES (23456, '2024')");
        db.seed(2026);
        require(db.scalar(L"SELECT COUNT(*) FROM server_ports") == L"1", L"preserve existing endpoints");
        require(db.scalar(L"SELECT port || ':' || enabled FROM server_ports") == L"32123:0", L"preserve disabled endpoint");
        const wchar_t* servers[] = {
            L"('custom', 'Custom', 'remote', 1, 0)",
            L"('default', 'Main', 'remote', 0, 0)",
            L"('default', 'Main', 'remote', 1, 0), ('archive', 'Archive', 'remote', 1, 1)"
        };
        for (int i = 0; i < 3; ++i) {
            db.reset();
            db.exec(String(L"INSERT INTO servers VALUES ") + servers[i]);
            db.seed(2026);
            require(db.scalar(L"SELECT COUNT(*) FROM server_ports") == L"0", L"no guessing custom/disabled/multiple servers");
        }
        db.reset();
        db.exec(L"CREATE TRIGGER reject_seed BEFORE INSERT ON server_ports "
            L"BEGIN SELECT RAISE(ABORT, 'test rollback'); END");
        bool rejected = false;
        try { db.seed(2026); }
        catch (const Exception&) { rejected = true; }
        require(rejected, L"insertion failure propagates");
        require(db.scalar(L"SELECT COUNT(*) FROM servers") == L"0", L"server seed rolls back");
        require(db.scalar(L"SELECT COUNT(*) FROM server_ports") == L"0", L"port seed rolls back");
        // Real migration code on disposable databases: custom labels are not
        // years, disabled rows survive, and a repeated migration changes nothing.
        db.reset();
        db.exec(L"DROP TABLE server_ports");
        db.exec(L"CREATE TABLE server_ports (server_id VARCHAR, god VARCHAR, port INTEGER, enabled INTEGER, PRIMARY KEY(server_id,god))");
        db.exec(L"INSERT INTO server_ports VALUES ('a','2024',15004,0), ('b','Все годы',15005,1), ('c','99x6',15006,1)");
        ServerGroups::EnsureYearColumn(db.con.get(), String());
        require(db.scalar(L"SELECT server_year FROM server_ports WHERE server_id='a'") == L"2024", L"numeric group hint");
        require(db.scalar(L"SELECT COUNT(*) FROM server_ports WHERE server_year IS NULL") == L"2", L"unknown years remain null");
        require(db.scalar(L"SELECT enabled FROM server_ports WHERE server_id='a'") == L"0", L"disabled row survives");
        db.exec(L"UPDATE server_ports SET server_year=2025 WHERE server_id='b'");
        ServerGroups::EnsureYearColumn(db.con.get(), String());
        require(db.scalar(L"SELECT server_year FROM server_ports WHERE server_id='b'") == L"2025", L"idempotence preserves edits");
        db.exec(L"DROP TABLE server_ports");
        db.exec(L"CREATE TABLE server_ports (server_id VARCHAR, god VARCHAR, port INTEGER, enabled INTEGER)");
        db.exec(L"INSERT INTO server_ports VALUES ('a','2024',15004,1)");
        db.exec(L"CREATE TRIGGER reject_year BEFORE UPDATE ON server_ports BEGIN SELECT RAISE(ABORT,'migration rollback'); END");
        rejected = false;
        try { ServerGroups::EnsureYearColumn(db.con.get(), String()); }
        catch (const Exception&) { rejected = true; }
        require(rejected && !ServerGroups::HasYearColumn(db.con.get()), L"migration rolls back ALTER and UPDATE");
        require(db.scalar(L"SELECT god||':'||port FROM server_ports") == L"2024:15004", L"rollback keeps original row");
        std::unique_ptr<TStringList> groups(new TStringList());
        groups->Add(L"Все годы"); groups->Add(L"2024");
        require(ServerGroups::PreferredIndex(groups.get(), L"Все годы", L"2024") == 0, L"last group before default");
        require(ServerGroups::PreferredIndex(groups.get(), L"removed", L"2024") == 1, L"fallback default");
        require(ServerGroups::PreferredIndex(groups.get(), L"removed", L"removed") == -1, L"ambiguous group requires choice");
        groups->Delete(1);
        require(ServerGroups::PreferredIndex(groups.get(), String(), String()) == 0, L"sole group");
        wchar_t tempDir[MAX_PATH] = {}, tempDb[MAX_PATH] = {};
        require(GetTempPathW(MAX_PATH, tempDir) > 0 && GetTempFileNameW(tempDir, L"SCG", 0, tempDb) != 0, L"temporary migration file");
        user_file_replace::TemporaryFileGuard originalGuard, backupGuard;
        originalGuard.path = tempDb;
        {
            Fixture disk{String(tempDb)};
            disk.reset();
            disk.exec(L"DROP TABLE server_ports");
            disk.exec(L"CREATE TABLE server_ports (server_id VARCHAR, god VARCHAR, port INTEGER, enabled INTEGER)");
            disk.exec(L"INSERT INTO server_ports VALUES ('archive','2024',15004,0)");
            const String backup = ServerGroups::EnsureYearColumn(disk.con.get(), String(tempDb));
            backupGuard.path = std::wstring(backup.c_str());
            require(FileExists(backup), L"SQLite backup created before migration");
            Fixture saved{backup};
            require(!ServerGroups::HasYearColumn(saved.con.get()), L"backup keeps old schema");
            require(saved.scalar(L"SELECT god||':'||port||':'||enabled FROM server_ports") == L"2024:15004:0", L"backup keeps original data");
            require(ServerGroups::EnsureYearColumn(disk.con.get(), String(tempDb)).IsEmpty(), L"no second backup after successful migration");
        }
        // Instantiate the actual DFM without showing a window or contacting a
        // server; check source/year changes and replacement consent.
        Application->Initialize();
        std::unique_ptr<TfSelectServer> dialog(new TfSelectServer(nullptr));
        dialog->SetOpisMode(String(tempDir), L"2026");
        ServerEndpoint known; known.id="archive"; known.host="127.0.0.1"; known.port="15004"; known.year="2024";
        ServerEndpoint unknown; unknown.id="unknown"; unknown.host="127.0.0.1"; unknown.port="15005";
        dialog->SetServers({known, unknown}, "archive");
        require(dialog->SelectedOpisYear() == L"2024", L"opis follows selected service year");
        dialog->lstServers->ItemIndex = 1;
        dialog->lstServersClick(nullptr);
        require(dialog->SelectedOpisYear() == L"2026", L"unknown service year uses calendar suggestion");
        dialog->edtOpisYear->Text = L"20";
        require(!dialog->btnOk->Enabled, L"invalid opis year disables save");
        return 0;
    }
    catch (const Exception& e) {
        OutputDebugStringW((L"Server catalog self-test failed: " + e.Message).c_str());
        return 61;
    }
}
