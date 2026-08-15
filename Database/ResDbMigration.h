#pragma once

#include <System.hpp>
#include <Data.DB.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <SysUtils.hpp>
#include <memory>

#include "SqliteDb.h"
#include "UserFileReplace.h"

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

	inline bool tableExists(TFDConnection* con, const String& tableName);

	inline bool ensureResDeletedColumn(const String& searchResDbPath, String* errorMsg)
	{
		if (!FileExists(searchResDbPath))
			return true;
		try {
			VclPtr<TFDConnection> con = openConnection(searchResDbPath);
			if (!tableExists(con.get(), L"res")) {
				if (errorMsg)
					*errorMsg =
						L"Локальная схема SearchRes неполная: нет таблицы res.";
				return false;
			}
			if (!resTableHasColumn(con.get(), L"deleted"))
				execSql(con.get(), L"ALTER TABLE res ADD COLUMN deleted INTEGER DEFAULT 0");
			if (!resTableHasColumn(con.get(), L"deleted")) {
				if (errorMsg)
					*errorMsg =
						L"Локальная схема SearchRes неполная: нет колонки res.deleted.";
				return false;
			}
			return true;
		}
		catch (const Exception& e) {
			if (errorMsg)
				*errorMsg =
					L"Не удалось проверить/обновить колонку res.deleted: " + e.Message;
			return false;
		}
	}

	inline bool ensureMultiServerSchema(const String& settingsDbPath,
										const String& searchResDbPath,
										String* errorMsg)
	{
		try {
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
				if (!tableExists(con.get(), L"servers")) {
					if (errorMsg)
						*errorMsg =
							L"Локальная схема Settings неполная: нет таблицы servers.";
					return false;
				}
				if (!tableExists(con.get(), L"server_ports")) {
					if (errorMsg)
						*errorMsg =
							L"Локальная схема Settings неполная: нет таблицы server_ports.";
					return false;
				}
			}

			if (!FileExists(searchResDbPath))
				return true;

			VclPtr<TFDConnection> con = openConnection(searchResDbPath);
			if (!tableExists(con.get(), L"res")) {
				if (errorMsg)
					*errorMsg =
						L"Локальная схема SearchRes неполная: нет таблицы res.";
				return false;
			}
			if (!resTableHasColumn(con.get(), L"source_server_id"))
				execSql(con.get(),
					L"ALTER TABLE res ADD COLUMN source_server_id VARCHAR NOT NULL DEFAULT 'default'");
			if (!resTableHasColumn(con.get(), L"source_server_id")) {
				if (errorMsg)
					*errorMsg =
						L"Локальная схема SearchRes неполная: нет колонки res.source_server_id.";
				return false;
			}
			return true;
		}
		catch (const Exception& e) {
			if (errorMsg)
				*errorMsg =
					L"Не удалось проверить/обновить multi-server схему: " + e.Message;
			return false;
		}
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

	inline bool tableExists(TFDConnection* con, const String& tableName)
	{
		VclPtr<TFDQuery> q(new TFDQuery(NULL));
		q->Connection = con;
		q->SQL->Text =
			L"SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = :name";
		q->ParamByName(L"name")->AsString = tableName;
		q->Open();
		return !q->Eof;
	}

	inline bool tableRowCount(TFDConnection* con, const String& tableName, int* count)
	{
		try {
			VclPtr<TFDQuery> q(new TFDQuery(NULL));
			q->Connection = con;
			q->SQL->Text = L"SELECT COUNT(*) AS n FROM \"" + tableName + L"\"";
			q->Open();
			if (count)
				*count = q->FieldByName(L"n")->AsInteger;
			return true;
		} catch (...) {
			return false;
		}
	}

	inline void closeConnection(VclPtr<TFDConnection>& con)
	{
		if (!con)
			return;
		try {
			if (con->Connected)
				con->Connected = false;
		} catch (...) {}
		con.reset();
	}

	inline String allocateUniqueSibling(const String& directory, const wchar_t* prefix)
	{
		const String parent = ExcludeTrailingPathDelimiter(directory);
		for (int attempt = 0; attempt < 256; ++attempt) {
			const String candidate =
				parent + L"\\" +
				String(user_file_replace::NextTemporaryLeaf(prefix, L".tmp").c_str());
			if (!FileExists(candidate) && !DirectoryExists(candidate))
				return candidate;
		}
		return String();
	}

	inline bool requiredSettingsTablesPresent(TFDConnection* con)
	{
		static const wchar_t* tables[] = {
			L"map", L"map_ish", L"map_vh", L"ports", L"settings"
		};
		for (int i = 0; i < (int)(sizeof(tables) / sizeof(tables[0])); ++i) {
			if (!tableExists(con, tables[i]))
				return false;
		}
		return true;
	}

	inline bool countsMatchLegacy(
		TFDConnection* legacyCon,
		TFDConnection* targetCon,
		const wchar_t* const* tables,
		int tableCount)
	{
		for (int i = 0; i < tableCount; ++i) {
			const String name = tables[i];
			if (!tableExists(legacyCon, name))
				continue;
			if (!tableExists(targetCon, name))
				return false;
			int legacyCount = -1;
			int targetCount = -1;
			if (!tableRowCount(legacyCon, name, &legacyCount) ||
				!tableRowCount(targetCon, name, &targetCount))
				return false;
			if (legacyCount != targetCount)
				return false;
		}
		return true;
	}

	inline bool settingsMigrationComplete(const LocalDbPaths& paths)
	{
		if (!FileExists(paths.settingsDb) || !FileExists(paths.legacyResDb))
			return false;
		try {
			VclPtr<TFDConnection> settingsCon = openConnection(paths.settingsDb);
			VclPtr<TFDConnection> legacyCon = openConnection(paths.legacyResDb);
			if (!requiredSettingsTablesPresent(settingsCon.get()))
				return false;
			static const wchar_t* tables[] = {
				L"map", L"map_ish", L"map_vh", L"ports", L"settings"
			};
			const bool ok = countsMatchLegacy(
				legacyCon.get(), settingsCon.get(), tables,
				(int)(sizeof(tables) / sizeof(tables[0])));
			closeConnection(legacyCon);
			closeConnection(settingsCon);
			return ok;
		} catch (...) {
			return false;
		}
	}

	inline bool validateMigratedPair(
		const String& settingsTmp,
		const String& searchTmp,
		const String& legacyDb,
		String* errorMsg)
	{
		VclPtr<TFDConnection> settingsCon = openConnection(settingsTmp);
		VclPtr<TFDConnection> searchCon = openConnection(searchTmp);
		VclPtr<TFDConnection> legacyCon = openConnection(legacyDb);

		if (!requiredSettingsTablesPresent(settingsCon.get())) {
			if (errorMsg)
				*errorMsg = L"Временный Settings.db3 не содержит обязательные таблицы.";
			return false;
		}
		if (!tableExists(searchCon.get(), L"res")) {
			if (errorMsg)
				*errorMsg = L"Временный SearchRes.db3 не содержит таблицу res.";
			return false;
		}

		static const wchar_t* settingsTables[] = {
			L"map", L"map_ish", L"map_vh", L"ports", L"settings"
		};
		if (!countsMatchLegacy(
				legacyCon.get(), settingsCon.get(), settingsTables,
				(int)(sizeof(settingsTables) / sizeof(settingsTables[0])))) {
			if (errorMsg)
				*errorMsg = L"Число строк в скопированных таблицах настроек не совпало с Res.db3.";
			return false;
		}

		static const wchar_t* resTables[] = { L"res" };
		if (tableExists(legacyCon.get(), L"res") &&
			!countsMatchLegacy(legacyCon.get(), searchCon.get(), resTables, 1)) {
			if (errorMsg)
				*errorMsg = L"Число строк в таблице res не совпало с Res.db3.";
			return false;
		}

		closeConnection(legacyCon);
		closeConnection(searchCon);
		closeConnection(settingsCon);
		return true;
	}

	inline bool publishDbFile(const String& temporaryPath, const String& finalPath, String* errorMsg)
	{
		std::wstring publishError;
		if (user_file_replace::PublishExistingTemporary(
				std::wstring(temporaryPath.c_str()),
				std::wstring(finalPath.c_str()),
				&publishError))
			return true;
		if (errorMsg)
			*errorMsg = String(publishError.c_str());
		return false;
	}

	inline bool migrateLegacyResDb(const LocalDbPaths& paths, String* errorMsg)
	{
		static const wchar_t* settingsTables[] = {
			L"map", L"map_ish", L"map_vh", L"ports", L"settings"
		};

		const String settingsTmp = allocateUniqueSibling(paths.exeDir, L"_SCM-set-");
		const String searchTmp = allocateUniqueSibling(paths.exeDir, L"_SCM-res-");
		if (settingsTmp.IsEmpty() || searchTmp.IsEmpty()) {
			if (errorMsg)
				*errorMsg = L"Не удалось выбрать уникальные временные файлы миграции.";
			return false;
		}

		user_file_replace::TemporaryFileGuard settingsGuard;
		user_file_replace::TemporaryFileGuard searchGuard;
		settingsGuard.path = std::wstring(settingsTmp.c_str());
		searchGuard.path = std::wstring(searchTmp.c_str());

		try {
			createDatabaseSchema(settingsTmp, settingsDdl());
			createDatabaseSchema(searchTmp, resDdl());

			for (int i = 0; i < (int)(sizeof(settingsTables) / sizeof(settingsTables[0])); ++i) {
				if (!copyTable(paths.legacyResDb, settingsTmp, settingsTables[i], errorMsg))
					return false;
			}

			if (!copyTable(paths.legacyResDb, searchTmp, L"res", errorMsg))
				return false;

			if (!validateMigratedPair(settingsTmp, searchTmp, paths.legacyResDb, errorMsg))
				return false;

			// SearchRes first, then Settings (the retry gate). Res.db3 last.
			searchGuard.Release();
			if (!publishDbFile(searchTmp, paths.searchResDb, errorMsg))
				return false;

			settingsGuard.Release();
			if (!publishDbFile(settingsTmp, paths.settingsDb, errorMsg))
				return false;

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

		if (legacyExists && !ResDbDetail::settingsMigrationComplete(paths)) {
			if (!ResDbDetail::migrateLegacyResDb(paths, errorMsg))
				return false;
		} else {
			if (legacyExists) {
				if (!DeleteFile(paths.legacyResDb.c_str())) {
					if (errorMsg)
						*errorMsg = L"Не удалось удалить старый файл Res.db3. Закройте другие программы, использующие базу.";
					return false;
				}
			}
			ResDbDetail::ensureDatabaseExists(paths.settingsDb, ResDbDetail::settingsDdl());
			ResDbDetail::ensureDatabaseExists(paths.searchResDb, ResDbDetail::resDdl());
		}

		if (!ResDbDetail::ensureResDeletedColumn(paths.searchResDb, errorMsg))
			return false;
		if (!ResDbDetail::ensureMultiServerSchema(paths.settingsDb, paths.searchResDb, errorMsg))
			return false;

		return true;
	} catch (const Exception& e) {
		if (errorMsg)
			*errorMsg = L"Ошибка инициализации баз данных: " + e.Message;
		return false;
	}
}
