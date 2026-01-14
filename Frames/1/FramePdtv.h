//---------------------------------------------------------------------------

#ifndef FramePdtvH
#define FramePdtvH
#include "MyFrameDbg.h"
//---------------------------------------------------------------------------
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
#include "ToolCtrlsEh.hpp"
//---------------------------------------------------------------------------
class TfrPdtv : public TFrameDbg
{
__published:	// IDE-managed Components
	TDBGridEh *dbgPdtv;
	TMemo *memPdtv;
	void __fastcall dbgPdtvRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow);




private:	// User declarations
	void InitDbgDS() override;

public:		// User declarations

	void FillDbg() override;
	__fastcall TfrPdtv(TComponent* Owner);
	__fastcall TfrPdtv(TComponent* Owner, MyAsioClient* client, TDBGridEh *dbgMain, TDBGridEh *dbgMenu); // <-- ÎÁßÇÀÒÅËÅÍ
};
//---------------------------------------------------------------------------
extern PACKAGE TfrPdtv *frPdtv;
//---------------------------------------------------------------------------
#endif
