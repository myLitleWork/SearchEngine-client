//---------------------------------------------------------------------------

#ifndef formSelectServerH
#define formSelectServerH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <string>
#include <vector>

#include "MultiServerManager.h"
//---------------------------------------------------------------------------
class TfSelectServer : public TForm
{
__published:
	TPanel *pnBottom;
	TButton *btnOk;
	TButton *btnCancel;
	TLabel *lbHint;
	TListBox *lstServers;
	TPanel *pnOpis;
	TLabel *lblOpisYear;
	TEdit *edtOpisYear;
	TLabel *lblOpisPath;
	TCheckBox *chkReplaceOpis;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormAfterMonitorDpiChanged(TObject *Sender, int OldDPI, int NewDPI);
	void __fastcall lstServersDblClick(TObject *Sender);
	void __fastcall lstServersClick(TObject *Sender);
	void __fastcall btnOkClick(TObject *Sender);
	void __fastcall OpisOptionsChange(TObject *Sender);
private:
	std::vector<std::string> serverIds_;
	std::vector<ServerEndpoint> endpoints_;
	bool opisMode_ = false;
	String opisDirectory_;
	String currentYear_;
	int designPpi_ = 96;
	void ApplyDpiMetrics();
	void SyncOkEnabled();
	void UpdateOpisSource();
	String OpisPath() const;
public:
	__fastcall TfSelectServer(TComponent* Owner);

	void SetServers(
		const std::vector<ServerEndpoint>& endpoints,
		const std::string& preferredServerId);
	std::string SelectedServerId() const;
	void SetOpisMode(const String& directory, const String& currentYear);
	String SelectedOpisYear() const;
};
//---------------------------------------------------------------------------
extern PACKAGE TfSelectServer *fSelectServer;
//---------------------------------------------------------------------------
#endif
