#pragma once

#include <System.hpp>
#include <Data.DB.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <SysUtils.hpp>
#include <memory>

#include "SqliteDb.h"

// ---------------------------------------------------------------------------
// Header-only module: splitting legacy Res.db3 into Settings.db3 + SearchRes.db3.
// Implemented inline on purpose so it does not require a separate .obj in the
// C++Builder project (the IDE project manager would otherwise need to track it).
// ---------------------------------------------------------------------------

struct LocalDbPaths
{
	String exeDir;
	String settingsDb;
	String searchResDb;
	String legacyResDb;
};

namespace LocalDbFiles
{
	inline const wchar_t* const LEGACY_DB   = L"Res.db3";
	inline const wchar_t* const SETTINGS_DB = L"Settings.db3";
	inline const wchar_t* const SEARCH_DB   = L"SearchRes.db3";
}

namespace ResDbDetail
{
	template<typename T>
	struct VclDeleter {
		void operator()(T* p) const { delete p; }
	};

	template<typename T>
	using VclPtr = std::unique_ptr<T, VclDeleter<T> >;

	inline const char* settingsDdl()
	{
		return
			"PRAGMA foreign_keys = false;"
			"DROP TABLE IF EXISTS \"map\";"
			"CREATE TABLE \"map\" (\"Value\" TEXT(255), \"Key\" TEXT(255));"
			"DROP TABLE IF EXISTS \"map_ish\";"
			"CREATE TABLE \"map_ish\" (\"Value\" TEXT(255), \"Key\" TEXT(255));"
			"DROP TABLE IF EXISTS \"map_vh\";"
			"CREATE TABLE \"map_vh\" (\"Value\" TEXT(255), \"Key\" TEXT(255));"
			"DROP TABLE IF EXISTS \"ports\";"
			"CREATE TABLE \"ports\" (\"port\" integer NOT NULL, \"god\" VARCHAR NOT NULL, PRIMARY KEY (\"port\", \"god\"));"
			"DROP TABLE IF EXISTS \"server_ports\";"
			"CREATE TABLE \"server_ports\" (\"server_id\" VARCHAR NOT NULL, \"god\" VARCHAR NOT NULL, "
			"\"port\" INTEGER NOT NULL, \"enabled\" INTEGER NOT NULL DEFAULT 1, PRIMARY KEY (\"server_id\", \"god\"));"
			"DROP TABLE IF EXISTS \"servers\";"
			"CREATE TABLE \"servers\" (\"server_id\" VARCHAR NOT NULL PRIMARY KEY, \"display_name\" VARCHAR NOT NULL, "
			"\"host\" VARCHAR NOT NULL, \"enabled\" INTEGER NOT NULL DEFAULT 1, \"sort_order\" INTEGER NOT NULL DEFAULT 0);"
			"DROP TABLE IF EXISTS \"settings\";"
			"CREATE TABLE \"settings\" (\"key\" VARCHAR NOT NULL, \"value\" VARCHAR, PRIMARY KEY (\"key\"));"
			"PRAGMA foreign_keys = true;";
	}

	inline const char* resDdl()
	{
		return
			"PRAGMA foreign_keys = false;"
			"DROP TABLE IF EXISTS \"res\";"
			"CREATE TABLE \"res\" ("
			"\"num\" INTEGER NOT NULL, \"date\" DATE, \"date_podp\" DATE, \"isp\" VARCHAR, \"type\" VARCHAR,"
			"\"podp_num\" VARCHAR, \"from_to\" VARCHAR, \"rel\" real, \"dir\" VARCHAR, \"tel_num\" INTEGER,"
			"\"kr\" VARCHAR, \"pril_name\" VARCHAR, \"pril_count\" VARCHAR, \"last_mesto\" VARCHAR,"
			"\"gde_sht\" VARCHAR, \"blank\" VARCHAR, \"deleted\" INTEGER DEFAULT 0, "
			"\"source_server_id\" VARCHAR NOT NULL DEFAULT 'default');"
			"PRAGMA foreign_keys = true;";
	}

}

inline LocalDbPaths makeLocalDbPaths(const String& exeDir)
{
	LocalDbPaths p;
	p.exeDir = exeDir;
	p.settingsDb  = IncludeTrailingPathDelimiter(exeDir) + LocalDbFiles::SETTINGS_DB;
	p.searchResDb = IncludeTrailingPathDelimiter(exeDir) + LocalDbFiles::SEARCH_DB;
	p.legacyResDb = IncludeTrailingPathDelimiter(exeDir) + LocalDbFiles::LEGACY_DB;
	return p;
}

namespace ResDbDetail
{
	inline String sqlitePathForAttach(const String& path)
	{
		String p = path;
		for (int i = 1; i <= p.Length(); ++i)
			if (p[i] == L'\\')
				p[i] = L'/';
		return p;
	}

