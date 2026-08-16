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
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormAfterMonitorDpiChanged(TObject *Sender, int OldDPI, int NewDPI);
	void __fastcall lstServersDblClick(TObject *Sender);
	void __fastcall lstServersClick(TObject *Sender);
	void __fastcall btnOkClick(TObject *Sender);
private:
	std::vector<std::string> serverIds_;
	int designPpi_ = 96;
	void ApplyDpiMetrics();
	void SyncOkEnabled();
public:
	__fastcall TfSelectServer(TComponent* Owner);

	void SetServers(
		const std::vector<ServerEndpoint>& endpoints,
		const std::string& preferredServerId);
	std::string SelectedServerId() const;
};
//---------------------------------------------------------------------------
extern PACKAGE TfSelectServer *fSelectServer;
//---------------------------------------------------------------------------
#endif
