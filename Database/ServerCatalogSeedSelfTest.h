#pragma once

#include "SqliteDb.h"
#include "ServerCatalogSeed.h"
#include <windows.h>

namespace ServerCatalogTest
{
struct Fixture
{
    std::unique_ptr<TFDConnection> con;
    Fixture() : con(new TFDConnection(nullptr))
    {
        ClientDb::Open(con.get(), L":memory:", ClientDb::OpenMode::CreateIfMissing);
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
        exec(L"CREATE TABLE servers (server_id VARCHAR PRIMARY KEY, display_name VARCHAR, "
            L"host VARCHAR, enabled INTEGER, sort_order INTEGER)");
        exec(L"CREATE TABLE server_ports (server_id VARCHAR, god VARCHAR, port INTEGER, "
            L"enabled INTEGER, PRIMARY KEY (server_id, god))");
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
        db.exec(L"INSERT INTO server_ports VALUES ('default', '2026', 32123, 0)");
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
        return 0;
    }
    catch (const Exception& e) {
        OutputDebugStringW((L"Server catalog self-test failed: " + e.Message).c_str());
        return 61;
    }
}