	inline void execSql(TFDConnection* con, const String& sql)
	{
		VclPtr<TFDQuery> q(new TFDQuery(NULL));
		q->Connection = con;
		q->SQL->Text = sql;
		q->ExecSQL();
	}

	inline void execSqlScript(TFDConnection* con, const String& script)
	{
		String stmt;
		for (int i = 1; i <= script.Length(); ++i) {
			const wchar_t ch = script[i];
			if (ch == L';') {
				stmt = Trim(stmt);
				if (!stmt.IsEmpty())
					execSql(con, stmt);
				stmt = "";
			} else {
				stmt += ch;
			}
		}
		stmt = Trim(stmt);
		if (!stmt.IsEmpty())
			execSql(con, stmt);
	}

	inline VclPtr<TFDConnection> openConnection(
		const String& dbPath,
		ClientDb::OpenMode openMode = ClientDb::OpenMode::Existing)
	{
		VclPtr<TFDConnection> con(new TFDConnection(NULL));
		ClientDb::Open(con.get(), dbPath, openMode);
		return con;
	}

	inline void createDatabaseSchema(const String& dbPath, const String& ddl)
	{
		VclPtr<TFDConnection> con =
			openConnection(dbPath, ClientDb::OpenMode::CreateIfMissing);
		execSqlScript(con.get(), ddl);
		con->Connected = false;
	}

	inline bool resTableHasColumn(TFDConnection* con, const String& columnName)
	{
		VclPtr<TFDQuery> q(new TFDQuery(NULL));
		q->Connection = con;
		q->SQL->Text = L"PRAGMA table_info(res)";
		ClientDb::PrepareForLocalSort(q.get());
		q->Open();
		while (!q->Eof) {
			if (SameText(q->FieldByName(L"name")->AsString, columnName))
				return true;
			q->Next();
		}
		return false;
	}

	inline void ensureResDeletedColumn(const String& searchResDbPath)
	{
		if (!FileExists(searchResDbPath))
			return;
		VclPtr<TFDConnection> con = openConnection(searchResDbPath);
		if (!resTableHasColumn(con.get(), L"deleted"))
			execSql(con.get(), L"ALTER TABLE res ADD COLUMN deleted INTEGER DEFAULT 0");
	}

	inline void ensureMultiServerSchema(const String& settingsDbPath,
									const String& searchResDbPath)
	{
		{
			VclPtr<TFDConnection> con = openConnection(settingsDbPath);
			execSql(con.get(),
				L"CREATE TABLE IF NOT EXISTS servers ("
				L"server_id VARCHAR NOT NULL PRIMARY KEY, display_name VARCHAR NOT NULL, "
				L"host VARCHAR NOT NULL, enabled INTEGER NOT NULL DEFAULT 1, "
				L"sort_order INTEGER NOT NULL DEFAULT 0)");
			execSql(con.get(),
				L"CREATE TABLE IF NOT EXISTS server_ports ("
				L"server_id VARCHAR NOT NULL, god VARCHAR NOT NULL, port INTEGER NOT NULL, "
				L"enabled INTEGER NOT NULL DEFAULT 1, PRIMARY KEY (server_id, god))");
		}

		if (!FileExists(searchResDbPath))
			return;
		VclPtr<TFDConnection> con = openConnection(searchResDbPath);
		if (!resTableHasColumn(con.get(), L"source_server_id"))
			execSql(con.get(),
				L"ALTER TABLE res ADD COLUMN source_server_id VARCHAR NOT NULL DEFAULT 'default'");
	}

	inline bool copyTableAttach(
		TFDConnection* targetCon,
		const String& legacyDbPath,
		const String& tableName,
		String* errorMsg)
	{
		const String attachAlias = L"legacy_src";
		const String attachPath = sqlitePathForAttach(legacyDbPath);

		try {
			execSql(targetCon, "ATTACH DATABASE '" + attachPath + "' AS " + attachAlias);
			execSql(targetCon,
				"INSERT INTO main.\"" + tableName + "\" SELECT * FROM " +
				attachAlias + ".\"" + tableName + "\"");
			execSql(targetCon, "DETACH DATABASE " + attachAlias);
			return true;
		} catch (const Exception& e) {
			try { execSql(targetCon, "DETACH DATABASE " + attachAlias); } catch (...) {}
			if (errorMsg)
				*errorMsg = L"ATTACH copy " + tableName + L": " + e.Message;
			return false;
		}
	}

