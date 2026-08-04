//---------------------------------------------------------------------------

#pragma once
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
#include "DBCtrlsEh.hpp"
#include <System.ImageList.hpp>
#include <Vcl.ImgList.hpp>
//---------------------------------------------------------------------------
class TfrAttachments : public TFrameDbg
{
__published:	// IDE-managed Components
	TDBGridEh *dbgAttachments;
	TImageList *ImageList1;
	void __fastcall dbgAttachmentsRowDetailPanelHide(TCustomDBGridEh *Sender, bool &CanHide);
	void __fastcall dbgAttachmentsColumns1CellButtons0Draw(TCustomDBGridEh *Grid, TColumnEh *Column,
		  TDBGridCellButtonEh *CellButton, TCanvas *Canvas, Gridseh::TGridCoord &Cell,
          Gridseh::TGridCoord &AreaCell, const TRect &ARect, TCellButtonDrawParamsEh *ButtonDrawParams,
		  bool &Handled);
	void __fastcall dbgAttachmentsColumns2CellButtons0Click(TObject *Sender, bool &Handled);



private:	// User declarations

	void InitDbgDS() override;

public:		// User declarations

	void FillDbg() override;

	__fastcall TfrAttachments(TComponent* Owner, MultiServerManager& manager, TDBGridEh *dbgMain, TDBGridEh *dbgMenu); // <-- ÎÁßÇÀÒÅËÅÍ
};
//---------------------------------------------------------------------------
//extern PACKAGE TfrAttachments *frAttachments;
//---------------------------------------------------------------------------
