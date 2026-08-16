//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "FrameWay.h"
#include "utils.h"
#include "myAsioClient.h"
#include "myJson.h"
#include <System.Threading.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "ToolCtrlsEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "ToolCtrlsEh"
#pragma resource "*.dfm"
//TfrWay *frWay;


__fastcall TfrWay::TfrWay(TComponent* _Owner, MultiServerManager& manager, TDBGridEh *_dbgMain, TDBGridEh *_dbgMenu)
  : TFrameDbg(_Owner, manager, _dbgMain, _dbgMenu)
{
	InitDbgDS();
}

//---------------------------------------------------------------------------

void TfrWay::InitDbgDS()
{
	if (mt) return;

	mt = new TMemTableEh(this);
	mt->FieldDefs->Clear();
	mt->FieldDefs->Add(L"n",     ftInteger,    0, false);
	mt->FieldDefs->Add(L"ddate", ftWideString, 20, false);
	mt->FieldDefs->Add(L"ttime", ftWideString, 12, false);
	mt->FieldDefs->Add(L"kuda",  ftWideString, 64, false);
	mt->FieldDefs->Add(L"str",   ftWideString, 64, false);
	mt->FieldDefs->Add(L"pos",   ftWideString, 64, false);
	mt->CreateDataSet();

	ds = new TDataSource(this);
	ds->DataSet = mt;

	dbgWay->DataSource = ds;

    // ≈сли колонки у dbgWay заданы вручную, св¤жем их с пол¤ми:
	// (можно сделать в дизайнере Ч FieldName у каждой колонки)
	if (dbgWay->Columns->Count >= 6) {
		dbgWay->Columns->Items[0]->FieldName = L"n";
		dbgWay->Columns->Items[1]->FieldName = L"ddate";
        dbgWay->Columns->Items[2]->FieldName = L"ttime";
		dbgWay->Columns->Items[3]->FieldName = L"kuda";
		dbgWay->Columns->Items[4]->FieldName = L"str";
		dbgWay->Columns->Items[5]->FieldName = L"pos";
	}
}

void TfrWay::FillDbgWayFromResult(
	const std::list<std::map<std::string,std::string>>& result)
{


	mt->DisableControls();
	mt->EmptyTable();

	int i = 1;
	for (const auto& m : result)
	{
        auto get = [&](const char* k)->String {
			auto it = m.find(k);
			return (it != m.end()) ? String(conv::to_wstring(it->second).c_str()) : String();
		};

		String date = get("ddate");
		String time = get("ttime");
		String kuda = get("kuda");
		String str  = get("str");
        String pos  = get("pos");

        if (kuda.Length() < 6) kuda += L"     ";

		mt->AppendRecord(ARRAYOFCONST(( i, date, time, kuda, str, pos )));
        ++i;
    }

	mt->EnableControls();
}

void TfrWay::FillDbg()
{
	const String numberS = dbgMain->Fields[0]->AsString;
	const String dirType = dbgMain->Fields[7]->AsString;
	const COMMAND cmd = (dirType == L"Вх.") ? COMMAND::GET_VH_TELEGA_WAY
											: COMMAND::GET_ISH_TELEGA_WAY;
	const std::string serverId = SourceServerId();


	Screen->Cursor = crHourGlass;

	auto live = uiLive_;
	auto op = TrackOp();

	System::Threading::TTask::Run([this, numberS, cmd, serverId, live, op]() {
		std::string number = AnsiString(numberS).c_str();
		std::string jsonStr;
		bool ok = true;

		std::list<std::map<std::string, std::string>> result;

		try {
			if (live && !live->load(std::memory_order_acquire))
				ok = false;

			std::string err;

			if (ok && !serverManager.requestText(serverId, number, cmd, jsonStr, nullptr, &err)) {
				const String message = "Ошибка запроса: " + Utf8ToVcl(err);
				TThread::Queue(nullptr, [live, message]() {
					if (!live || !live->load(std::memory_order_acquire))
						return;
					ShowMessage(message);
				});
				ok = false;
			}

			if (ok) {
				result = JSON::get<std::list<std::map<std::string, std::string>>>(jsonStr);
			}
		} catch (...) {
			ok = false;
		}

		TThread::Queue(nullptr, [this, ok, result, live, op]() {
			if (!live || !live->load(std::memory_order_acquire) ||
				detached_.load(std::memory_order_acquire))
				return;

			if (!ok) { Screen->Cursor = crDefault; return; }


			FillDbgWayFromResult(result);
			Screen->Cursor = crDefault;
		});
	});



}

void __fastcall TfrWay::dbgWayRowDetailPanelHide(TCustomDBGridEh *Sender, bool &CanHide)

{
    MinusbgHeigth();
}
//---------------------------------------------------------------------------

