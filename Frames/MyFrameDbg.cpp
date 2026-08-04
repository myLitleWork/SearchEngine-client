//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "MyFrameDbg.h"
#include "utils.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "ToolCtrlsEh"
#pragma resource "*.dfm"

//TFrameDbg *FrameDbg;
//---------------------------------------------------------------------------
__fastcall TFrameDbg::TFrameDbg(TComponent* Owner, MultiServerManager& manager, TDBGridEh* g, TDBGridEh *_dbgMenu)
	: TFrame(Owner), serverManager(manager), dbgMain(g), dbgMenu(_dbgMenu)
{
    InitDbgDS();
}

std::string TFrameDbg::SourceServerId() const
{
	try {
		if (dbgMain && dbgMain->DataSource && dbgMain->DataSource->DataSet &&
			dbgMain->DataSource->DataSet->Active) {
			TField* field = dbgMain->DataSource->DataSet->FindField(L"source_server_id");
			if (field && !field->AsString.IsEmpty())
				return AnsiString(field->AsString).c_str();
		}
	}
	catch (...) {
	}
	return serverManager.primaryServerId();
}
//---------------------------------------------------------------------------
void TFrameDbg::PlusDbgHeigth()
{
	const int main_heigth = dbgMain->Height/5;
	dbgMain->RowDetailPanel->Height += main_heigth * kof;
	dbgMenu->RowDetailPanel->Height = main_heigth * kof;
}

// � TFrameDbg.cpp
void TFrameDbg::Detach()
{
    try {
        if (dbgMain) dbgMain->DataSource = nullptr;
        if (dbgMenu) dbgMenu->DataSource = nullptr;

        if (ds) ds->DataSet = nullptr;

        if (mt) {
            if (mt->Active) {
                mt->Active = false;
            }
        }
    }
    catch (...) {
        // �� ������ ������ ��������� ��� ���������� ��� ��������
    }
}


void TFrameDbg::OnShow()
{
	if (dbgMain && dbgMain->DataSource && dbgMain->DataSource->DataSet &&
		isDeletedRow(dbgMain->DataSource->DataSet)) {
		ShowMessage(deletedFileMessage());
		return;
	}

	this->Visible = true;
	PlusDbgHeigth();
	FillDbg();
}

void TFrameDbg::setHeigthMulti(double _kof)
{
	kof = _kof;
}

void TFrameDbg::MinusbgHeigth()
{
	const int main_heigth = dbgMain->Height/5;
	dbgMain->RowDetailPanel->Height -= main_heigth * kof;
	dbgMenu->RowDetailPanel->Height -= main_heigth * kof;
}

