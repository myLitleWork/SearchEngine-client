#pragma once

#include <System.SysUtils.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <memory>

namespace ClientDb
{
// Keep existing endpoints (including disabled ones) authoritative. The default
// matches configure-local-machine's first candidate for the portable template.
inline void SeedServerCatalog(TFDConnection* connection, const String& fallbackHost,
    int calendarYear)
{
    std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
    q->Connection = connection;
    connection->StartTransaction();
    try {
        q->SQL->Text = L"SELECT COUNT(*) AS cnt FROM servers";
        q->Open();
        const bool emptyCatalog = q->FieldByName(L"cnt")->AsInteger == 0;
        q->Close();
        if (emptyCatalog) {
            q->SQL->Text =
                L"INSERT INTO servers (server_id, display_name, host, enabled, sort_order) "
                L"VALUES ('default', 'Основной сервер', :host, 1, 0)";
            q->ParamByName(L"host")->AsString = fallbackHost;
            q->ExecSQL();
        }

        // Also repair a default catalog created by older clients with no ports.
        // Custom or multi-server catalogs require explicit endpoint settings.
        q->SQL->Text =
            L"SELECT COUNT(*) AS cnt FROM servers WHERE server_id = 'default' "
            L"AND enabled <> 0 AND (SELECT COUNT(*) FROM servers) = 1 "
            L"AND NOT EXISTS (SELECT 1 FROM server_ports)";
        q->Open();
        const bool needsPorts = q->FieldByName(L"cnt")->AsInteger == 1;
        q->Close();
        if (emptyCatalog || needsPorts) {
            q->SQL->Text =
                L"INSERT OR IGNORE INTO server_ports (server_id, god, port, enabled) "
                L"SELECT 'default', god, port, 1 FROM ports";
            q->ExecSQL();
        }
        if (needsPorts && calendarYear >= 2000 && calendarYear <= 2099) {
            q->SQL->Text =
                L"INSERT INTO server_ports (server_id, god, port, enabled) "
                L"SELECT 'default', :god, :port, 1 "
                L"WHERE NOT EXISTS (SELECT 1 FROM server_ports) "
                L"AND NOT EXISTS (SELECT 1 FROM ports)";
            q->ParamByName(L"god")->AsString = IntToStr(calendarYear);
            q->ParamByName(L"port")->AsInteger = 15000 + calendarYear % 10;
            q->ExecSQL();
        }
        connection->Commit();
    }
    catch (...) {
        connection->Rollback();
        throw;
    }
}
}
