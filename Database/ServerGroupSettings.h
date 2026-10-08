#pragma once

#include <System.SysUtils.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <memory>

namespace ServerGroups
{
inline bool IsYear(const String& value)
{
    if (value.Length() != 4 || value[1] < L'1' || value[1] > L'9') return false;
    for (int i = 2; i <= 4; ++i)
        if (value[i] < L'0' || value[i] > L'9') return false;
    return true;
}

inline String ReadSetting(TFDConnection* con, const String& key)
{
    std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
    q->Connection = con;
    q->SQL->Text = L"SELECT value FROM settings WHERE key = :key";
    q->ParamByName(L"key")->AsString = key;
    q->Open();
    return q->Eof ? String() : q->Fields->Fields[0]->AsString;
}

inline void WriteSetting(TFDConnection* con, const String& key, const String& value)
{
    std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
    q->Connection = con;
    q->SQL->Text = L"INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)";
    q->ParamByName(L"key")->AsString = key;
    q->ParamByName(L"value")->AsString = value;
    q->ExecSQL();
}

inline int PreferredIndex(TStrings* groups, const String& last, const String& preferred)
{
    int index = last.IsEmpty() ? -1 : groups->IndexOf(last);
    if (index < 0 && !preferred.IsEmpty()) index = groups->IndexOf(preferred);
    if (index < 0 && groups->Count == 1) index = 0;
    return index;
}

inline bool HasYearColumn(TFDConnection* con)
{
    std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
    q->Connection = con;
    q->SQL->Text = L"PRAGMA table_info(server_ports)";
    q->Open();
    while (!q->Eof) {
        if (q->FieldByName(L"name")->AsString == L"server_year") return true;
        q->Next();
    }
    return false;
}

// Run before opening the main Settings connection. VACUUM INTO creates a
// consistent SQLite snapshot, including committed pages in a journal/WAL.
// Empty path is reserved for disposable in-memory self-tests.
inline String EnsureYearColumn(TFDConnection* con, const String& databasePath)
{
    if (HasYearColumn(con)) return String();
    String backupPath;
    std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
    q->Connection = con;
    if (!databasePath.IsEmpty()) {
        String backup = databasePath + L".before-server-groups-" +
            FormatDateTime(L"yyyymmdd-hhnnss-zzz", Now());
        int suffix = 0;
        String candidate = backup + L".bak";
        while (FileExists(candidate)) candidate = backup + L"-" + IntToStr(++suffix) + L".bak";
        backupPath = candidate;
        q->SQL->Text = L"VACUUM INTO :backup";
        q->ParamByName(L"backup")->AsString = candidate;
        q->ExecSQL();
    }
    con->StartTransaction();
    try {
        q->SQL->Text = L"ALTER TABLE server_ports ADD COLUMN server_year INTEGER";
        q->ExecSQL();
        q->SQL->Text = L"UPDATE server_ports SET server_year = CAST(trim(god) AS INTEGER) "
            L"WHERE trim(god) GLOB '[1-9][0-9][0-9][0-9]'";
        q->ExecSQL();
        con->Commit();
    } catch (...) {
        con->Rollback();
        throw;
    }
    return backupPath;
}

inline void ValidatePortRow(TDataSet* row, TFDConnection* con)
{
    const String group = row->FieldByName(L"god")->AsString;
    if (Trim(group).IsEmpty() || group != Trim(group))
        throw Exception(L"Укажите имя группы без пробелов по краям.");
    const String id = row->FieldByName(L"server_id")->AsString;
    std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
    q->Connection = con;
    q->SQL->Text = L"SELECT host FROM servers WHERE server_id = :id";
    q->ParamByName(L"id")->AsString = id;
    q->Open();
    if (id.IsEmpty() || q->Eof)
        throw Exception(L"server_id должен ссылаться на существующую запись servers.");
    const String host = Trim(q->Fields->Fields[0]->AsString);
    q->Close();
    int port = 0;
    if (!TryStrToInt(row->FieldByName(L"port")->AsString, port) || port < 1 || port > 65535)
        throw Exception(L"Укажите TCP-порт от 1 до 65535.");
    const String year = row->FieldByName(L"server_year")->AsString;
    if (!year.IsEmpty() && !IsYear(year))
        throw Exception(L"Год службы должен состоять из четырёх цифр либо быть пустым.");
    if (row->FieldByName(L"enabled")->AsInteger == 0) return;
    q->SQL->Text = L"SELECT p.server_id FROM server_ports p JOIN servers s ON s.server_id=p.server_id "
        L"WHERE p.god=:group AND p.enabled<>0 AND s.enabled<>0 AND p.server_id<>:id "
        L"AND lower(trim(s.host))=lower(:host) AND p.port=:port";
    q->ParamByName(L"group")->AsString = group;
    q->ParamByName(L"id")->AsString = id;
    q->ParamByName(L"host")->AsString = host;
    q->ParamByName(L"port")->AsInteger = port;
    q->Open();
    if (!q->Eof)
        throw Exception(L"Этот адрес и порт уже включены в группе под другим server_id.");
}
}
