#pragma once
#include "MyFrameDbg.h"
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
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
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
#include <list>
#include <map>
//---------------------------------------------------------------------------
class TfrWay : public TFrameDbg
{
__published:	// IDE-managed Components

	TDBGridEh *dbgWay;
	void __fastcall dbgWayRowDetailPanelHide(TCustomDBGridEh *Sender, bool &CanHide);



private:	// User declarations

	 void InitDbgDS() override;

	 void FillDbgWayFromResult(
	const std::list<std::map<std::string,std::string>>& result);

public:		// User declarations

	void FillDbg() override;


	__fastcall TfrWay(TComponent* Owner, MyAsioClient& client, TDBGridEh *dbgMain, TDBGridEh *dbgMenu); // <-- ÎÁßÇÀÒÅËÅÍ
};
//---------------------------------------------------------------------------
//extern PACKAGE TfrWay *frWay;
