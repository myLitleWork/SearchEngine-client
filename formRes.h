//---------------------------------------------------------------------------

#ifndef formResH
#define formResH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.CheckLst.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.WinXPickers.hpp>
#include <map>
#include <set>
//---------------------------------------------------------------------------
class TfRes : public TForm
{
__published:	// IDE-managed Components
	TMemo *memo;
	TButton *bAdd;
	TPanel *Panel1;
	TLabel *Label1;
	TPanel *Panel2;
	TCheckListBox *chkExcs;
	TLabel *Label2;
	TPanel *Panel3;
	TLabel *Label3;
	TPanel *Panel4;
	TLabel *Label4;
	TListBox *lstIsp;
	TListBox *lstSoIsp;
	TPanel *Panel5;
	TRadioGroup *rgIS;
	TButton *bAddRem;
	TDatePicker *dp;
	TCheckBox *chkSrok;
	void __fastcall bAddClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall chkSrokClick(TObject *Sender);
	void __fastcall chkExcsClick(TObject *Sender);
	void __fastcall lstIspClick(TObject *Sender);
	void __fastcall lstSoIspClick(TObject *Sender);
	void __fastcall bAddRemClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	std::string n_sht;
    bool add_res_press = false;
	std::string __fastcall getJsonRes();
	std::map<String, String> myExcs;
	std::set<String> sIsp, sSoIsp;
	__fastcall TfRes(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfRes *fRes;
//---------------------------------------------------------------------------
#endif
