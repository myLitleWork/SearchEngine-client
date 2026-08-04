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

#include "MultiServerManager.h"

//---------------------------------------------------------------------------
class TFrameDbg : public TFrame
{
__published:	// IDE-managed Components

protected:	// User declarations

	TDBGridEh *dbgMain = nullptr;
	TDBGridEh *dbgMenu = nullptr;
	TDataSource *ds = nullptr;
	MultiServerManager& serverManager;
	double kof = 1.5;
	std::string SourceServerId() const;

	virtual void InitDbgDS() = 0;



public:

    virtual ~TFrameDbg() = default;

	void OnShow();
	virtual void FillDbg() = 0;
	void PlusDbgHeigth();
	void MinusbgHeigth();
	void setHeigthMulti(double kof);
	void Detach();   // 👈 добавляем метод

	TMemTableEh *mt = nullptr;

	__fastcall TFrameDbg(TComponent* Owner, MultiServerManager& manager, TDBGridEh* g, TDBGridEh *dbgMenu);
};
//---------------------------------------------------------------------------
//extern PACKAGE TFrameDbg *FrameDbg;
//---------------------------------------------------------------------------
#endif

