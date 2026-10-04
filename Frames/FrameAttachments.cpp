//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "FrameAttachments.h"
#include "OutgoingArchiveZip.h"
#include "TelegramAttachmentClient.h"
#include "utils.h"
#include "myJson.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "ToolCtrlsEh"
#pragma link "DBCtrlsEh"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfrAttachments::TfrAttachments(TComponent* _Owner, MultiServerManager& manager, TDBGridEh *_dbgMain, TDBGridEh *_dbgMenu)
  : TFrameDbg(_Owner, manager, _dbgMain, _dbgMenu)
{
	InitDbgDS();
}
//---------------------------------------------------------------------------
void TfrAttachments::InitDbgDS()
{
	if (mt) return;

	mt = new TMemTableEh(this);
	mt->FieldDefs->Clear();
	mt->FieldDefs->Add(L"num",       ftWideString, 30, false);
	mt->FieldDefs->Add(L"file",       ftWideString, 255, false);
	mt->FieldDefs->Add(L"exist", ftWideString, 30, false);
	mt->FieldDefs->Add(L"size",        ftWideString, 30, false);
	mt->CreateDataSet();

	ds = new TDataSource(this);
	ds->DataSet = mt;

	dbgAttachments->DataSource = ds;
}

namespace
{
	String formatAttachmentSize(std::uint64_t bytes)
	{
		const wchar_t* units[] = {L"Б", L"КБ", L"МБ", L"ГБ", L"ТБ"};
		int u = 0;
		double v = static_cast<double>(bytes);
		while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }

		if (v >= 100.0 || u == 0)
			return String().sprintf(L"%.0f %s", v, units[u]);
		if (v >= 10.0)
			return String().sprintf(L"%.1f %s", v, units[u]);
		return String().sprintf(L"%.2f %s", v, units[u]);
	}

	String datasetText(TDataSet* dataSet, const String& fieldName)
	{
		if (!dataSet)
			return String();
		TField* field = dataSet->FindField(fieldName);
		if (!field || field->IsNull)
			return String();
		return field->AsString;
	}

	std::uint64_t fileSizeBytes(const String& path)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		const auto size = fs::file_size(fs::path(path.c_str()), ec);
		if (ec)
			return 0;
		return static_cast<std::uint64_t>(size);
	}
}

void TfrAttachments::FillDbg()
{
	if (!dbgMain || !dbgMain->DataSource || !dbgMain->DataSource->DataSet) {
		ShowMessage(L"Нет выбранной телеграммы");
		return;
	}

	TDataSet* mainSet = dbgMain->DataSource->DataSet;
	int id = 0;
	try {
		id = StrToInt(datasetText(mainSet, L"num"));
	}
	catch (const Exception& e) {
		ShowMessage(L"Некорректный номер телеграммы: " + e.Message);
		return;
	}

	const bool isIn = datasetText(mainSet, L"type") == L"Вх.";
	std::vector<TelegramAttachmentInfo> items;
	std::string err;
	if (!requestTelegramAttachmentList(
			serverManager, SourceServerId(), id, isIn ? 0 : 1, items, err)) {
		ReportRequestError(L"Ошибка запроса", err);
		return;
	}

	auto appendRow = [this](const String& num, const String& file, bool exist, const String& size)
	{
		const String str_exist = exist ? L"Да" : L"Нет";
		mt->AppendRecord(ARRAYOFCONST((num, file, str_exist, size)));
	};

	mt->DisableControls();
	try {
		mt->EmptyTable();

		if (isIn) {
			int i = 1;
			for (const auto& item : items) {
				appendRow(
					IntToStr(i++),
					Utf8ToVcl(item.name),
					item.exists,
					formatAttachmentSize(item.size));
			}
		}
		else {
			const std::string zipName = outgoing_archive_zip::ZipFileNameUtf8(id);
			bool zipExists = false;
			const TelegramAttachmentInfo* advertised =
				findAdvertisedTelegramAttachment(items, zipName);
			if (advertised && advertised->exists)
				zipExists = true;

			std::vector<String> prilTokens;
			outgoing_archive_zip::SplitPrilNames(
				datasetText(mainSet, L"pril_name"), prilTokens);
			for (int i = 0; i < static_cast<int>(prilTokens.size()); ++i) {
				if (!outgoing_archive_zip::IsValidLeafName(prilTokens[i]))
					continue;
				appendRow(
					IntToStr(i + 1),
					prilTokens[i],
					zipExists,
					L"—");
			}
		}
	}
	catch (...) {
	}
	mt->EnableControls();
}
void __fastcall TfrAttachments::dbgAttachmentsRowDetailPanelHide(TCustomDBGridEh *Sender,
          bool &CanHide)
{
	MinusbgHeigth();
}
//---------------------------------------------------------------------------