	inline bool copyTableRowByRow(
		const String& legacyDbPath,
		const String& targetDbPath,
		const String& tableName,
		String* errorMsg)
	{
		try {
			VclPtr<TFDConnection> srcCon = openConnection(legacyDbPath);
			VclPtr<TFDConnection> dstCon = openConnection(targetDbPath);

			VclPtr<TFDQuery> src(new TFDQuery(NULL));
			src->Connection = srcCon.get();
			src->SQL->Text = L"SELECT * FROM \"" + tableName + L"\"";
			ClientDb::ApplyLegacyTextCompatibility(src.get());
			src->Open();

			if (src->RecordCount == 0)
				return true;

			VclPtr<TFDQuery> dst(new TFDQuery(NULL));
			dst->Connection = dstCon.get();

			const int fieldCount = src->FieldCount;
			String cols, placeholders;
			for (int i = 0; i < fieldCount; ++i) {
				if (i > 0) {
					cols += L", ";
					placeholders += L", ";
				}
				cols += L"\"" + src->Fields->Fields[i]->FieldName + L"\"";
				placeholders += L":" + src->Fields->Fields[i]->FieldName;
			}

			dst->SQL->Text =
				L"INSERT INTO \"" + tableName + L"\" (" + cols + L") VALUES (" + placeholders + L")";

			for (int i = 0; i < fieldCount; ++i) {
				TFDParam* p =
					dst->ParamByName(src->Fields->Fields[i]->FieldName);
				p->DataType = src->Fields->Fields[i]->DataType;
			}

			dst->Prepared = true;

			while (!src->Eof) {
				for (int i = 0; i < fieldCount; ++i) {
					TField* f = src->Fields->Fields[i];
					TFDParam* p = dst->ParamByName(f->FieldName);
					if (f->IsNull)
						p->Value = Null();
					else
						p->Value = f->Value;
				}
				dst->ExecSQL();
				src->Next();
			}
			return true;
		} catch (const Exception& e) {
			if (errorMsg)
				*errorMsg = L"Row copy " + tableName + L": " + e.Message;
			return false;
		}
	}

	inline bool copyTable(
		const String& legacyDbPath,
		const String& targetDbPath,
		const String& tableName,
		String* errorMsg)
	{
		try {
			VclPtr<TFDConnection> targetCon = openConnection(targetDbPath);
			if (copyTableAttach(targetCon.get(), legacyDbPath, tableName, NULL))
				return true;
		} catch (...) {}

		return copyTableRowByRow(legacyDbPath, targetDbPath, tableName, errorMsg);
	}

	inline bool migrateLegacyResDb(const LocalDbPaths& paths, String* errorMsg)
	{
		static const wchar_t* settingsTables[] = {
			L"map", L"map_ish", L"map_vh", L"ports", L"settings"
		};

		try {
			createDatabaseSchema(paths.settingsDb, settingsDdl());
			createDatabaseSchema(paths.searchResDb, resDdl());

			for (int i = 0; i < (int)(sizeof(settingsTables) / sizeof(settingsTables[0])); ++i) {
				if (!copyTable(paths.legacyResDb, paths.settingsDb, settingsTables[i], errorMsg))
					return false;
			}

			if (!copyTable(paths.legacyResDb, paths.searchResDb, L"res", errorMsg))
				return false;

			{
				VclPtr<TFDConnection> con = openConnection(paths.settingsDb);
				VclPtr<TFDQuery> q(new TFDQuery(NULL));
				q->Connection = con.get();
				q->SQL->Text = L"SELECT COUNT(*) FROM ports";
				q->Open();
			}

			if (!DeleteFile(paths.legacyResDb.c_str())) {
				if (errorMsg)
					*errorMsg = L"Не удалось удалить старый файл Res.db3. Закройте другие программы, использующие базу.";
				return false;
			}

			return true;
		} catch (const Exception& e) {
			if (errorMsg)
				*errorMsg = L"Ошибка миграции: " + e.Message;
			return false;
		}
	}

	inline void ensureDatabaseExists(const String& dbPath, const String& ddl)
	{
		if (!FileExists(dbPath)) {
			createDatabaseSchema(dbPath, ddl);
		} else {
			try {
				VclPtr<TFDConnection> con = openConnection(dbPath);
				VclPtr<TFDQuery> q(new TFDQuery(NULL));
				q->Connection = con.get();
				q->SQL->Text = L"SELECT 1";
				q->Open();
			} catch (...) {
				createDatabaseSchema(dbPath, ddl);
			}
		}
	}
}

inline bool ensureLocalDatabases(const String& exeDir, String* errorMsg)
{
	const LocalDbPaths paths = makeLocalDbPaths(exeDir);

	try {
		const bool legacyExists = FileExists(paths.legacyResDb);
		const bool settingsExists = FileExists(paths.settingsDb);

		if (legacyExists && !settingsExists) {
			if (!ResDbDetail::migrateLegacyResDb(paths, errorMsg))
				return false;
		} else {
			ResDbDetail::ensureDatabaseExists(paths.settingsDb, ResDbDetail::settingsDdl());
			ResDbDetail::ensureDatabaseExists(paths.searchResDb, ResDbDetail::resDdl());
		}

		ResDbDetail::ensureResDeletedColumn(paths.searchResDb);
		ResDbDetail::ensureMultiServerSchema(paths.settingsDb, paths.searchResDb);

		return true;
	} catch (const Exception& e) {
		if (errorMsg)
			*errorMsg = L"Ошибка инициализации баз данных: " + e.Message;
		return false;
	}
}
