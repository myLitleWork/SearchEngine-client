//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "searchEH.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ADODataDriverEh"
#pragma link "DataDriverEh"
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
#pragma link "PrnDbgeh"
#include "DbUtilsEh.hpp"
#include "EhlibADO.hpp"
#include "EhlibMTE.hpp"
#pragma resource "*.dfm"
TForm7 *Form7;
//---------------------------------------------------------------------------
__fastcall TForm7::TForm7(TComponent* Owner)
	: TForm(Owner)
{
	DBGridEhCenter()->FilterEditCloseUpApplyFilter = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm7::N1Click(TObject *Sender)
{
       dbp->Print();
}
//---------------------------------------------------------------------------


void __fastcall TForm7::Button1Click(TObject *Sender)
{
	//DBGridEh1->FieldColumns["number"]->STFilter->ExpressionStr = "=8315";
   	//Edit1->Text = DBGridEh1->FieldColumns["number"]->STFilter->ExpressionStr;
}
//---------------------------------------------------------------------------

void __fastcall TForm7::FormShow(TObject *Sender)
{
    memTbl->Active = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm7::FormCreate(TObject *Sender)
{
    memTbl->Active = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm7::FormClose(TObject *Sender, TCloseAction &Action)
{
    memTbl->Active = false;
}
//---------------------------------------------------------------------------

