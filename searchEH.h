//---------------------------------------------------------------------------

#ifndef searchEHH
#define searchEHH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include "ADODataDriverEh.hpp"
#include "DataDriverEh.hpp"
#include "DBAxisGridsEh.hpp"
#include "DBGridEh.hpp"
#include "DBGridEhGrouping.hpp"
#include "DBGridEhToolCtrls.hpp"
#include "DynVarsEh.hpp"
#include "EhLibVCL.hpp"
#include "GridsEh.hpp"
#include "MemTableDataEh.hpp"
#include "MemTableEh.hpp"
#include "ToolCtrlsEh.hpp"
#include <Data.DB.hpp>
#include <Data.Win.ADODB.hpp>
#include <Vcl.Menus.hpp>
#include "PrnDbgeh.hpp"
//---------------------------------------------------------------------------
class TForm7 : public TForm
{
__published:	// IDE-managed Components
	TDBGridEh *DBGridEh1;
	TADOQuery *qry;
	TADODataDriverEh *ADODataDriverEh1;
	TDataSource *DataSource1;
	TMemTableEh *memTbl;
	TIntegerField *qrynumber;
	TWideStringField *qryddate;
	TWideStringField *qryttime;
	TWideStringField *qrykuda;
	TIntegerField *qrytype;
	TAutoIncField *qryind;
	TPopupMenu *PopupMenu1;
	TMenuItem *N1;
	TPrintDBGridEh *dbp;
	TIntegerField *qrystr;
	TIntegerField *qrypos;
	void __fastcall N1Click(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
private:	// User declarations
public:		// User declarations
	__fastcall TForm7(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm7 *Form7;
//---------------------------------------------------------------------------
#endif
