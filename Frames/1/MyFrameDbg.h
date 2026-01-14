//---------------------------------------------------------------------------

#ifndef MyFrameDbgH
#define MyFrameDbgH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include "MemTableDataEh.hpp"
#include "MemTableEh.hpp"
#include <Data.DB.hpp>
#include "DBAxisGridsEh.hpp"
#include "DBGridEh.hpp"
#include "DBGridEhGrouping.hpp"
#include "DBGridEhToolCtrls.hpp"
#include "DynVarsEh.hpp"
#include "EhLibVCL.hpp"
#include "GridsEh.hpp"
#include "ToolCtrlsEh.hpp"

#include "myAsioClient.h"

//---------------------------------------------------------------------------
class TFrameDbg : public TFrame
{
__published:	// IDE-managed Components

protected:	// User declarations

	TDBGridEh *dbgMain = nullptr;
	TDBGridEh *dbgMenu = nullptr;
	TDataSource *ds = nullptr;
	MyAsioClient* client;
	double kof = 1.5;

	virtual void InitDbgDS() = 0;

	virtual ~TFrameDbg() = default;

public:

	void OnShow();
	virtual void FillDbg() = 0;
	void PlusDbgHeigth();
	void MinusbgHeigth();
	void setHeigthMulti(double kof);

	TMemTableEh *mt = nullptr;
	__fastcall TFrameDbg(TComponent* Owner);
	__fastcall TFrameDbg(TComponent* Owner, MyAsioClient* c, TDBGridEh* g, TDBGridEh *dbgMenu);
};
//---------------------------------------------------------------------------
extern PACKAGE TFrameDbg *FrameDbg;
//---------------------------------------------------------------------------
#endif
