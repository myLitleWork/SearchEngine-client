#include <vcl.h>
#pragma hdrstop

#include "SqliteDb.h"

#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <FireDAC.Comp.UI.hpp>
#include <FireDAC.DApt.hpp>
#include <FireDAC.Phys.hpp>
#include <FireDAC.Phys.Intf.hpp>
#include <FireDAC.Phys.SQLite.hpp>
#include <FireDAC.Phys.SQLiteDef.hpp>
#include <FireDAC.Phys.SQLiteWrapper.Stat.hpp>
#include <FireDAC.Stan.Async.hpp>
#include <FireDAC.Stan.Def.hpp>
#include <FireDAC.Stan.Error.hpp>
#include <FireDAC.Stan.Intf.hpp>
#include <FireDAC.Stan.Option.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <FireDAC.VCLUI.Wait.hpp>

#pragma package(smart_init)
#pragma link "FireDAC.Phys.SQLite"
#pragma link "FireDAC.Phys.SQLiteDef"
#pragma link "FireDAC.Phys.SQLiteWrapper.Stat"
#pragma link "FireDAC.Stan.ExprFuncs"

namespace ClientDb
{

namespace
{

const unsigned kMaxStringSize = 32767;

void applyUnicodeRules(TFDConnection* connection)
{
	TFDFormatOptions* options = connection->FormatOptions;
	options->OwnMapRules = true;
	options->MapRules->Clear();
	options->MaxStringSize = kMaxStringSize;

	TFDMapRule* rule = options->MapRules->Add();
	rule->SourceDataType = dtAnsiString;
	rule->TargetDataType = dtWideString;

	rule = options->MapRules->Add();
	rule->SourceDataType = dtMemo;
	rule->TargetDataType = dtWideMemo;
}

void ensureWideMemoRule(TFDFormatOptions* options, TFDDataType sourceType)
{
	for (int i = 0; i < options->MapRules->Count; ++i)
	{
		TFDMapRule* rule = options->MapRules->Items[i];
		if (rule->NameMask.IsEmpty() &&
			rule->TypeMask.IsEmpty() &&
			rule->SourceDataType == sourceType &&
			rule->TargetDataType == dtWideMemo)
		{
			rule->Index = 0;
			return;
		}
	}

	TFDMapRule* rule = options->MapRules->Add();
	rule->SourceDataType = sourceType;
	rule->TargetDataType = dtWideMemo;
	rule->Index = 0;
}

void ensureInfrastructure()
{
	static bool ready = false;
	if (ready)
		return;
	ready = true;

	// Force FireDAC.Stan.Def initialization into the executable before the
	// first connection is opened.
	(void)FDLoadConnDefGlobalFileName();
	if (!FDManager()->Active)
		FDManager()->Active = true;

	FDManager()->FormatOptions->MaxStringSize = kMaxStringSize;
	new TFDPhysSQLiteDriverLink(Application);

	TFDGUIxWaitCursor* waitCursor = new TFDGUIxWaitCursor(Application);
	waitCursor->Provider = L"Forms";
}

void configure(TFDConnection* connection,
	const String& databasePath,
	OpenMode openMode)
{
	connection->Connected = false;
	connection->LoginPrompt = false;
	connection->DriverName = L"SQLite";
	connection->Params->Clear();
	connection->Params->DriverID = L"SQLite";
	connection->Params->Database = databasePath;
	connection->DriverName = L"SQLite";

	connection->Params->Add(openMode == OpenMode::CreateIfMissing
		? String(L"OpenMode=CreateUTF8")
		: String(L"OpenMode=ReadWrite"));
	connection->Params->Add(L"JournalMode=DELETE");
	connection->Params->Add(L"LockingMode=Normal");
	connection->Params->Add(L"Synchronous=Normal");
	connection->Params->Add(L"BusyTimeout=10000");

	applyUnicodeRules(connection);
}

} // namespace

void EnsureReady()
{
	ensureInfrastructure();
}

void Open(TFDConnection* connection,
	const String& databasePath,
	OpenMode openMode)
{
	if (!connection)
		throw EArgumentNilException(L"connection");
	if (databasePath.Trim().IsEmpty())
		throw EArgumentException(L"SQLite database path is empty.");
	if (openMode == OpenMode::Existing && !FileExists(databasePath))
		throw Exception(L"Не найден файл базы данных: " + databasePath);

	ensureInfrastructure();
	configure(connection, databasePath, openMode);

	if (connection->DriverName.IsEmpty())
		throw Exception(L"FireDAC SQLite DriverName is empty: " + databasePath);

	connection->Connected = true;
}

TFDConnection* CreateConnection(TComponent* owner,
	const String& databasePath,
	OpenMode openMode)
{
	TFDConnection* connection = new TFDConnection(owner);
	try
	{
		Open(connection, databasePath, openMode);
	}
	catch (...)
	{
		delete connection;
		throw;
	}
	return connection;
}

void PrepareForLocalSort(TFDQuery* query)
{
	if (!query)
		return;

	query->FetchOptions->Mode = fmAll;
	query->FormatOptions->MaxStringSize = kMaxStringSize;
}

void ApplyLegacyTextCompatibility(TFDQuery* query)
{
	if (!query)
		return;
	if (query->Active)
		throw EInvalidOp(
			L"FireDAC SQLite text compatibility must be applied before Open.");

	query->FetchOptions->Mode = fmAll;
	TFDFormatOptions* options = query->FormatOptions;
	options->OwnMapRules = true;
	options->MaxStringSize = kMaxStringSize;
	ensureWideMemoRule(options, dtAnsiString);
	ensureWideMemoRule(options, dtWideString);
}

void ShowMemoText(TDataSet* dataSet)
{
	if (!dataSet)
		return;

	for (int i = 0; i < dataSet->FieldCount; ++i)
	{
		TBlobField* field =
			dynamic_cast<TBlobField*>(dataSet->Fields->Fields[i]);
		if (field)
			field->DisplayValue = dvFull;
	}
}

} // namespace ClientDb
