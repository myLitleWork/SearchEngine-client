//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "FrameAttachments.h"
#include "myJson.h"
#include <map>
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
//TfrAttachments *frAttachments;

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
	mt->FieldDefs->Add(L"file",       ftWideString, 30, false);
	mt->FieldDefs->Add(L"exist", ftWideString, 30, false);
	mt->FieldDefs->Add(L"size",        ftWideString, 30, false);
	mt->CreateDataSet();

	ds = new TDataSource(this);
	ds->DataSet = mt;

	dbgAttachments->DataSource = ds;
}

void TfrAttachments::FillDbg()
{


	   auto cmd = COMMAND::GET_TELEGA_ATACHMENTS;

		const int id   = StrToInt(dbgMain->Fields[0]->AsString);     // номер
		const bool isIn = (dbgMain->Fields[7]->AsString == L"Вх.");  // тип

		nh::json j = {
			{"id",   id},
			{"type", isIn ? 0 : 1}
		};

		std::string str_command = j.dump();



	   std::string err;

	   std::string jsonStr;

		// 1) отправка
		if (!serverManager.requestText(
				SourceServerId(), str_command, cmd, jsonStr, nullptr, &err)) {
			ShowMessage("Ошибка запроса: " + String(UTF8String(err.c_str())));
			return;
		}

	   nh::json result = nh::json::parse(jsonStr);

	   auto appendRow = [this](const String& num, const String& file, bool exist, const String& size)
					{
							String str_exist = exist ? "Да" : "Нет";
							mt->DisableControls();
							mt->AppendRecord(ARRAYOFCONST((num, file, str_exist, size)));
							mt->EnableControls();

					};


	// Очищаем таблицу перед заполнением
	mt->DisableControls();

	try {
		mt->EmptyTable();

		// result должен быть массивом: [ {name, exists, size}, ... ]
		if (!result.is_array()) {
			// Ничего не делаем, можно вывести лог/сообщение если нужно
			mt->EnableControls();
			return;
		}

		auto formatSize = [](std::uint64_t bytes)->String {
			const wchar_t* units[] = {L"Б", L"КБ", L"МБ", L"ГБ", L"ТБ"};
			int u = 0;
			double v = static_cast<double>(bytes);
			while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }

			if (v >= 100.0 || u == 0)
				return String().sprintf(L"%.0f %s", v, units[u]);
			if (v >= 10.0)
				return String().sprintf(L"%.1f %s", v, units[u]);
			return String().sprintf(L"%.2f %s", v, units[u]);
		};


		int i = 1;
		for (const auto& item : result) {
			// Безопасные извлечения с дефолтами
			std::string name  = item.value("name",  std::string{});
			bool        exist = item.value("exists", false);
			std::uint64_t sz  = 0;
			try {
				// size может быть > 2^31; читаем как unsigned long long
				if (item.contains("size") && (item["size"].is_number_unsigned() || item["size"].is_number_integer()))
					sz = item["size"].get<std::uint64_t>();
			} catch (...) { sz = 0; }

			// Конвертация имени файла в VCL String (UTF-8 → UTF-16)
			String file = UTF8String(name.c_str());

			appendRow(IntToStr(i++), file, exist, formatSize(sz));
		}
	}
	catch (...) {
		// по желанию: показать сообщение/лог
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
    const int id   = StrToInt(dbgMain->DataSource->DataSet->FieldValues["num"]);     // номер
    AnsiString file_name = dbgAttachments->DataSource->DataSet->FieldValues["file"];
	AnsiString exist = dbgAttachments->DataSource->DataSet->FieldValues["exist"];

	if(exist != "Да")
	{
		ShowMessage("Файл не доступен для скачивания!");
        return;
	}

    std::string err;
	std::string file_name_str = file_name.c_str();
    const bool isIn = (dbgMain->Fields[7]->AsString == L"Вх.");  // тип

    nh::json j = {
        {"id",   id},
        {"type", isIn ? 0 : 1},
        {"file_name", file_name_str}
    };
    auto str_command = j.dump();

    auto cmd = COMMAND::GET_TELEGA_SINGLE_ATACHMENT;

    // Сначала выбираем путь: при отмене диалога файл не скачивается впустую.
    std::unique_ptr<TSaveDialog> dlg(new TSaveDialog(this));
    dlg->FileName = file_name;            // предлагаем исходное имя
    dlg->Filter = "Все файлы (*.*)|*.*";  // фильтр
    if (!dlg->Execute())
        return; // пользователь отменил

    // SaveDialog may confirm overwrite, but the existing target must stay
    // intact until staging succeeds (replace-after-success inside transport).
    std::uint64_t receivedSize = 0;
    const std::string outPath = AnsiString(dlg->FileName).c_str();
	if (!serverManager.requestFileToPath(
			SourceServerId(),
            str_command,
            cmd,
            outPath,
            nullptr,
            &receivedSize,
            &err,
            true,
            true)) {
        ShowMessage("Ошибка сохранения файла \"" + file_name + "\":\n" +
                    String(UTF8String(err.c_str())));
        return;
    }

    ShowMessage("Файл успешно сохранён:\n" + dlg->FileName +
                "\nРазмер: " + UIntToStr(receivedSize) + " байт");
}
//---------------------------------------------------------------------------

