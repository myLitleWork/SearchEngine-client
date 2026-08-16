//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "formSelectServer.h"
#include <algorithm>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfSelectServer *fSelectServer;
//---------------------------------------------------------------------------

namespace {
String Utf8Caption(const std::string& text)
{
	if (text.empty())
		return String();
	return String(UTF8String(text.c_str()));
}
}

__fastcall TfSelectServer::TfSelectServer(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TfSelectServer::FormCreate(TObject *Sender)
{
	(void)Sender;
	Caption = L"Обновление базы";
	lbHint->Caption =
		L"Для выбранного года настроено несколько серверов.\r\n"
		L"Выберите, на какой сервер отправить команду обновления базы.";
	btnOk->Caption = L"Отправить";
	btnCancel->Caption = L"Отмена";
	Constraints->MinWidth = 420;
	Constraints->MinHeight = 320;
	designPpi_ = PixelsPerInch > 0 ? PixelsPerInch : 96;
	ApplyDpiMetrics();
}
//---------------------------------------------------------------------------

void __fastcall TfSelectServer::FormShow(TObject *Sender)
{
	(void)Sender;
	ApplyDpiMetrics();
	SyncOkEnabled();
	if (lstServers->CanFocus())
		lstServers->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TfSelectServer::FormAfterMonitorDpiChanged(
	TObject *Sender, int OldDPI, int NewDPI)
{
	(void)Sender;
	(void)OldDPI;
	(void)NewDPI;
	ApplyDpiMetrics();
}
//---------------------------------------------------------------------------

void TfSelectServer::ApplyDpiMetrics()
{
	const int ppi = CurrentPPI > 0 ? CurrentPPI : designPpi_;
	const int base = designPpi_ > 0 ? designPpi_ : 96;
	auto scale = [ppi, base](int value) {
		return std::max(1, MulDiv(value, ppi, base));
	};

	lbHint->Margins->Left = scale(16);
	lbHint->Margins->Top = scale(16);
	lbHint->Margins->Right = scale(16);
	lbHint->Margins->Bottom = scale(8);

	lstServers->Margins->Left = scale(16);
	lstServers->Margins->Top = scale(8);
	lstServers->Margins->Right = scale(16);
	lstServers->Margins->Bottom = scale(8);

	pnBottom->Height = scale(60);
	pnBottom->Padding->Left = scale(16);
	pnBottom->Padding->Top = scale(12);
	pnBottom->Padding->Right = scale(16);
	pnBottom->Padding->Bottom = scale(12);

	btnOk->Width = scale(110);
	btnCancel->Width = scale(110);
	btnOk->Margins->Left = scale(8);
	btnCancel->Margins->Left = scale(8);

	Constraints->MinWidth = scale(420);
	Constraints->MinHeight = scale(320);
}
//---------------------------------------------------------------------------

void TfSelectServer::SyncOkEnabled()
{
	btnOk->Enabled = lstServers->ItemIndex >= 0 &&
		lstServers->ItemIndex < static_cast<int>(serverIds_.size());
}
//---------------------------------------------------------------------------

void TfSelectServer::SetServers(
	const std::vector<ServerEndpoint>& endpoints,
	const std::string& preferredServerId)
{
	serverIds_.clear();
	lstServers->Items->BeginUpdate();
	try {
		lstServers->Clear();
		int preferredIndex = -1;
		for (std::size_t i = 0; i < endpoints.size(); ++i) {
			const auto& ep = endpoints[i];
			serverIds_.push_back(ep.id);

			String title = Utf8Caption(ep.displayName);
			if (title.IsEmpty())
				title = String(ep.id.c_str());

			String detail = String(ep.host.c_str());
			if (!ep.port.empty()) {
				if (!detail.IsEmpty())
					detail += L":";
				detail += String(ep.port.c_str());
			}

			String line = title;
			if (!detail.IsEmpty())
				line += L"  —  " + detail;
			if (!ep.id.empty() && title != String(ep.id.c_str()))
				line += L"  [" + String(ep.id.c_str()) + L"]";

			lstServers->Items->Add(line);
			if (preferredIndex < 0 && !preferredServerId.empty() &&
				ep.id == preferredServerId) {
				preferredIndex = static_cast<int>(i);
			}
		}
		if (preferredIndex < 0 && !serverIds_.empty())
			preferredIndex = 0;
		lstServers->ItemIndex = preferredIndex;
	}
	__finally {
		lstServers->Items->EndUpdate();
	}
	SyncOkEnabled();
}
//---------------------------------------------------------------------------

std::string TfSelectServer::SelectedServerId() const
{
	const int index = lstServers->ItemIndex;
	if (index < 0 || index >= static_cast<int>(serverIds_.size()))
		return {};
	return serverIds_[static_cast<std::size_t>(index)];
}
//---------------------------------------------------------------------------

void __fastcall TfSelectServer::lstServersClick(TObject *Sender)
{
	(void)Sender;
	SyncOkEnabled();
}
//---------------------------------------------------------------------------

void __fastcall TfSelectServer::lstServersDblClick(TObject *Sender)
{
	(void)Sender;
	if (btnOk->Enabled)
		ModalResult = mrOk;
}
//---------------------------------------------------------------------------

void __fastcall TfSelectServer::btnOkClick(TObject *Sender)
{
	(void)Sender;
	if (SelectedServerId().empty()) {
		ModalResult = mrNone;
		ShowMessage(L"Выберите сервер из списка.");
		return;
	}
	ModalResult = mrOk;
}
//---------------------------------------------------------------------------
