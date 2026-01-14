//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "MyFrameDbg.h"
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
__fastcall TFrameDbg::TFrameDbg(TComponent* Owner, MyAsioClient& c, TDBGridEh* g, TDBGridEh *_dbgMenu)
	: TFrame(Owner), client(c), dbgMain(g), dbgMenu(_dbgMenu)
{
    InitDbgDS();
}
//---------------------------------------------------------------------------
void TFrameDbg::PlusDbgHeigth()
{
	const int main_heigth = dbgMain->Height/5;
	dbgMain->RowDetailPanel->Height += main_heigth * kof;
	dbgMenu->RowDetailPanel->Height = main_heigth * kof;
}

// В TFrameDbg.cpp
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
        // на всякий случай подавляем все исключения при закрытии
    }
}


void TFrameDbg::OnShow()
{
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

