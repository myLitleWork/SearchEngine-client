#pragma once

#include <System.Classes.hpp>
#include <System.hpp>
#include <Data.DB.hpp>
#include <FireDAC.Comp.Client.hpp>

namespace ClientDb
{

enum class OpenMode
{
	Existing,
	CreateIfMissing
};

// Initialize FDManager and the native SQLite driver. Call from the main
// thread before any FireDAC connection is used by a worker thread.
void EnsureReady();

// Configure and open a SQLite connection without ADO, ODBC or a DSN.
void Open(TFDConnection* connection,
	const String& databasePath,
	OpenMode openMode = OpenMode::Existing);

TFDConnection* CreateConnection(
	TComponent* owner,
	const String& databasePath,
	OpenMode openMode = OpenMode::Existing);

// DBGridEh local sorting/filtering and code that relies on RecordCount need
// the whole result set fetched.
void PrepareForLocalSort(TFDQuery* query);

// Apply only to dynamic queries that read legacy SQLite schemas. SQLite does
// not enforce VARCHAR(n), while FireDAC sizes its DatS columns from n.
void ApplyLegacyTextCompatibility(TFDQuery* query);

// WideMemo fields otherwise appear as "(WIDEMEMO)" in some data-aware
// controls.
void ShowMemoText(TDataSet* dataSet);

} // namespace ClientDb