void __fastcall TfrAttachments::dbgAttachmentsColumns1CellButtons0Draw(
    TCustomDBGridEh *Grid,
    TColumnEh *Column,
    TDBGridCellButtonEh *CellButton,
    TCanvas *Canvas,
	Gridseh::TGridCoord &Cell,
	Gridseh::TGridCoord &AreaCell,
    const TRect &ARect,
    TCellButtonDrawParamsEh *ButtonDrawParams,
    bool &Handled)
{
    // Чтобы грид сам кнопку не рисовал

}

//---------------------------------------------------------------------------

void __fastcall TfrAttachments::dbgAttachmentsColumns2CellButtons0Click(TObject *Sender,
          bool &Handled)
{
	if (!dbgMain || !dbgMain->DataSource || !dbgMain->DataSource->DataSet ||
		!dbgAttachments || !dbgAttachments->DataSource ||
		!dbgAttachments->DataSource->DataSet)
		return;

	TDataSet* mainSet = dbgMain->DataSource->DataSet;
	TDataSet* attachSet = dbgAttachments->DataSource->DataSet;
	const int id = StrToInt(datasetText(mainSet, L"num"));
	const String fileName = datasetText(attachSet, L"file");
	const String exist = datasetText(attachSet, L"exist");

	if (exist != L"Да")
	{
		ShowMessage(L"Файл не доступен для скачивания!");
		return;
	}

	const bool isIn = datasetText(mainSet, L"type") == L"Вх.";

	std::unique_ptr<TSaveDialog> dlg(new TSaveDialog(this));
	dlg->FileName = fileName;
	dlg->Filter = L"Все файлы (*.*)|*.*";
	dlg->Options = dlg->Options << ofOverwritePrompt << ofPathMustExist;
	if (!dlg->Execute())
		return;

	if (isIn) {
		std::string err;
		std::uint64_t receivedSize = 0;
		const std::string requestName = conv::to_string(std::wstring(fileName.c_str()));
		const std::string outPath = std::string(AnsiString(dlg->FileName).c_str());
		if (!downloadTelegramAttachment(
				serverManager,
				SourceServerId(),
				id,
				0,
				requestName,
				outPath,
				&receivedSize,
				&err,
				true,
				true)) {
			ShowMessage(L"Ошибка сохранения файла \"" + fileName + L"\":\n" +
				Utf8ToVcl(err));
			return;
		}

		ShowMessage(L"Файл успешно сохранён:\n" + dlg->FileName +
			L"\nРазмер: " + UIntToStr(receivedSize) + L" байт");
		return;
	}

	int payloadIndex = 0;
	try {
		payloadIndex = StrToInt(datasetText(attachSet, L"num"));
	}
	catch (...) {
		ShowMessage(L"Некорректный номер приложения");
		return;
	}

	std::vector<String> prilTokens;
	outgoing_archive_zip::SplitPrilNames(
		datasetText(mainSet, L"pril_name"), prilTokens);
	if (payloadIndex < 1 ||
		payloadIndex > static_cast<int>(prilTokens.size()) ||
		prilTokens[payloadIndex - 1] != fileName) {
		ShowMessage(L"Имя приложения не совпадает с карточкой телеграммы");
		return;
	}

	outgoing_archive_zip::UniqueTempDir tempDir;
	if (!tempDir.ok()) {
		ShowMessage(L"Не удалось создать временный каталог");
		return;
	}

	const std::string zipName = outgoing_archive_zip::ZipFileNameUtf8(id);
	const String zipPath = tempDir.path + Utf8ToVcl(zipName);
	std::string err;
	std::uint64_t receivedSize = 0;
	if (!downloadTelegramAttachment(
			serverManager,
			SourceServerId(),
			id,
			1,
			zipName,
			std::string(AnsiString(zipPath).c_str()),
			&receivedSize,
			&err,
			true,
			false)) {
		ShowMessage(L"Ошибка загрузки архива приложений:\n" + Utf8ToVcl(err));
		return;
	}

	std::wstring pathError;
	if (!outgoing_archive_zip::ExtractAttachmentToPath(
			zipPath, payloadIndex, dlg->FileName, &pathError)) {
		ShowMessage(L"Ошибка сохранения файла \"" + fileName + L"\":\n" +
			String(pathError.c_str()));
		return;
	}

	const std::uint64_t savedSize = fileSizeBytes(dlg->FileName);
	ShowMessage(L"Файл успешно сохранён:\n" + dlg->FileName +
		L"\nРазмер: " + UIntToStr(savedSize) + L" байт");
}
//---------------------------------------------------------------------------
