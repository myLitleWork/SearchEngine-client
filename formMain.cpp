//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <System.JSON.hpp>
#include "stdint.h"
#include "formMain.h"
#include "formRes.h"
#include <iostream>
#include <string>
#include <regex>
#include <printers.hpp>
#include <sys/stat.h>
#include <codecvt>
#include <windows.h>
#include "myJson.h"
#include "myRSA.h"
#include "flashNum.h"
#include "FileData.h"
#include "mySomeFunc.h"
#include <System.Threading.hpp>


#pragma link "ADODataDriverEh"
#pragma link "DataDriverEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "ToolCtrlsEh"
#pragma link "PrnDbgeh"
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.FileCtrl.hpp>
#include "DbUtilsEh.hpp"
#include "SqlCondition.h"



//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ADODataDriverEh"
#pragma link "DataDriverEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "ToolCtrlsEh"
#pragma link "ADODataDriverEh"
#pragma link "ADODataDriverEh"
#pragma link "DataDriverEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "ToolCtrlsEh"
#pragma link "ADODataDriverEh"
#pragma link "DataDriverEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "ToolCtrlsEh"
#pragma link "PrnDbgeh"
#pragma link "frxClass"
#pragma link "frxADOComponents"
#pragma link "frxDBSet"
#pragma link "frxPreview"
#pragma link "frxExportBaseDialog"
#pragma link "frxExportXLS"
#pragma resource "*.dfm"



TfMain *fMain;








class PanelGuard {
public:
	explicit PanelGuard(TDBGridEh* grid)
		: grid_(grid), active_(true)
	{
		if (grid_) grid_->Enabled = false;
	  //	flag_ = true;
		Screen->Cursor = crHourGlass;
	}

	~PanelGuard() {
		if (active_) {
			if (grid_) grid_->Enabled = true;
		  //	flag_ = false;
			Screen->Cursor = crDefault;
		}
	}

	// Позволяет вручную отключить авто-восстановление
	void release() { active_ = false; }

private:
	TDBGridEh* grid_;
	bool active_;
};


struct BusyGuardAtomic {
	std::atomic_bool& flag;
	bool owns{false};

	explicit BusyGuardAtomic(std::atomic_bool& f) : flag(f) {
		// занять флаг (true) если он был false
		owns = !flag.exchange(true, std::memory_order_acquire);
	}
	~BusyGuardAtomic() {
		if (owns) flag.store(false, std::memory_order_release);
	}
	explicit operator bool() const { return owns; }
};







size_t getFileSize(const std::string& fs)
{
	struct stat stat_buf;
	int rc = stat(fs.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

#include "chekMestoMultiEkz.h"

//#include <my_json/ConverterJSON.h>
//#include <my_json/ConverterJSON.cpp>

String sqlCSstart = "DSN=SQLite3 Datasource;Database=";
String sqlCSsend = ";StepAPI=0;SyncPragma=NORMAL;NoTXN=0;Timeout=100000;ShortNames=0;LongNames=0;NoCreat=0;NoWCHAR=0;FKSupport=0;JournalMode=;OEMCP=0;LoadExt=;BigInt=0;JDConv=0;PWD=";


#include <my_json/Telega.cpp>
#include <my_json/Telega.cpp>
#define CSTRING_RES = Provider=MSDASQL.1;Persist Security Info=False;Extended Properties="DSN=SQLite3 Datasource;Database=E:\Projects-client asio_0508\Projects-client asio\Projects\Win32\Debug\Res.db3;StepAPI=0;SyncPragma=NORMAL;NoTXN=0;Timeout=100000;ShortNames=0;LongNames=0;NoCreat=0;NoWCHAR=0;FKSupport=0;JournalMode=;OEMCP=0;LoadExt=;BigInt=0;JDConv=0;"
using namespace std;
typedef std::map<std::string,std::map<std::string, Request>> mapTemplates;









void FillCheckListBoxFromDBGrid(TCheckListBox* chkField, TDBGridEh* grid) {
	chkField->Items->BeginUpdate();
	try {
		chkField->Items->Clear();
        for (int i = 0; i < grid->Columns->Count; ++i) {
			auto col = grid->Columns->Items[i];
			int idx = chkField->Items->Add(col->Title->Caption);
			chkField->Checked[idx] = col->Visible;
        }
	}
	catch(...) {
		chkField->Items->EndUpdate();
	}
}


struct ComponentsGuard {
private:
    std::atomic_bool* flag_ {nullptr};
    TfMain*           form_ {nullptr};
    bool              owns_ {false};

public:
    // Нельзя копировать (иначе будет двойной release)
    ComponentsGuard(const ComponentsGuard&)            = delete;
    ComponentsGuard& operator=(const ComponentsGuard&) = delete;

    // Конструктор: пытаемся "занять" флаг и выключить компоненты
	ComponentsGuard(std::atomic_bool& f, TfMain* frm) noexcept
        : flag_(&f), form_(frm)
    {
        owns_ = !flag_->exchange(true, std::memory_order_acquire);
        if (owns_ && form_)
			form_->componentsEnable(false);   // ВАЖНО: вызывать только из UI-потока!
    }

    // Move-конструктор: крадём владение и обнуляем источник
    ComponentsGuard(ComponentsGuard&& other) noexcept {
        steal(other);
    }

    // Move-оператор: сначала освобождаем своё, затем крадём
    ComponentsGuard& operator=(ComponentsGuard&& other) noexcept {
        if (this != &other) {
            release();
            steal(other);
        }
        return *this;
    }

    ~ComponentsGuard() noexcept {
        release();
    }

    // Явное приведение к bool — заняли ли ресурс
    explicit operator bool() const noexcept { return owns_; }

    // Ручной безопасный отпуск ресурса (симметричен конструктору)
    void release() noexcept {
        if (owns_) {
            // ВАЖНО: componentsEnable(true) следует вызывать в UI-потоке.
            // Если guard может разрушаться в фоне — перенесите этот вызов в UI,
            // а здесь оставьте только сброс флага. См. пример использования ниже.
            if (form_)
				form_->componentsEnable(true);
            if (flag_)
                flag_->store(false, std::memory_order_release);
        }
		// Полное "обнуление" чтобы деструкторы/повторные release были idempotent
		owns_ = false;
		form_ = nullptr;
		flag_ = nullptr;
	}

    // Отменить авто-release в деструкторе (иногда нужно, если отпускать будете вручную позже)
    void dismiss() noexcept { owns_ = false; }

    // Переустановить ссылку на форму (редко нужно, но бывает)
    void resetForm(TfMain* frm) noexcept { form_ = frm; }

private:
    void steal(ComponentsGuard& o) noexcept {
        flag_ = o.flag_;
        form_ = o.form_;
        owns_ = o.owns_;
        o.flag_ = nullptr;
        o.form_ = nullptr;
        o.owns_ = false;
    }
};


std::vector<String> TfMain::SelectFiles()
{
	std::vector<String> filePaths;

	// Создаем диалоговое окно для выбора файлов
	TOpenDialog *openDialog = new TOpenDialog(NULL);
	try
	{
		openDialog->Filter = "Все файлы (*.*)|*.*";  // Фильтр для всех типов файлов
		openDialog->Options << ofAllowMultiSelect;   // Включаем возможность выбора нескольких файлов
		openDialog->InitialDir = String{token_dir_.c_str()};
		// Показываем диалоговое окно
		if (openDialog->Execute())
		{
            // Получаем пути к выбранным файлам
			for (int i = 0; i < openDialog->Files->Count; ++i)
            {
				filePaths.push_back(openDialog->Files->Strings[i]);
			}
		}
	}
	__finally
	{
		delete openDialog;  // Очищаем память
	}

	return filePaths;
}
void TfMain::FillPopUpMenuWithGridColumns(TPopupMenu *popDbg, TDBGridEh *grid) {
    popDbg->Items->Clear();
    for (int i = 0; i < grid->Columns->Count; ++i) {
        TMenuItem *mi = new TMenuItem(popDbg);
        mi->Caption = grid->Columns->Items[i]->Title->Caption;
        mi->Checked = grid->Columns->Items[i]->Visible;
        mi->Tag = i; // Сохраняем индекс колонки
        mi->OnClick = GridColumnMenuClick; // Обработчик, см. ниже
        popDbg->Items->Add(mi);
    }
}

void __fastcall TfMain::GridColumnMenuClick(TObject *Sender)
{
    TMenuItem *mi = dynamic_cast<TMenuItem*>(Sender);
    if (!mi) return;

    int idx = mi->Tag;
	if (idx < 0 || idx >= dbgEh->Columns->Count)
        return;

    // Переключаем видимость
	dbgEh->Columns->Items[idx]->Visible = !dbgEh->Columns->Items[idx]->Visible;
	mi->Checked = dbgEh->Columns->Items[idx]->Visible;
}

void TfMain::pushLog(const String& s)
{
	std::lock_guard<std::mutex> lk(logMtx);
	pendingLog.push_back(s);
}


//---------------------------------------------------------------------------
__fastcall TfMain::TfMain(TComponent* Owner)
	: TForm(Owner)
{
	ConnectionRes->ConnectionString = sqlCSstart +	ExtractFileDir(Application->ExeName) + "\\Res.db3" + sqlCSsend;
	ConnectionRes->Connected = true;
	DBGridEhCenter()->FilterEditCloseUpApplyFilter = true;
	qryTelegi->SQL->Clear();
	qryTelegi->SQL->Add("delete from res");
	qryTelegi->ExecSQL();
	qryTelegi->SQL->Clear();
	qryTelegi->SQL->Add("select * from res");
	qryTelegi->Open();
	qryTelegi->Active=true;
	memTbl->Active=true;

	FillPopUpMenuWithGridColumns(popDbg, dbgEh);
}
std::vector<uint8_t> TfMain::loadFileToVector2(const std::string& filePath, const std::wstring& file_name) {
	if (filePath.empty()) {
		ShowMessage("File path is empty. Cannot open file.");
		return {};
    }

	std::wstring wfilePath = String(filePath.c_str()).c_str(); // Преобразуем std::string в std::wstring
	TFileStream* fileStream = nullptr;

    try {
        fileStream = new TFileStream(wfilePath.c_str(), fmOpenRead | fmShareDenyWrite);
	}
	catch (...) {
        ShowMessage("Failed to open the file.");
		delete fileStream;
        return {};
	}

	if (!fileStream) {
        ShowMessage("Failed to create file stream.");
		return {};
	}

	// Читаем содержимое файла в вектор
	std::vector<uint8_t> fileContent(fileStream->Size);
    fileStream->ReadBuffer(&fileContent[0], fileStream->Size);

	// Получаем имя файла из пути
	std::string baseFileNameStr = conv::to_string(file_name);

	FileData fileDataObject(baseFileNameStr, fileContent);

    std::vector<uint8_t> serializedData = serializeToBytes(fileDataObject);
  //	FileData fd = deserializeFromBytes(serializedData);

	int g = serializedData.size();

    //ShowMessage(IntToStr(g).c_str());
    //ShowMessage(IntToStr(sizeof(fd)).c_str());
    //ShowMessage(fd.getFilename().c_str());

	delete fileStream;
	return serializedData;
}
void connectLocal(TADOConnection* con_, String base_)
{
	static String sqlCSstart = "DSN=SQLite3 Datasource;Database=";
	static String sqlCSsend = ";StepAPI=0;SyncPragma=NORMAL;NoTXN=0;Timeout=100000;ShortNames=0;LongNames=0;NoCreat=0;NoWCHAR=0;FKSupport=0;JournalMode=;OEMCP=0;LoadExt=;BigInt=0;JDConv=0;PWD=";

	con_->Connected = false;

	con_->ConnectionString = sqlCSstart + base_ + sqlCSsend;

	con_->Connected = true;
}

void TfMain::InitDbgMenuDS()
{

	if (mtMenu) return;

	mtMenu = new TMemTableEh(this);
	mtMenu->FieldDefs->Clear();
	mtMenu->FieldDefs->Add(L"menu",       ftWideString, 30, false);
	mtMenu->CreateDataSet();

	dsMenu = new TDataSource(this);
	dsMenu->DataSet = mtMenu;

	dbgMenu->DataSource = dsMenu;

}

void TfMain::loadTextToEditFromServer()
{

	rchEdt->Lines->Clear();
	if(!checkUser() && useF12Control())
		return;

	AnsiString ss;

	try{
		ss = dbgEh->DataSource->DataSet->FieldValues["dir"];
	}
	catch(...){}

	if(ss == "")
		return;


	std::string err;

	if (!client_.socketWrite(std::string(AnsiString(ss).c_str()), COMMAND::FILETEXT, &err)) {
		ShowMessage("Ошибка отправки: " + String(err.c_str()));
		return;
	}

	std::string str;
	if (!client_.socketRead(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + String(err.c_str()));
		return;
	}


	auto reg = new TStringList();
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg);
	rchEdt->Text = reg->Text;
	delete reg;

	auto sl = new TStringList();
	sl->DelimitedText = cmbReq->Text;

	if(chkUpper->Checked)
		sl->DelimitedText = cmbReq->Text;
	else
		sl->DelimitedText = cmbReq->Text.UpperCase();

	for(const auto& s:sl)
		setColor(s);

}
//---------------------------------------------------------------------------
std::map<String,String> TfMain::getMapFromQry(TADOQuery* qry, const String& table_name)
{
	std::map<String,String> m;
	qry->SQL->Text = "select * from " + table_name;
	qry->Open();

	while(!qry->Eof)
	{
		auto value = qry->FieldByName("Value")->AsString;
		auto key = qry->FieldByName("Key")->AsString;
		m.insert(std::make_pair(value,key));
		qry->Next();
	}

	return std::move(m);
}

bool TfMain::useF12Control()
{

	qrySettings->SQL->Text = "select value from settings where key = 'use_f12'";
	qrySettings->Open();

	if(qrySettings->RecordCount == 0)
		return false;

	auto value = qrySettings->FieldByName("value")->AsString;

	if(value == "yes")
		return true;
	else
		return false;

}

void TfMain::registryOnServer(const std::string& user_name)
{
    const COMMAND command = COMMAND::USER_REGISTRY;
    std::string err;

    if (!client_.socketWrite(user_name, command, &err)) {
        ShowMessage("Ошибка отправки регистрации: " + String(err.c_str()));
        return;
    }

    std::string reply;
    if (!client_.socketRead(reply, /*progress*/nullptr, &err)) {
        ShowMessage("Ошибка чтения ответа регистрации: " + String(err.c_str()));
        return;
    }

    if (reply != "OK") {
        ShowMessage("Ошибка регистрации на сервере! Ответ: " + String(reply.c_str()));
    }
}

void __fastcall TfMain::finishSearch()
{
	if (!searchSuccess) {
        ShowMessage(String(searchError.c_str()));
		btnSearch->Enabled = true;
        return;
    }
    telegi = telegi_async;
	ggSaveProgress->Progress = ggSaveProgress->MaxValue;
	if(telegi.size() > 0)
		convTelegs(telegi);
    // ... всё остальное из Synchronize
    cmbReq->SelStart = 0;
	cmbReq->SelLength = cmbReq->Text.Length();


	qryTelegi->SQL->Text = ("select * from res");
	qryTelegi->Open();

	memTbl->Active = false;
	memTbl->Active = true;

    dbgEh->Columns->Items[4]->Title->SortMarker = smDownEh;
    dbgEh->DefaultApplySorting();

	int gg = dbgEh->RowCount;

	if(dbgEh->RowCount == 2) {
		dbgEh->SelectedRows->CurrentRowSelected = true;
		dbgEh->OnCellClick((TColumnEh*)lastSender);
	}
	static_cast<TButton*>(lastSender)->Enabled = true;
	dbgEh->Enabled = true;

	if(cmbReq->CanFocus())
	cmbReq->SetFocus();
}





void __fastcall TfMain::btnSearchClick(TObject *Sender)
{

	auto tok = std::make_shared<ComponentsGuard>(gSearchProcessing, this);
	if (!*tok) return;


   //	ComponentsGuard guard2(gSaveProcessing, this);
  //	if (!guard2) return;


    last_loaded_num_ = "";

	if (cmbReq->Text == "") {
        return;
	}

	ComboHistory.add(AnsiString{cmbReq->Text}.c_str());
	clear();

    static_cast<TButton*>(Sender)->Enabled = false;
    dbgEh->RowDetailPanel->Visible = false;

    // --- Собираем копии нужных полей для лямбды ---
    AnsiString s;
    if (chkUpper->Checked || chkSQL->Checked)
        s = cmbReq->Text;
    else
        s = cmbReq->Text.UpperCase();

    if (chkOEM->Checked)
        AnsiToOem(s.c_str(), s.c_str());

	String cmbFieldText = cmbField->Text;
    int rgTypeTIndex = rgTypeT->ItemIndex;
    int cmbMaxReqInt = 0;
    try { cmbMaxReqInt = StrToInt(cmbMaxReq->Text); } catch (...) {}
	bool isChkSQL = chkSQL->Checked;
    bool isChkExc = chkExc->Checked;

    // --- Запуск фоновой задачи ---
	System::Threading::TTask::Run([this, tok, s, isChkSQL, Sender, rgTypeTIndex, cmbMaxReqInt, cmbFieldText, isChkExc]() {
		std::string d_local;
		COMMAND command_local;
		AnsiString s_local = s;

		// Логика выбора команды и параметров (оставь свою!)
        if (isChkSQL) {
            SqlCondition::TYPE type;
			if (rgTypeTIndex == 0) {
                command_local = COMMAND::GET_VH_TELEGI_FROM_SQL;
                type = SqlCondition::TYPE::VH;
            } else {
                command_local = COMMAND::GET_ISH_TELEGI_FROM_SQL;
                type = SqlCondition::TYPE::ISH;
            }
			SqlCondition::max = cmbMaxReqInt;
			if (cmbFieldText == "SQL") {
                s_local += " order by `index` desc limit " + IntToStr(SqlCondition::max);
                d_local = conv::to_string(String(s_local).c_str());
            } else {
				auto myFunc = SqlCondition::getFuncCondition(type, isChkExc);
                d_local = conv::to_string(myFunc(cmbFieldText, s_local).c_str());
            }
        } else {
            command_local = COMMAND::GETSQLJSONANSWEAR;
            d_local = AnsiString(s_local).c_str();
        }

		std::string str;
        // --- Работа с сетью и разбор ответа ---
		try {
			std::string err;

			// 1) отправка
			if (!client_.socketWrite(d_local, command_local, &err)) {
				this->searchSuccess = false;
				this->searchError   = "Ошибка отправки данных на сервер: " + my_utils::toAnsi(err);
				this->lastSender    = Sender;
				TThread::Queue(nullptr, [this, tok]() { finishSearch(); });
				return;
			}

			// 2) чтение ответа
			if (!client_.socketRead(str, /*progress*/nullptr, &err)) {  // или &rxProgress_
				this->searchSuccess = false;
				this->searchError   = "Ошибка получения данных от сервера: " + my_utils::toAnsi(err);
				this->lastSender    = Sender;
				TThread::Queue(nullptr, [this, tok]() { finishSearch(); });
				return;
			}

			// дальше используйте str

        } catch (const std::exception& e) {
			this->searchSuccess = false;
			this->searchError = e.what();
			this->lastSender = Sender;
			TThread::Queue(nullptr, [this, tok]() {
			finishSearch();
			});
            return;
		}

		if (str == "update") {
			this->searchSuccess = false;
			this->searchError = "Идет обновление! Повторите запрос позже!";
			this->lastSender = Sender;
			TThread::Queue(nullptr, [this, tok]() { finishSearch(); });
            return;
        }

        try {
			this->telegi_async = JSON::get<std::vector<Telega>>(str);
			this->searchSuccess = true;
			this->searchError.clear();
            this->lastSender = Sender;
		} catch (...) {
			this->searchSuccess = false;
			this->searchError = "Ошибка разбора данных!";
            this->lastSender = Sender;
            // Можно не дублировать вызов, сделать ниже (но для надёжности лучше всегда вызывать сразу)
        }
		// --- Завершение: возврат в UI-поток
		TThread::Queue(nullptr, [this, tok]() {
		finishSearch();
		});
	});
}


//---------------------------------------------------------------------------
void __fastcall TfMain::FormClose(TObject *Sender, TCloseAction &Action)
{

	auto safeRelease = [this](auto* &fr) {

		if (fr) {
			fr->Detach();        // доступно, потому что все наследники TFrameDbg
			fr->Parent = nullptr;
			delete fr;
			fr = nullptr;
		}

	};

	safeRelease(myFrAttachments);
	safeRelease(myFrPdtv);
	safeRelease(myFrWay);

	WordServer::Cleanup();
}

void TfMain::clear()
{


	qryTelegi->SQL->Text = ("delete from res");
	qryTelegi->ExecSQL();


	qryTelegi->SQL->Text = ("select * from res");
	qryTelegi->Open();

	memTbl->Active = false;
	memTbl->Active = true;

	rchEdt->Text = "";
	ggSaveProgress->Progress = 0;
	Label1->Caption = "Relative index:";
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void __fastcall TfMain::btnConnectClick(TObject *Sender)
{

	//AnsiString port, host;
   /*
	if(edtPort->Text == "")
	{
	   //	edtPort->Text = port ;
	}
	else
	{
		port = edtPort->Text;
	}
	*/

	//port = mapPorts.at(cmbGod->Text);
   //	host = Edit2->Text;

	lbStatus->Caption = "Статус: подключение к серверу " + Edit2->Text + ":" + mapPorts.at(cmbGod->Text) + " " + cmbGod->Text + " год ...";


	qryTelegi->SQL->Text = ("delete from res");
	qryTelegi->ExecSQL();


	qryTelegi->SQL->Text = ("select * from res");
	qryTelegi->Open();

	memTbl->Active = false;
	memTbl->Active = true;


	clear();

	auto port = mapPorts.at(cmbGod->Text);
	auto host = Edit2->Text;
	AnsiString err;

	bool connecting = client_.tryConnect(host,port,err);

	if(tryConnect() && auth_)
	{
		registryOnServer(user_);
	}

	Application->ProcessMessages();


    return;

	std::wstring gr2 = L"2 отделение";
	std::wstring gr4 = L"4 группа";
	std::wstring gr10 = L"10 группа";
	std::wstring adm = L"admin";

	name_gr["Sherstobitov"] = conv::to_string(gr2);
	name_gr["Petrakov"] = conv::to_string(gr2);
	name_gr["Penzin"] = conv::to_string(gr2);
	name_gr["Serikov"] = conv::to_string(gr2);
	name_gr["Gorbachev"] = conv::to_string(gr2);

	name_gr["Tarasov"] = conv::to_string(adm);
	name_gr["Nemcov"] = conv::to_string(gr2);

	name_gr["Kulkin"] = conv::to_string(gr4);
	name_gr["Lebedev"] = conv::to_string(gr4);
	name_gr["Smetanin"] = conv::to_string(gr4);
	name_gr["Krechetnikov"] = conv::to_string(gr4);
	name_gr["Zverev"] = conv::to_string(gr4);

	name_gr["Gavrilov"] = conv::to_string(gr10);

	/*
	std::wstring kulk = L"т. Кулькину Н.А.";
	name_solo["Kulkin"] = conv::to_string(kulk);

	std::wstring penzin = L"т. Пензину А.Д.";
	name_solo["Penzin"] = conv::to_string(penzin);
	*/

	{
		std::ifstream jsonFile("groups.json");
		grp = nh::json::parse(jsonFile).get<std::map<std::string, std::set<std::string>>>();
		jsonFile.close();
	}

	{
		std::ifstream jsonFile("user_group.json");
		name_gr = nh::json::parse(jsonFile).get<std::map<std::string, std::string>>();
		jsonFile.close();
	}


	{
		std::ifstream jsonFile("user_group.json");
		name_solo = nh::json::parse(jsonFile).get<std::map<std::string, std::string>>();
		jsonFile.close();
	}



	/*
	while(!qry2->Eof)
	{

		std::wstring gr = qry2->FieldByName("Group")->AsString.c_str();
		std::wstring fio = qry2->FieldByName("full_value")->AsString.c_str();
		std::string gr_s = conv::to_string(gr);
		std::string fio_s = conv::to_string(fio);

		grp[gr_s].insert(fio_s);



		qry2->Next();
	}

	{
	std::ofstream jsonFile("groups.json");
	nh::json j = grp;
	jsonFile << std::setw(2) << j;
	jsonFile.close();
	}

    {
	std::ofstream jsonFile("user_group.json");
	nh::json j = name_gr;
	jsonFile << std::setw(2) << j;
	jsonFile.close();
	}
    */

}
//---------------------------------------------------------------------------

void TfMain::disConnect()
{
	client_.disConnect();  // безопасно: не бросает, закрывает всё внутри

    PageControl1->ActivePageIndex = 0;
    lbStatus->Caption = "Статус: отключено от сервера";
    componentsEnable(false);
}



//---------------------------------------------------------------------------



//---------------------------------------------------------------------------

void __fastcall TfMain::cmbReqChange(TObject *Sender)
{

	return;

	if(cmbReq->Items->Count == 0)
		return;

	clear();
	ListBox1->Clear();
	String req = cmbReq->Text;

    if(mapAnswers.find(req) != mapAnswers.end())
		myMap = mapAnswers.at(req);
	else
		return;

	std::vector<String> bufVec;
	for(const auto& i:mapAnswers.at(req))
		bufVec.push_back(i.first);

	std::sort(bufVec.begin(),bufVec.end(),[this, &req](const auto& i1,const auto& i2)
	{
		return mapAnswers.at(req).at(i2) < mapAnswers.at(req).at(i1);
	});

	for(const auto& i:bufVec)
		ListBox1->Items->Add(i);

}
//---------------------------------------------------------------------------

void __fastcall TfMain::cmbReqKeyUp(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if(Key == VK_RETURN && btnSearch->Enabled)
		btnSearch->Click();

}
//---------------------------------------------------------------------------

void __fastcall TfMain::tmFilterEnableAlarmTimer(TObject *Sender)
{
	for(const auto& i : dbgEh->Columns)
	{
	  if(i->STFilter->ExpressionStr != "")
	  {
		lbFilter->Visible = true;
		return;
	  }
	}

	lbFilter->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TfMain::componentsEnable(bool b)
{
	   tmLoadText->Enabled = b;
	   //	rchEdt->Enabled = b;
		ListBox1->Enabled  = b;
	  //	ggSaveProgress->Enabled = b;
		btnSearch->Enabled = b;
		cmbReq->Enabled = b;
		Label1->Enabled = b;
		Button3->Enabled = b;
		Button1->Enabled = b;
		btnSave->Enabled = b;
		btnUpdate->Enabled = b;
		btnAddFilesLoad->Enabled = b;
		btnLoadOnServerFiles->Enabled = b;
		btnSaveOpisBase->Enabled = b;
		btnAttachSave->Enabled = b;
		btnLoadRazn->Enabled = b;
		lstShGroups->Enabled = b;
		lstTemplates->Enabled = b;
		btnLoadRazn->Enabled = b;
		btnAttachSave->Enabled = b;
		btnSaveOpisBase->Enabled = b;
		btnAddFilesLoad->Enabled = b;
		btnLoadOnServerFiles->Enabled = b;
		dbgEh->Enabled = b;


}


//---------------------------------------------------------------------------
void __fastcall TfMain::Button6Click(TObject *Sender)
{
   /*	lstDocs->Items->Clear();
	socketWrite("PRIVET", COMMAND::UPDATE);

	std::string str;

	if(!socketRead(str))
			return;

	std::vector<std::string> vDoc{};
	std::map<std::string, std::string> mExcsBuff;

	nh::json jsonResolutions;
	jsonResolutions = nh::json::parse(str);

	jsonResolutions.at("docs").get_to(vDoc);
	jsonResolutions.at("execs").get_to(mExcsBuff);

	for(const auto& s:vDoc)
		lstDocs->Items->Add(s.c_str());

	fRes->myExcs.clear();
	for(const auto& s:mExcsBuff)
	{
		String w1 = conv::to_wstring(s.first).c_str();
		String w2 = conv::to_wstring(s.second).c_str();

		fRes->myExcs.insert(std::make_pair(w1,w2));
	}
		 */

}
//---------------------------------------------------------------------------

inline UnicodeString US(const std::string& s){ return AnsiString(s.c_str()); }


//---------------------------------------------------------------------------
void TfMain::convTelegs(std::vector<Telega>& telegi)
{
    static const String SQL_INSERT_RES =
        "insert into res ("
        "num, date, podp_num, date_podp, tel_num, isp, from_to, rel, dir, type, kr, "
        "pril_name, pril_count, last_mesto, gde_sht, blank"
        ") values ("
        ":num, :date, :podp_num, :date_podp, :tel_num, :isp, :from_to, :rel, :dir, :type, :kr, "
        ":pril_name, :pril_count, :last_mesto, :gde_sht, :blank"
        ")";

    qryTelegi->Close();
    qryTelegi->SQL->Text = SQL_INSERT_RES;

    // типы параметров
    auto P = qryTelegi->Parameters;
    P->ParamByName("num")->DataType        = ftString;
    P->ParamByName("podp_num")->DataType   = ftString;
    P->ParamByName("tel_num")->DataType    = ftString;
    P->ParamByName("isp")->DataType        = ftString;
    P->ParamByName("from_to")->DataType    = ftString;
	P->ParamByName("dir")->DataType        = ftString;
	P->ParamByName("type")->DataType 	   = ftWideString;
	P->ParamByName("kr")->DataType         = ftString;
    P->ParamByName("pril_name")->DataType  = ftString;
    P->ParamByName("pril_count")->DataType = ftString;
    P->ParamByName("last_mesto")->DataType = ftString;
    P->ParamByName("gde_sht")->DataType    = ftString;
    P->ParamByName("blank")->DataType      = ftString;

    P->ParamByName("rel")->DataType        = ftInteger;
    P->ParamByName("date")->DataType       = ftDate;
    P->ParamByName("date_podp")->DataType  = ftDate;

    qryTelegi->Prepared = true;

    ggSaveProgress->MaxValue = telegi.size();
    ggSaveProgress->Progress = 0;

    TADOConnection* conn = qryTelegi->Connection;
    bool tranStarted = false;

    try { conn->BeginTrans(); tranStarted = true; }
    catch (const Exception& e) { ShowMessage(L"Не удалось начать транзакцию: " + e.Message); }

    auto normalize_gde_sht = [](std::string s)
    {
        if (std::none_of(s.begin(), s.end(), ::isdigit))
            s.clear();
        return s;
    };

    for (auto& t : telegi)
    {
        // не модифицируй исходные поля без нужды, но раз уж делаешь:
        std::replace(t.from_to.begin(), t.from_to.end(), '\'', '"');
        std::replace(t.kr.begin(),      t.kr.end(),      '\'', '"');

        const String type = (t.type == Telega::TYPE::VHOD) ? L"Вх." : L"Исх.";

        try
        {
            // date_podp
            try {
				TDateTime d2 = StrToDate(US(t.date_podp));
                P->ParamByName("date_podp")->Value = d2;   // ✅ ДАТА КАК ДАТА
            } catch (...) {
                P->ParamByName("date_podp")->Value = Null();
            }

            // date
            try {
                TDateTime d1 = StrToDate(US(t.date));
                P->ParamByName("date")->Value = d1;        // ✅ ДАТА КАК ДАТА
            } catch (...) {
                P->ParamByName("date")->Value = Null();
            }

            // строки — БЕЗ c_str()
            P->ParamByName("num")->Value        = US(t.num);
            P->ParamByName("podp_num")->Value   = US(t.podp_num);
            P->ParamByName("tel_num")->Value    = US(t.tel_num);
            P->ParamByName("isp")->Value        = US(t.isp);
            P->ParamByName("from_to")->Value    = US(t.from_to);
            P->ParamByName("rel")->Value        = (int)t.rel;
            P->ParamByName("dir")->Value        = US(t.dir);
			P->ParamByName("type")->Value       = type;
            P->ParamByName("kr")->Value         = US(t.kr);
            P->ParamByName("pril_name")->Value  = US(t.pril_name);
            P->ParamByName("pril_count")->Value = US(t.pril_count);
            P->ParamByName("last_mesto")->Value = US(t.last_mesto);
            P->ParamByName("gde_sht")->Value    = US(normalize_gde_sht(t.gde_sht));
            P->ParamByName("blank")->Value      = US(t.blank);

            qryTelegi->ExecSQL();
        }
        catch (const Exception& e)
        {
            ShowMessage(
                L"Ошибка вставки:\n"
                L"Тип: " + type +
                L"\nНомер: " + US(t.num) +
                L"\nПодписной номер: " + US(t.podp_num) +
                L"\n\n" + e.Message
            );
        }

        ggSaveProgress->AddProgress(1);
        Application->ProcessMessages();
    }

    if (tranStarted)
    {
        try { conn->CommitTrans(); }
        catch (...) { conn->RollbackTrans(); throw; }
    }

    ggSaveProgress->Progress = ggSaveProgress->MaxValue;
}




void __fastcall TfMain::FormCreate(TObject *Sender)
{
	InitDbgMenuDS();

	ComboHistory = RecentComboHistory(cmbReq,9);
	myRequest::init(cmbField, cmbReq, chkSQL, chkExc, rgTypeT);

	SqlCondition::map =    getMapFromQry(qrySettings, "map");
	SqlCondition::mapVh =  getMapFromQry(qrySettings, "map_vh");
	SqlCondition::mapIsh = getMapFromQry(qrySettings, "map_ish");

	auto sl = new TStringList();

	try
	{
		sl->LoadFromFile("SERVER_IP.INI");
		Edit2->Text = sl->Strings[0];
	}
	catch(...)
	{
		ShowMessage("Файл \"SERVER_IP.INI\" - не найден\nили имеет не верную структуру!\n\
		Пример верного содержания - \"192.168.0.1\"");
		Edit2->Text = "localhost";
		ShowMessage("Адрес сервера выбран по умолчанию как localhost.");
	}

	delete sl;

	qrySettings->Active = false;
	qrySettings->SQL->Text = "select * from ports";
	qrySettings->Active = true;

	while(!qrySettings->Eof)
	{
		auto port = qrySettings->FieldByName("port")->AsString;
		auto god = qrySettings->FieldByName("god")->AsString;
		mapPorts.insert(std::make_pair(god,port));
		cmbGod->Items->Add(god);
		qrySettings->Next();
	}

	cmbGod->ItemIndex = 0;



	if(FileExists("Templates.json"))
		myRequest::loadTemplates(JSON::getFromFile<mapTemplates>("Templates.json"));

	for(const auto& i : myRequest::groupsNames())
		lstShGroups->Items->Add(i);


	if(!useF12Control())
		dbgEh->RowDetailPanel->Active = false;


   //	return;

	myFrAttachments = new TfrAttachments(nullptr, client_, dbgEh, dbgMenu);
	myFrAttachments->Parent = dbgMenu->RowDetailPanelControl;
	myFrAttachments->setHeigthMulti(0.7);
	myFrAttachments->Align = alClient;
	myFrAttachments->Visible = false;



	myFrWay = new TfrWay(nullptr, client_, dbgEh, dbgMenu);
	myFrWay->Parent = dbgMenu->RowDetailPanelControl;
	myFrWay->setHeigthMulti(1.6);
	myFrWay->Align = alClient;
	myFrWay->Visible = false;


	myFrPdtv  = new TfrPdtv(nullptr, client_, dbgEh, dbgMenu);
	myFrPdtv->Parent = dbgMenu->RowDetailPanelControl;
	myFrPdtv->setHeigthMulti(1.4);
	myFrPdtv->Align = alClient;
	myFrPdtv->Visible = false;




}
//--------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TfMain::setColor(String sS)
{

	{
	int pos = 0;
	for (int k = 0; k < rchEdt->Lines->Count; k++)
		{
		if (rchEdt->Lines->Strings[k].Pos(sS) != 0)
			{
			//выделение текста
			rchEdt->SelStart = pos + rchEdt->Lines->Strings[k].Pos(sS) - 1;
			rchEdt->SelLength = sS.Length();
			rchEdt->SelAttributes->Color = clRed;
			rchEdt->SelAttributes->Style = rchEdt->SelAttributes->Style << fsBold;
			}
		pos = pos + rchEdt->Lines->Strings[k].Length() + 1;
		}
	}

}
void __fastcall TfMain::dbgEhColumns0CellButtons0Click(TObject *Sender, bool &Handled)
{
    clear();

    const String ss = dbgEh->Fields[9]->AsString;
    if (ss.IsEmpty()) {
        Handled = true;
        return;
    }

    std::string err;

    // 1) отправляем запрос FILETEXT
    if (!client_.socketWrite(std::string(AnsiString(ss).c_str()), COMMAND::FILETEXT, &err)) {
        ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
        Handled = true;
        return;
    }

    // 2) читаем ответ
    std::string str;
    if (!client_.socketRead(str, /*progress*/nullptr, &err)) {
        ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
        Handled = true;
        return;
    }

    // 3) перекодировка и вывод в редактор
    std::unique_ptr<TStringList> reg(new TStringList());
    reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg.get());
    rchEdt->Text = reg->Text;

    // 4) подсветка по словам из cmbReq
    std::unique_ptr<TStringList> sl(new TStringList());
    sl->DelimitedText = cmbReq->Text;
    for (const auto& s : sl.get())
        setColor(s);

    Handled = true;
}

//---------------------------------------------------------------------------

void __fastcall TfMain::PageControl1Change(TObject *Sender)
{


	if (PageControl1->ActivePage->Name == "tbSettings") {


        try {
            // Получаем список таблиц из базы данных
            qrySettings->Close();
            qrySettings->SQL->Text =
                "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";
            qrySettings->Open();

            // Очищаем вкладки TabControl перед заполнением
            tbcSettings->Tabs->Clear();

            // Заполняем вкладки именами таблиц
            while (!qrySettings->Eof) {
                String tableName = qrySettings->FieldByName("name")->AsString;
                tbcSettings->Tabs->Add(tableName);
                qrySettings->Next();
            }

           // ShowMessage("Вкладки заполнены таблицами базы данных.");
        } catch (const Exception &e) {
            ShowMessage("Ошибка при загрузке таблиц: " + e.Message);
		}

        tbcSettingsChange(Sender);
    }
}




bool TfMain::checkUser()
{
   //  ShowMessage(user.c_str());
 //	ShowMessage(name_gr.at(user).c_str());

     return true;
	if(user_.empty() || name_gr.find(user_) == name_gr.end())
		return false;

	if(name_gr.at(user_) == "admin")
		return true;

	String podp_num = dbgEh->Fields[3]->AsString;

	if( podp_num.Pos("/П") == podp_num.Length()-1 )
		return true;


	auto names = grp.at(name_gr.at(user_));



	COMMAND command;
	auto number = dbgEh->Fields[0]->AsString;

	auto type = dbgEh->Fields[7]->AsString;
	int type_int = 0;

	if(type == "Вх.")
	{
		type_int = 1;
		command = COMMAND::GET_VH_TELEGA_WAY;
	}
	else if(type == "Исх.")
	{
		type_int = 2;
		command = COMMAND::GET_ISH_TELEGA_WAY;
	}
	else
	{
	   //	ShowMessage("Неизвестный тип шт!");
		return false;
	}

	std::list<std::map<std::string,std::string>> result_way{};


		std::string err;

	// 1) готовим данные
	std::string d = std::string(AnsiString(number).c_str());

	// 2) отправляем
	if (!client_.socketWrite(d, command, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return false;
	}

	// 3) читаем
	std::string str;
	if (!client_.socketRead(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
		return false;
	}

	// если дошли сюда — всё ок


	result_way = JSON::get<std::list<std::map<std::string,std::string>>>(str);
  //	ShowMessage(user.c_str());
  //	ShowMessage(name_gr.at(user).c_str());

	std::string solo_name;
	if(name_solo.find(user_) != name_solo.end())
		solo_name = name_solo.at(user_);

	for(const auto& m : result_way)
	{
		for(const auto name : names)
		{
			auto kuda = m.at("kuda");

			if(kuda == name || kuda == solo_name)
				return true;
		}
	}


	return false;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TfMain::N1Click(TObject *Sender)
{
	frxReport->ShowReport();
}
//---------------------------------------------------------------------------
void __fastcall TfMain::dbgEhColumns6CellButtons0Click(TObject *Sender, bool &Handled)
{
	clear();

	auto ss = dbgEh->Fields[3]->AsString;

	if(ss == "")
		return;

	cmbReq->Text = ss;
	if(btnSearch->CanFocus())
		btnSearch->SetFocus();

}
//---------------------------------------------------------------------------
void __fastcall TfMain::dbgEhCellClick(TColumnEh *Column)
{
	if(!save_pocessing)
   		tmLoadText->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TfMain::cmbGodChange(TObject *Sender)
{
	cmbGod->Enabled = false;

	static String last_base;

	try
	{
		if(last_base != cmbGod->Text && auth_)
		{
			last_base = cmbGod->Text;
			btnConnectClick(Sender);
		}
	}
	catch(...){};

	cmbGod->Enabled = true;
}


int TfMain::SaveFileVh2(const std::string& out_dir,
					   const std::pair<std::string, std::string>& f)
{
	std::string err;

	// отправка
	if (!client_.socketWrite(f.second, COMMAND::GETBINFILE, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return 0;
	}

	// чтение заголовка
	header hdr{};
	if (!client_.readHeader(hdr, &err)) {
		ShowMessage("Ошибка чтения заголовка: " + AnsiString(err.c_str()));
		return 0;
	}

	if (hdr.command == COMMAND::SOMEERROR || hdr.size == 0) {
		return 0;
	}


    std::string file_name = f.first;
    std::string full_file_name = out_dir + "\\" + file_name;
	std::string pref;
    int try_limit = 1;

    // Проверка на существование файла
	if (FileExists(String{full_file_name.c_str()})) {


        do {
            pref = "(" + std::to_string(try_limit++) + ") ";
            file_name = pref + f.first;
            full_file_name = out_dir + "\\" + file_name;
        }
        while (FileExists(String{full_file_name.c_str()}) && try_limit <= 30);

        if (try_limit > 30) {
            ShowMessage("Ошибка при генерации имени файла!");
            return 0;
        }
    }
	ggSaveProgress->Progress = 0;
	// === Основная часть ===
	ggSaveProgress->MaxValue = hdr.size;

	SocketFileSaver saver(client_.getSocket(), full_file_name, hdr.size, 64 * 1024,
			[this](std::size_t bytes) {
				ggSaveProgress->AddProgress(bytes);
				Application->ProcessMessages();
			});

		err = "";

        if (!saver.saveToFile(err)) {
            rchEdt->Lines->Add(err.c_str());
            return 0;
        }

        std::string result;
        if (file_name == f.first)
            result = "\"" + f.first + "\"";
		else
			result = "\"" + f.first + "\" (\"" + file_name + "\")";

		rchEdt->Lines->Add(("Файл " + result + ", " + std::to_string(hdr.size) + " байт - успешно сохранен!").c_str());


    return 1;
}



int TfMain::SaveFileVh(const std::string& out_dir,
					   const std::pair<std::string, std::string>& f,
					   bool makeDocx)
{

	std::string err;

	std::vector<uint8_t> str;

		if (!client_.getSocket().is_open()) {
		pushLog("socket is NOT open");
		return 0;
	}



	if (!client_.requestBinFile(f.second, str, nullptr, &err) || str.empty()) {
		// тут НЕ ShowMessage / НЕ rchEdt из фона
		pushLog("Файл \"" + String(f.second.c_str()) + "\" не получен: " + String(err.c_str()));
		return 0;
	}



	std::string file_name = f.first;
	std::string full_file_name = out_dir + "\\" + file_name;
	std::string pref;
	int try_limit = 1;

	if (!makeDocx && FileExists(String{full_file_name.c_str()}))
	{
		auto exist_file_size = getFileSize(full_file_name);

		if (exist_file_size == str.size())
		{
			pushLog(("Файл \"" + file_name + "\", " + std::to_string(str.size()) + " байт - уже существует!").c_str());
			return 1;
		}

        do {
            pref = "(" + std::to_string(try_limit++) + ") ";
            file_name = pref + f.first;
            full_file_name = out_dir + "\\" + file_name;
		}
        while (FileExists(String{full_file_name.c_str()}) && try_limit <= 30);

        if (try_limit > 30)
        {
            ShowMessage("Ошибка при генерации имени файла!");
            return 0;
        }
    }

	if (makeDocx)
    {
        try {
            String shtPath = ExtractFilePath(Application->ExeName) + "sht.docm";
            String outDocx = ChangeFileExt(String(out_dir.c_str()) + "\\" + String(f.first.c_str()), ".docx");

            // Преобразуем байты в текст, применяя OemToAnsi
            AnsiString raw((const char*)&str[0], str.size());
            OemToAnsi(raw.c_str(), raw.c_str());
            raw = raw.UpperCase();

            TStringList* sl = new TStringList();
            sl->Text = raw;
            String text = sl->Text;
			delete sl;


            auto* ws = WordServer::getInstance();   // singleton

				ProcessPlainText_WS(*ws,             // WordServer
                    text,
                    L"Обработка_ШТ_2022",
                    outDocx,
					shtPath);

				pushLog("DOCX создан: " + outDocx);





		}
        catch (const Exception &e) {
			pushLog("Ошибка при создании DOCX: " + e.Message);
            return 0;
		}
    }
    else
	{
        FILE* file = fopen(full_file_name.c_str(), "wb");

		if (file)
        {
            fwrite(&str[0], sizeof(BYTE), str.size(), file);
            fclose(file);
		}
		else
		{
			pushLog(("Не удалось открыть файл для записи: " + String(full_file_name.c_str())).c_str());
			return 0;
        }

        std::string result;
        if (file_name == f.first)
            result = "\"" + f.first + "\"";
		else
            result = "\"" + f.first + "\" (\"" + file_name + "\")";

		pushLog(("Файл " + result + ", " + std::to_string(str.size()) + " байт - успешно сохранен!").c_str());
    }

	return 1;
}


int TfMain::SaveFileIsh(const std::string& out_dir, const std::pair<String, std::vector<String>>& f)
{
		// Отправляем запрос на получение бинарного файла
	AnsiString zip_dir = f.first;
	std::string err;

	// 1) отправляем запрос
	if (!client_.socketWrite(std::string(zip_dir.c_str()), COMMAND::GETBINFILE, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return 0;
	}

	// 2) читаем бинарный ответ
	std::vector<uint8_t> str;
	if (!client_.socketReadBin(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
		return 0;
	}

	if (str.empty()) {
		pushLog("Файл \"" + zip_dir + "\" - не был найден!");
		return 0;
	}


	static int cc = 0;
	++cc;

	AnsiString temp_zip_dir = "ZipExtractTemp" + IntToStr(cc) + "\\";
	AnsiString zip_name = ExtractFileName(f.first);
	AnsiString tempDir = TPath::GetTempPath() + temp_zip_dir;
	AnsiString fullDirZip = tempDir + zip_name;
	TDirectory::CreateDirectory(tempDir);

	FILE* file = fopen((fullDirZip).c_str(), "wb");

	if (file)
	{
		fwrite(&str[0], sizeof(BYTE), str.size(), file);
		fclose(file);
	}
	else
	{
		pushLog("Не удалось открыть файл для записи: " + tempDir);
		return 0; // Неудача
	}

	if(cc > 99999)
		cc = 0;

	copyFile(f.second, fullDirZip, String{out_dir.c_str()});

	TDirectory::Delete(tempDir, true);

	pushLog(("Файл " + std::string{zip_name.c_str()} + ", " + std::to_string(str.size()) + " байт - успешно сохранен и распакован!").c_str());

	return 1; // Успех
}

//---------------------------------------------------------------------------


void __fastcall TfMain::btnSaveClick(TObject *Sender)
{

	ComponentsGuard compGuard(gSaveProcessing, this);
    if (!compGuard) return;



    if (dbgEh->SelectedRows->Count == 0) {
        ShowMessage("Ничего не выбрано!");
        return;
    }

    if (!chkPril->Checked && !chkText->Checked && !chkDocx->Checked) {
        ShowMessage("Не выбрано что сохранять!");
        if (grpSaveOptions->CanFocus())
            grpSaveOptions->SetFocus();
        return;
    }

    std::string out_dir;
    if (edtSaveDir->Text == "") {
        String f;
		SelectDirectory("Выберите папку для сохранения файлов", "пк", f, TSelectDirExtOpts(), 0);
        out_dir = AnsiString(f).c_str();
        edtSaveDir->Text = f;
    } else {
        out_dir = AnsiString(edtSaveDir->Text).c_str();
    }

	if (out_dir.empty())
        return;

	if (!DirectoryExists(out_dir.c_str())) {
        ShowMessage(String(out_dir.c_str()) + " - дирректория не существует!");
        edtSaveDir->Text = "";
        return;
    }

	// ===== подготовка данных (в UI потоке) =====

	saveRunning.store(true, std::memory_order_release);

	std::map<std::string, std::string> pril_map_vh;
    std::map<std::string, std::string> text_map;
	std::map<String, std::vector<String>> pril_map_ish;

	std::string out_dir_async = out_dir;

    if (dbgEh->SelectedRows->Count > 1 && chkMakeDir->Checked) {
        std::string dir_zapr = AnsiString(cmbReq->Text).c_str();
        std::replace(dir_zapr.begin(), dir_zapr.end(), '/', '-');
        out_dir_async = out_dir_async + "\\" + dir_zapr + "\\";
        CreateDir(out_dir_async.c_str());
    }

    rchEdt->Lines->Clear();

    TStringList* preLog = new TStringList(); // только для подготовки (UI поток)

	for (int i = 0; i < dbgEh->SelectedRows->Count; i++) {
        namespace fs = std::filesystem;

        dbgEh->DataSource->DataSet->GotoBookmark(dbgEh->SelectedRows->Items[i]);

        auto getPrilCount = [this]() {
            String pril_count = dbgEh->DataSource->DataSet->FieldValues["pril_count"];
            try { return StrToInt(pril_count); } catch (...) { return 0; }
        };

		int pril_count = getPrilCount();

		String num  = dbgEh->DataSource->DataSet->FieldValues["num"];
        String type = dbgEh->DataSource->DataSet->FieldValues["type"];

        if (pril_count == 0 && !(chkText->Checked || chkDocx->Checked)) {
            preLog->Add("Шт. " + num + " - не содержит приложений!");
            continue;
        }

        if (!checkUser() && useF12Control()) {
            preLog->Add(num + " - нет доступа к данной шт!");
            continue;
        }

        AnsiString dd = dbgEh->DataSource->DataSet->FieldValues["dir"];
        std::string dir_file_name = dd.c_str();

        if (chkPril->Checked && getPrilCount()) {
            auto getPrilsName = [this]() {
                using namespace std;
                AnsiString s = dbgEh->DataSource->DataSet->FieldValues["pril_name"];
                string str = s.c_str();
                regex regex{"[;]"};
                sregex_token_iterator first{str.begin(), str.end(), regex, -1}, last;
                return vector<string>{first, last};
            };

            auto dir = fs::path{dir_file_name.c_str()}.parent_path().string();
            for (const auto& pn : getPrilsName()) {
                auto ss = dir + "\\" + pn;
                std::string dir_file_name_pril = ss.c_str();
                if (type == "Вх.")
                    pril_map_vh.insert(std::make_pair(pn, dir_file_name_pril));
                else {
                    auto ish_zip_dir = ExtractFileDir(dd) + "\\" + num + ".zip";
                    pril_map_ish[ish_zip_dir].push_back(String{pn.c_str()});
				}
            }
        }

        if (chkText->Checked || chkDocx->Checked) {
			fs::path filePath = fs::path{dir_file_name.c_str()}.filename();
            if (filePath.extension().empty())
                filePath.replace_extension(".tlg");
            text_map.insert(std::make_pair(filePath.string(), dir_file_name));
        }
	}

    const int total_file_count = (int)(pril_map_vh.size() + pril_map_ish.size() + text_map.size());
    if (!total_file_count) {
        delete preLog;
        return;
    }

    // ===== старт прогресса (UI поток) =====
    saveFilesDone.store(0, std::memory_order_relaxed);
    saveFilesTotal.store((uint32_t)total_file_count, std::memory_order_relaxed);


    {
		std::lock_guard<std::mutex> lk(log_mtx);
		pendingLog.clear();
        // переносим preLog в очередь
        for (int i = 0; i < preLog->Count; ++i)
			pendingLog.push_back(preLog->Strings[i]);
    }
	delete preLog;

	tmSaveProgress->Enabled = true;

    const bool makeDocx = chkDocx->Checked;  // фиксируем состояние чекбокса сейчас
	const std::string out_dir_copy = out_dir_async;

    auto ctx = std::make_shared<SaveTaskContext>();

	ctx->text_map = std::move(text_map);
	ctx->pril_vh  = std::move(pril_map_vh);
	ctx->pril_ish = std::move(pril_map_ish);

	ctx->out_dir  = out_dir_copy;
	ctx->makeDocx = chkDocx->Checked;



	// ===== фон =====
	System::Threading::TTask::Run([this, ctx, guard = std::move(compGuard)]()
	{
		int count_suc_files = 0;

		try
		{
			for (const auto& f : ctx->text_map) {
				count_suc_files += SaveFileVh(ctx->out_dir, f, ctx->makeDocx);
				Sleep(15);
				saveFilesDone.fetch_add(1, std::memory_order_relaxed);
			}

			for (const auto& f : ctx->pril_vh) {
				count_suc_files += SaveFileVh2(ctx->out_dir, f);
				Sleep(15);
				saveFilesDone.fetch_add(1, std::memory_order_relaxed);
			}

			for (const auto& f : ctx->pril_ish) {
				count_suc_files += SaveFileIsh(ctx->out_dir, f);
				Sleep(15);
				saveFilesDone.fetch_add(1, std::memory_order_relaxed);
			}
		}
		catch (...)
		{
			pushLog("Ошибка в процессе сохранения");
		}



		pendingLog.push_back(
				String(count_suc_files) + " из " +
				String((int)saveFilesTotal.load()) +
				" файлов успешно сохранены!"
			);

		Sleep(tmSaveProgress->Interval+10);
		tmSaveProgress->Enabled = false;
		tmSaveProgressTimer(nullptr);

		ggSaveProgress->Progress = ggSaveProgress->MaxValue;
		saveRunning.store(false, std::memory_order_release);


	});


}
//---------------------------------------------------------------------------

void __fastcall TfMain::chkSQLClick(TObject *Sender)
{
	if(chkSQL->Checked)
	{
		chkOEM->Checked = false;
		chkOEM->Enabled = false;
		chkExc->Enabled = true;
		rgTypeT->Enabled = true;
		cmbField->Enabled = true;
		cmbMaxReq->Enabled = true;
	}
	else
	{
	   chkOEM->Checked = true;
	   chkOEM->Enabled = true;
	   chkExc->Enabled = false;
	   rgTypeT->Enabled = false;
	   cmbField->Enabled = false;
	   cmbMaxReq->Enabled = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TfMain::btnUpdateClick(TObject *Sender)
{

	std::string err;

	// 1) отправка
	if (!client_.socketWrite(std::string("START update!"), COMMAND::START_UPDATE_BASE, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return;
	}

	// 2) чтение ответа
	std::string str;
	if (!client_.socketRead(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
		return;
	}

	// дальше используем str

}
//---------------------------------------------------------------------------


void __fastcall TfMain::Button3Click(TObject *Sender)
{
		pnShablons->Visible = !pnShablons->Visible;
}
//---------------------------------------------------------------------------


void __fastcall TfMain::Button1Click(TObject *Sender)
{

	auto tm_group = lstShGroups->ItemIndex != -1 ? lstShGroups->Items->Strings[lstShGroups->ItemIndex] : "";
	tm_group = InputBox("Введите группу шаблона", "Группа:",  tm_group);

	if(tm_group == "")
		return;

	auto tm_name = lstTemplates->ItemIndex != -1 ? lstTemplates->Items->Strings[lstTemplates->ItemIndex] : "";
	tm_name =  InputBox("Введите название шаблона", "Шаблон:",  tm_name);

	if(tm_name == "")
		return;

	myRequest::saveTemplate(tm_group, tm_name);

	JSON::saveToJsonFile(myRequest::getTemplates(), "Templates.json");

	if(lstShGroups->Items->IndexOf(tm_group) == -1)
		lstShGroups->Items->Add(tm_group);
	else if(lstShGroups->ItemIndex != -1 && tm_group == lstShGroups->Items->Strings[lstShGroups->ItemIndex] && lstTemplates->Items->IndexOf(tm_name) == -1)
		lstTemplates->Items->Add(tm_name);

}
//---------------------------------------------------------------------------

void __fastcall TfMain::lstShGroupsClick(TObject *Sender)
{
	if(lstShGroups->ItemIndex == -1)
		return;


	lstTemplates->Clear();

	auto group = lstShGroups->Items->Strings[lstShGroups->ItemIndex];

	for(const auto& t: myRequest::groupTempaltes(group))
		lstTemplates->Items->Add(t);
}
//---------------------------------------------------------------------------

void __fastcall TfMain::lstTemplatesClick(TObject *Sender)
{
	if(lstTemplates->ItemIndex == -1)
		return;

	auto group = lstShGroups->Items->Strings[lstShGroups->ItemIndex];
	auto name = lstTemplates->Items->Strings[lstTemplates->ItemIndex];

	myRequest::setTemplate(group, name);

}
//---------------------------------------------------------------------------
void __fastcall TfMain::N2Click(TObject *Sender)
{
	 auto name  = lstTemplates->Items->Strings[lstTemplates->ItemIndex];
	 String title = L"Удаление шаблона";
	 String sub_title = L"Удалить шаблон '" + name + L"'?";
	 const int result = MessageBoxW(NULL, sub_title.c_str(), title.c_str() ,  MB_OKCANCEL);

	if(result == idCancel)
		return;

	if(lstTemplates->ItemIndex == -1 || lstShGroups->ItemIndex == -1)
		return;


	auto group = lstShGroups->Items->Strings[lstShGroups->ItemIndex];

	myRequest::removeTemplate(group, name);
	JSON::saveToJsonFile(myRequest::getTemplates(), "Templates.json");

	lstTemplates->Items->Delete(lstTemplates->ItemIndex);

}
//---------------------------------------------------------------------------

void __fastcall TfMain::N3Click(TObject *Sender)
{
	if(lstShGroups->ItemIndex == -1)
		return;

	String tm_group =  lstShGroups->Items->Strings[lstShGroups->ItemIndex];
	String sub_title = L"Удаление группы шаблонов";
	String title = L"Удалить гуруппу '" + tm_group  +  L"' со всеми шаблонами?";

	const int result = MessageBoxW(NULL, title.c_str(), sub_title.c_str(),  MB_OKCANCEL);

	if(result == idCancel)
		return;

	myRequest::removeGroup(tm_group);

	JSON::saveToJsonFile(myRequest::getTemplates(), "Templates.json");

	lstShGroups->Items->Delete(lstShGroups->ItemIndex);
	lstTemplates->Items->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::FormMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y)
{
    offsetX = X;
	offsetY = Y;
}
//---------------------------------------------------------------------------

void __fastcall TfMain::FormMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)

{
    if (Shift.Contains(ssLeft))
    {
        Left += X - offsetX;
        Top += Y - offsetY;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfMain::FormMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  int X, int Y)
{
    offsetX = 0;
    offsetY = 0;
}
//---------------------------------------------------------------------------

void __fastcall TfMain::lstTemplatesDblClick(TObject *Sender)
{
	lstTemplatesClick(Sender);
	btnSearchClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfMain::edtSaveDirDblClick(TObject *Sender)
{
	edtSaveDir->Text = "";
}
//---------------------------------------------------------------------------


void TfMain::makeErrorWay(bool error)
{


}

void __fastcall TfMain::N5Click(TObject *Sender)
{
	makeErrorWay(false);
}
//---------------------------------------------------------------------------

void __fastcall TfMain::dbgEhKeyPress(TObject *Sender, System::WideChar &Key)
{

   	tmLoadText->Enabled = false;


	if(Key == VK_SPACE)
	{
	if(dbgEh->SelectedRows->CurrentRowSelected == false)
		dbgEh->SelectedRows->CurrentRowSelected = true;
	else
	   dbgEh->SelectedRows->CurrentRowSelected = false;
    }
}

//---------------------------------------------------------------------------

void __fastcall TfMain::srcSearchDataChange(TObject *Sender, TField *Field)
{
	if(save_pocessing)
		return;

	tmLoadText->Enabled = false;
	tmLoadText->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TfMain::actDoSearchExecute(TObject *Sender)
{
	chkSQL->Checked = false;
	if(cmbReq->CanFocus())
		cmbReq->SetFocus();
	cmbReq->SelectAll();
}
//---------------------------------------------------------------------------



void __fastcall TfMain::tmLoadTextTimer(TObject *Sender)
{



	if (!auth_ || !dbgEh || !dbgEh->DataSource || !dbgEh->DataSource->DataSet ||
		!dbgEh->DataSource->DataSet->Active  || dbgEh->Fields[0]->AsString.IsEmpty())
		return;

	 if(saveRunning.load(std::memory_order_acquire))
	{
		return;
	}


	std::lock_guard<std::mutex> lock(clickMutex);
	String new_num = dbgEh->Fields[0]->AsString;

	if (last_loaded_num_ == new_num)
		return;

	last_loaded_num_ = new_num;
	tmLoadText->Enabled = false;

	String dir;
	try {
		dir = dbgEh->DataSource->DataSet->FieldValues["dir"];
	} catch (...) {
		return;
	}
	if (dir.IsEmpty())
		return;

	// Подготовка переменных
	const bool f12lock = (!checkUser() && useF12Control());
	if (f12lock)
		return;

std::string dir_str = AnsiString(dir).c_str();




System::Threading::TTask::Run([=]() {
    std::lock_guard<std::mutex> lock(clickMutex);
	std::string str;
	std::string err;

	// 1) отправка
	if (!client_.socketWrite(dir_str, COMMAND::FILETEXT, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return;
	}

	// 2) чтение

	if (!client_.socketRead(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
		return;
	}

	// дальше работаем со str


	TStringList* reg = new TStringList();
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg);
	String text = reg->Text;
	delete reg;

    // Переносим всё в GUI-поток
    TThread::Queue(nullptr, [=]() {
        if (last_loaded_num_ != new_num)
            return;

        // --- Freeze: отключить перерисовку ---
        SendMessage(rchEdt->Handle, WM_SETREDRAW, FALSE, 0);


        // --- Bulk‑обновление ---
        rchEdt->Lines->BeginUpdate();
        rchEdt->Lines->Text = text;

        std::unique_ptr<TStringList> sl(new TStringList());
        sl->DelimitedText = chkUpper->Checked ? cmbReq->Text : cmbReq->Text.UpperCase();
        for (const auto& s : sl.get())
            setColor(s);

        // Каретка в начало
        CHARRANGE cr{0, 0};
        SendMessage(rchEdt->Handle, EM_EXSETSEL, 0, (LPARAM)&cr);

        // Жёстко ставим скролл в самый верх
        POINT top{0, 0};
        SendMessage(rchEdt->Handle, EM_SETSCROLLPOS, 0, (LPARAM)&top);

		rchEdt->Lines->EndUpdate();
		rchEdt->Enabled = true;

		if(dbgEh->CanFocus())
			dbgEh->SetFocus();

        tmLoadText->Enabled = true;
    });
});


}




//---------------------------------------------------------------------------


void __fastcall TfMain::actSetVhExecute(TObject *Sender)
{
	chkSQL->Checked = true;
	rgTypeT->ItemIndex = 0;
	cmbField->ItemIndex = 0;
	if(cmbReq->CanFocus())
		cmbReq->SetFocus();
	cmbReq->SelectAll();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::actSetIshExecute(TObject *Sender)
{
    chkSQL->Checked = true;
	rgTypeT->ItemIndex = 1;
	cmbField->ItemIndex = 0;

	if(cmbReq->CanFocus())
		cmbReq->SetFocus();
	cmbReq->SelectAll();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::actSetFieldExecute(TObject *Sender)
{
	if(!chkSQL->Checked)
		return;
	if(cmbField->CanFocus())
		cmbField->SetFocus();
	cmbField->DroppedDown = true;
}
//---------------------------------------------------------------------------


void __fastcall TfMain::actSaveExecute(TObject *Sender)
{
	btnSaveClick(nullptr);
}
//---------------------------------------------------------------------------

void __fastcall TfMain::actSetPrilExecute(TObject *Sender)
{
	if(chkPril->Checked)
	   chkPril->Checked = false;
	else
      chkPril->Checked = true;
}

//---------------------------------------------------------------------------

void __fastcall TfMain::actSetTextExecute(TObject *Sender)
{
	if(chkText->Checked)
	   chkText->Checked = false;
	else
	  chkText->Checked = true;
}
//---------------------------------------------------------------------------


void __fastcall TfMain::N6Click(TObject *Sender)
{
	rchEdt->Print("Печать");
}
//---------------------------------------------------------------------------


void __fastcall TfMain::actShowRowPanelExecute(TObject *Sender)
{
	// нет строки – нечего показывать
	if (!qryTelegi->Active || qryTelegi->RecordCount == 0)
        return;

	dbgEh->RowDetailPanel->Visible = !dbgEh->RowDetailPanel->Visible;

	if(dbgEh->CanFocus())
		dbgEh->SetFocus();

	bool b;
  //	dbgEh->OnRowDetailPanelShow(dbgEh, b);
	dbgMenu->RowDetailPanel->Visible = !dbgMenu->RowDetailPanel->Visible;
  //	dbgMenu->OnRowDetailPanelShow(dbgMenu, b);
}
//---------------------------------------------------------------------------


std::string getExecutablePath() {
	return std::string(AnsiString{ExtractFilePath(Application->ExeName)}.c_str());
}



#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <Windows.h>

void __fastcall TfMain::onSuccessfulAuthorization(const std::string& user_name, const std::string& filePath) {
	componentsEnable(true);
	lblUser->Caption = "Авторизация : " + AnsiString(user_name.c_str());
	user_ = user_name;
	auth_ = true;
	lblUser->Font->Color = clGreen;
	token_dir_ = filePath;
	//tmLoginIn->Enabled = false;
	tmLoginOut->Enabled = true;
}



// Отключаем критические окна ОС для текущего потока (RAII)
struct FailCriticalErrorsGuard {
    DWORD oldMode{0};
    FailCriticalErrorsGuard() {
        // Для Win7+ предпочтительнее SetThreadErrorMode, но SetErrorMode тоже сработает
        typedef BOOL (WINAPI *PFN_SetThreadErrorMode)(DWORD, PDWORD);
        static auto pSetThreadErrorMode =
            reinterpret_cast<PFN_SetThreadErrorMode>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadErrorMode"));
        DWORD newMode = SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX;
        if (pSetThreadErrorMode) pSetThreadErrorMode(newMode, &oldMode);
        else oldMode = SetErrorMode(newMode);
    }
    ~FailCriticalErrorsGuard() {
        typedef BOOL (WINAPI *PFN_SetThreadErrorMode)(DWORD, PDWORD);
        static auto pSetThreadErrorMode =
            reinterpret_cast<PFN_SetThreadErrorMode>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadErrorMode"));
		if (pSetThreadErrorMode) pSetThreadErrorMode(oldMode, nullptr);
        else SetErrorMode(oldMode);
    }
};

// Быстрый exists для Windows (без std::filesystem)
inline bool SafeExistsWin(const std::wstring& path) {
    FailCriticalErrorsGuard guard;
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        // Для пустого диска вернётся ERROR_NOT_READY / ERROR_PATH_NOT_FOUND и т.п. — это ОК
        return false;
    }
    return true;
}

static std::vector<std::wstring> enumerateRemovableTokenPaths() {
    std::vector<std::wstring> out;
    DWORD mask = GetLogicalDrives();
    if (!mask) return out;

    for (int i = 0; i < 26; ++i) {
		if ((mask & (1u << i)) == 0) continue;
        wchar_t root[] = { wchar_t(L'A' + i), L':', L'\\', L'\0' };
		UINT dtype = GetDriveTypeW(root);
        if (dtype != DRIVE_REMOVABLE) continue;

		std::wstring tokenPath = std::wstring(root) + L"token-HS.bin";
        if (SafeExistsWin(tokenPath))
			out.push_back(tokenPath);
    }
	return out;
}

std::wstring toW(const std::string& s) {
	int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(n - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
	return w;
}



void __fastcall TfMain::tmLoginInTimer(TObject *Sender)
{

	BusyGuardAtomic guard(gLogInBusy);
    if (!guard) return; // уже идёт задача

    // Переносим владение гардом в shared_ptr, чтобы RAII дожил до конца задачи
    auto guard_holder = std::make_shared<BusyGuardAtomic>(std::move(guard));

    System::Threading::TTask::Run([this, guard_holder]() {
        auto rootOf = [](const std::string& p)->std::string { return p.substr(0,2); };
        static std::string admin_file = getExecutablePath() + "\\admin984.bin";

        // 1) быстрый admin-токен
        if (!auth_ && user_ != "admin" && SafeExistsWin(toW(admin_file))) {
            TThread::Queue(nullptr, [this]() {
                onSuccessfulAuthorization("admin", "");
                tryConnect();
                registryOnServer("admin");
                gCurrentPath = getExecutablePath() + "\\admin984.bin";
            });
            return; // guard_holder разрушится здесь -> флаг освободится
        }

        auto w2u8 = [](const std::wstring& ws){
            int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string s(n ? n - 1 : 0, '\0');
            if (n > 1) WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], n, nullptr, nullptr);
            return s;
        };

        auto isValid = [&](const std::string& path)->bool {
            try {
                FlashPodpis fp = loadFromBinaryFile(path);
                std::string serial = GetVolumeSerialNumber(rootOf(path));
                if (serial.empty() || fp.serial_number != serial) return false;
                RSA r; r.init(); fp.calcHash();
                auto decrypt_hash = r.RsaPubDecrypt(fp.podpis);
                return fp.checkPodp(decrypt_hash);
            } catch (...) { return false; }
        };

        std::unordered_set<std::string> now;
        for (const auto& wpath : enumerateRemovableTokenPaths()) {
            std::string p = w2u8(wpath);
            if (isValid(p)) now.insert(p);
        }

        std::string exeDir = std::filesystem::path(getExecutablePath()).parent_path().string();
        std::wstring wLocal = toW(exeDir + "\\token-HS.bin");
        if (SafeExistsWin(wLocal)) {
            std::string local = exeDir + "\\token-HS.bin";
            if (isValid(local)) now.insert(local);
        }

        for (auto it = gTokens.begin(); it != gTokens.end(); )
            it = now.count(it->first) ? std::next(it) : gTokens.erase(it);
        for (const auto& p : now)
            if (!gTokens.count(p)) gTokens[p] = ++gTick;

        if (gTokens.empty()) { gCurrentPath.clear(); return; }

        auto best = std::max_element(gTokens.begin(), gTokens.end(),
                                     [](auto&a, auto&b){ return a.second < b.second; });
        const std::string& desired = best->first;
        if (desired == gCurrentPath) return;

        try {
            FlashPodpis fp = loadFromBinaryFile(desired);
            TThread::Queue(nullptr, [this, fp, desired]() {
                onSuccessfulAuthorization(fp.fio, desired);
                gCurrentPath = desired;
                disConnect();
                if (tryConnect()) registryOnServer(fp.fio);
                else gTokens.erase(desired);
            });
        } catch (...) {
            gTokens.erase(desired);
        }
        // guard_holder выходит из области видимости -> освобождает busy-флаг
    });
}





//---------------------------------------------------------------------------
void __fastcall TfMain::updateFileLoadUI()
{
    if (!uiOk)
		memLogLoad->Lines->Add(uiFilePath + " - не удалось загрузить :(");
	else
		memLogLoad->Lines->Add(uiFilePath + " - успешно загружен :)");
	ggSaveProgress->AddProgress(1);
}

//---------------------------------------------------------------------------

void __fastcall TfMain::btnAddFilesLoadClick(TObject *Sender)
{
	std::set<String> set_files;


	for(const auto& f:lstFiles->Items)
		set_files.insert(f);

	for(const auto& f:SelectFiles())
	{
		if(set_files.find(f) == set_files.end())
			lstFiles->Items->Add(f);
	}


	ggSaveProgress->Progress = 0;
}
//---------------------------------------------------------------------------

void __fastcall TfMain::popItemClearFilesOnLoadClick(TObject *Sender)
{
	lstFiles->Items->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::delFileLoadClick(TObject *Sender)
{
	int ind = lstFiles->ItemIndex;

	if(ind == -1)
		return;

   lstFiles->Items->Delete(ind);
}
//---------------------------------------------------------------------------

void __fastcall TfMain::tbLoadShow(TObject *Sender)
{
	ggSaveProgress->Progress = 0;
	lstFiles->Items->Clear();
	memLogLoad->Lines->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::btnSaveOpisBaseClick(TObject *Sender)
{
    if (!checkUser()) {
        ShowMessage("Данная операция доступна только для авторизованных пользователей!");
        return;
    }

    // Копии нужных переменных для лямбды
    std::string dir = AnsiString(ExtractFileDir(String{token_dir_.c_str()})).c_str();
    std::string god = AnsiString{cmbGod->Text}.c_str();
    std::string full_file_name = dir + "\\" + god + ".db";

    // Заблокируем интерфейс
    componentsEnable(false);

    System::Threading::TTask::Run([=]() {
        // --- Фоновая часть ---
        try {
			client_.socketWrite(std::vector<uint8_t>{0}, COMMAND::GET_OPIS_BASE);
            std::vector<BYTE> str;
			if (!client_.socketReadBin(str)) {
                TThread::Queue(nullptr, [this]() {
                    ShowMessage("Ошибка получения базы!");
					componentsEnable(true);
                });
                return;
			}

            if (std::filesystem::exists(full_file_name))
                std::filesystem::remove(full_file_name);

            FILE* file = fopen((full_file_name).c_str(), "wb");
            if (file) {
                fwrite(&str[0], sizeof(BYTE), str.size(), file);
                fclose(file);

                TThread::Queue(nullptr, [this, full_file_name]() {
                    ShowMessage("База успешно сохранена - " + String{full_file_name.c_str()});
                    componentsEnable(true);
                });
            } else {
                TThread::Queue(nullptr, [this]() {
                    ShowMessage("Не удалось открыть файл для записи!");
                    componentsEnable(true);
                });
            }
        } catch (...) {
            TThread::Queue(nullptr, [this]() {
                ShowMessage("Ошибка при сохранении базы!");
                componentsEnable(true);
            });
        }
    });
}

//---------------------------------------------------------------------------

void __fastcall TfMain::btnLoadRaznClick(TObject *Sender)
{
	memLogLoad->Lines->Clear();

	for(const auto& filePath:SelectFiles())
		{

			std::wstring baseFileNameWStr = ExtractFileName(String(filePath.c_str())).c_str();

			auto file = loadFileToVector2(AnsiString(filePath).c_str(), baseFileNameWStr);

			std::string err;

			// 1) отправляем
			if (!client_.socketWrite(file, COMMAND::LOAD_RAZN, &err)) {
				memLogLoad->Lines->Add(filePath + " - ошибка отправки: " + AnsiString(err.c_str()));
				continue;
			}

			// 2) читаем бинарный ответ
			std::vector<uint8_t> str;
			if (!client_.socketReadBin(str, /*progress*/nullptr, &err)) {
				memLogLoad->Lines->Add(filePath + " - не удалось загрузить :( " + AnsiString(err.c_str()));
				continue;
			}

			// если всё ок — str содержит данные


			memLogLoad->Lines->Add(filePath + " - успешно загружен :)");

		}


}
//---------------------------------------------------------------------------
#include <stdexcept>
void normalizeByte(std::vector<uint8_t>& bytes, std::size_t offset, uint8_t value) {
	// Проверяем, что длина массива байтов позволяет обращаться к смещению
	if (bytes.size() > offset) {
		bytes[offset] = value;
	} else {
		throw std::runtime_error("Array size too small to normalize byte.");
	}
}

void writeBytesToFile(const std::vector<uint8_t>& bytes, const std::filesystem::path& filePath) {
	std::ofstream outFile(filePath, std::ios::binary);
	if (!outFile) {
		throw std::runtime_error("Failed to open file for writing: " + filePath.string());
	}
	outFile.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	outFile.close();
}
//---------------------------------------------------------------------------------------------
void __fastcall TfMain::btnAttachSaveClick(TObject *Sender)
{
	static auto createInnerDirectory = [](const std::filesystem::path& baseDir) {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::wstringstream dateStream;
		dateStream << std::put_time(std::localtime(&in_time_t), L"%d.%m.%y");
		std::wstring innerDirName = L"Приложения к шт. " + dateStream.str();
		return baseDir / innerDirName;
	};


	ggSaveProgress->Progress = 0;

	if (!checkUser()) {
		ShowMessage("Данная операция доступна только для авторизованных пользователей!");
		return;
	}

	std::string err;

	auto data_request = std::vector<uint8_t>(user_.begin(), user_.end());
	if (!client_.socketWrite(data_request, COMMAND::GET_ATTACHMENTS, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return;
	}

	std::vector<uint8_t> str;
	if (!client_.socketReadBin(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
		return;
	}

// дальше используем str


	int size = str.size();
	if (size < 33) {
		std::string s{str.begin(), str.end()};
		ShowMessage(("Ошибка данных: " + s).c_str());
		return;
	}

	std::wstring savePath = std::wstring(token_dir_.begin(), token_dir_.end());
	std::unique_ptr<TFileOpenDialog> dirDialog(new TFileOpenDialog(this));
	dirDialog->Options << fdoPickFolders;
	dirDialog->DefaultFolder = String{token_dir_.c_str()};
	dirDialog->Title = "Выбор дирректории сохранения приложений";
	//dirDialog->

	if (dirDialog->Execute()) {
		savePath = dirDialog->FileName.c_str();
	}
	if (savePath.empty()) {
		ShowMessage("Папка для сохранения не выбрана.");
		return;
	}

	auto progressForm = std::make_unique<TForm>(this);
	progressForm->BorderStyle = bsDialog;
	progressForm->Caption = L"Сохранение";
	progressForm->Width = 600;
	progressForm->Height = 200;
	progressForm->Position = poScreenCenter;

	auto label = std::make_unique<TLabel>(progressForm.get());
	label->Parent = progressForm.get();
	label->Caption = L"Идет сохранение, пожалуйста, подождите...";
	label->Align = alClient;
	label->Alignment = taCenter;
	label->Layout = tlCenter;
	label->Font->Style = TFontStyles() << fsBold;

	progressForm->Show();
	Application->ProcessMessages();

	auto save_byte = str[25];
	normalizeByte(str, 25, 0x11);

retry_save:

	try {
		try {
			message_ = Message::deserializeFromBytes(str);
		} catch (...) {
			str[25] = save_byte;
			message_ = Message::deserializeFromBytes(str);
		}

		auto save_path = createInnerDirectory(savePath);
		message_.saveToDirectory(save_path);

		ShowMessage("Сохранение завершено успешно!\n - " + AnsiString{save_path.string().c_str()});
		message_.attachments.clear();
	} catch (...) {
		int retryResult = Application->MessageBox(
			L"Произошла ошибка во время выполнения сохранения. Повторить попытку?",
			L"Ошибка сохранения",
			MB_YESNO | MB_ICONERROR
		);

		if (retryResult == IDYES) {
			goto retry_save;  // Переход к метке для повторной попытки
		} else {
			message_.attachments.clear();  // Очистка сообщения
		}
	}

}


//---------------------------------------------------------------------------

void __fastcall TfMain::tbcSettingsChange(TObject *Sender)
{
     try {
        // Получаем имя выбранной вкладки
        String selectedTab = tbcSettings->Tabs->Strings[tbcSettings->TabIndex];
        // Формируем запрос для открытия таблицы
        qrySettings->Close();
        qrySettings->SQL->Text = "SELECT * FROM " + selectedTab;
        qrySettings->Open();
      //  ShowMessage("Таблица \"" + selectedTab + "\" успешно загружена.");
    } catch (const Exception &e) {
        ShowMessage("Ошибка при загрузке таблицы: " + e.Message);
    }
}
//---------------------------------------------------------------------------



//  Обработчик кнопки – один сырой файл → готовый DOCX
// -----------------------------------------------------------

void __fastcall TfMain::chkDocxClick(TObject *Sender)
{
	if(!chkDocx->Checked)
		return;

	chkText->Checked = false;
}
//---------------------------------------------------------------------------

void __fastcall TfMain::chkTextClick(TObject *Sender)
{
	if(!chkText->Checked)
		return;

	chkDocx->Checked = false;
}
//---------------------------------------------------------------------------


void __fastcall TfMain::FormShow(TObject *Sender)
{
   //	Button2Click(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfMain::btnDisconnectClick(TObject *Sender)
{
	disConnect();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------


void __fastcall TfMain::Button2Click(TObject *Sender)
{

	frxReport->ShowReport();



}
//---------------------------------------------------------------------------
void __fastcall TfMain::btnLoadOnServerFilesClick(TObject *Sender)
{
    componentsEnable(false);
    ggSaveProgress->Progress = 0;
    memLogLoad->Lines->Clear();

	filesToDeleteUI.clear();

    // Готовим список файлов отдельно, не используем '=' для лямбды!
    std::vector<String> fileList;
    for (int i = 0; i < lstFiles->Items->Count; ++i)
		fileList.push_back(lstFiles->Items->Strings[i]);

    ggSaveProgress->MaxValue = fileList.size();
	ggSaveProgress->Progress = 0;

    // Запускаем задачу БЕЗ [=], только [this] и fileList явно!
	System::Threading::TTask::Run([this, fileList]() {
        for (const auto &filePath : fileList)
		{
			std::wstring baseFileNameWStr = ExtractFileName(filePath).w_str();  // начальное имя
			baseFileNameWStr = conv::to_wstring(user_) + L"\\" + baseFileNameWStr;  // полное имя


			std::string err;

			// загружаем файл
			auto file = loadFileToVector2(AnsiString(filePath).c_str(), baseFileNameWStr);

			// отправляем на сервер
			if (!client_.socketWrite(file, COMMAND::LOAD_TLG_TO_SEND, &err)) {
				ShowMessage("Ошибка отправки файла: " + AnsiString(err.c_str()));
				return;
			}

			std::vector<uint8_t> str;
			bool ok = client_.socketReadBin(str, /*progress*/nullptr, &err);

			if (!ok) {
				ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
				return;
			}


            // Сохраняем данные для UI (только через поля класса!)
            this->uiFilePath = filePath;
            this->uiOk = ok;

            if (ok)
                this->filesToDeleteUI.push_back(filePath);

            // Обновление UI (очень простая лямбда, ранее 100% работала!)
			TThread::Queue(nullptr, [this]() { updateFileLoadUI(); });
        }

        // Очистка списка и финальное обновление UI
        TThread::Queue(nullptr, [this]() { deleteFilesFromUI(); });
    });
}




void __fastcall TfMain::deleteFilesFromUI()
{
    for (auto it = filesToDeleteUI.rbegin(); it != filesToDeleteUI.rend(); ++it) {
        int delInd = lstFiles->Items->IndexOf(*it);
        if (delInd != -1)
            lstFiles->Items->Delete(delInd);
    }
	ggSaveProgress->AddProgress(1);
	componentsEnable(true);
}




//---------------------------------------------------------------------------

void __fastcall TfMain::rchEdtClick(TObject *Sender)
{
    pnShablons->Visible = false;
}
//---------------------------------------------------------------------------




void __fastcall TfMain::dbgEhRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow)
{
	if (!auth_ || !dbgEh || !dbgEh->DataSource || !dbgEh->DataSource->DataSet ||
        !dbgEh->DataSource->DataSet->Active || dbgEh->Fields[0]->AsString.IsEmpty())
        return;


    // очистка и заполнение меню
    mtMenu->EmptyTable();
    mtMenu->AppendRecord(ARRAYOFCONST(("Путь:")));
    mtMenu->AppendRecord(ARRAYOFCONST(("Приложения:")));

    String dirType = dbgEh->Fields[7]->AsString;
    if (dirType != L"Вх.") {
		mtMenu->AppendRecord(ARRAYOFCONST(("Подтверждения:")));
    }

    int rowCount   = mtMenu->RecordCount;
	int rowHeight  = dbgMenu->DefaultRowHeight;
	if (rowHeight <= 0)
		rowHeight = -dbgMenu->Font->Height + 6;  // fallback по шрифту

	int titleHeight = dbgMenu->TitleHeight;

	// +1 строка сверху, чтобы последняя помещалась, и небольшой запас
	int newHeight = 15 + titleHeight + (rowCount + 1) * rowHeight;

	dbgEh->RowDetailPanel->Height   = newHeight;
	dbgMenu->RowDetailPanel->Height = newHeight;
    dbgMenu->DataSource->DataSet->First();
}


//---------------------------------------------------------------------------


void __fastcall TfMain::tmLoginOutTimer(TObject *Sender)
{
	// не запускаем второй выход, пока предыдущий не завершён
	BusyGuardAtomic guard(gLogoutBusy);
	if (!guard) return;

	static std::string admin_file = getExecutablePath() + "\\admin984.bin";
	if (user_ == "admin")
		return;

	// если токен (каталог/файл) «есть» — выходим (ничего не делаем)
	// заменили std::filesystem::exists(token_dir_) на безопасную проверку
	if (SafeExistsWin(toW(token_dir_)))
		return;

	// Очищаем данные в фоне (если вдруг очистка attachments может быть долгой)
	System::Threading::TTask::Run([this]() {
		message_.attachments.clear();
		user_.clear();

		// Возврат в главный поток для UI и остальных операций
		TThread::Queue(nullptr, [this]() {
			clear();
			//tmLoginOut->Enabled = false;
			tmLoginIn->Enabled = true;
			pnShablons->Visible = false;
			lblUser->Caption = "Нет авторизации!";
			lblUser->Font->Color = clMaroon;
			auth_ = false;
			disConnect();
			componentsEnable(false);
		});
	});
}


bool TfMain::tryConnect()
{
    const AnsiString port = mapPorts.at(cmbGod->Text);
    const AnsiString host = Edit2->Text;

    AnsiString err;
    if (!client_.tryConnect(host, port, err)) {
        ShowMessage("Ошибка подключения: " + err);
        lbStatus->Caption = "Статус: нет подключения";
        componentsEnable(false);
        return false;
    }

    lbStatus->Caption = "Статус: подключено к серверу " + host + ":" + port + " " + cmbGod->Text + " год";
    componentsEnable(true);
    return true;
}


//---------------------------------------------------------------------------




void __fastcall TfMain::dbgMenuRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow)

{

		myFrWay->Visible = false;
		myFrPdtv->Visible = false;
		myFrAttachments->Visible = false;
		const String p_menu = dbgMenu->Fields[0]->AsString;

		if (p_menu == "Приложения:") {
			myFrAttachments->Visible = true;
			myFrAttachments->OnShow();
		}
		else if (p_menu == "Подтверждения:") {
			myFrPdtv->Visible = true;
			myFrPdtv->OnShow();
		}
		else if (p_menu == "Путь:") {
			myFrWay->Visible = true;
			myFrWay->OnShow();
		}
		else if (p_menu == "Выход") {
			Close();
		}

}
//---------------------------------------------------------------------------

void __fastcall TfMain::dbgMenuRowDetailPanelHide(TCustomDBGridEh *Sender, bool &CanHide)

{
		dbgEh->RowDetailPanel->Height -= dbgMenu->RowDetailPanel->Height;
}
//---------------------------------------------------------------------------


void __fastcall TfMain::excel1Click(TObject *Sender)
{
    if (!frxReport->PrepareReport())
		return;

        auto xls  = frxXLSExport1;

	TSaveDialog *dlg = new TSaveDialog(this);
    try
    {
        dlg->Filter = "Excel (*.xlsx)|*.xlsx";
        dlg->DefaultExt = "xlsx";

        if (dlg->Execute())
		{

            try
            {
                xls->FileName = dlg->FileName;
                xls->ShowDialog = false;
                xls->OpenAfterExport = true;
                xls->OverwritePrompt = true;

                frxReport->Export(xls);
            }
            __finally
            {
                delete xls;
            }
        }
    }
    __finally
    {
        delete dlg;
    }
}


void __fastcall TfMain::tmSaveProgressTimer(TObject *Sender)
{
	// сливаем логи пачкой
	{
		std::vector<String> batch;
        {
            std::lock_guard<std::mutex> lk(log_mtx);
			batch.swap(pendingLog);
        }
		for (auto &s : batch)
            rchEdt->Lines->Add(s);
    }

    if (!saveRunning.load(std::memory_order_acquire))
    {
        tmSaveProgress->Enabled = false;
        return;
    }

    const auto done  = (int)saveFilesDone.load(std::memory_order_relaxed);
    const auto total = (int)saveFilesTotal.load(std::memory_order_relaxed);

    // если у тебя ProgressBar есть
	if (ggSaveProgress)
	{
		ggSaveProgress->MaxValue = std::max(1, total);
		ggSaveProgress->Progress = std::min(done, total);
	}

   // if (lblStatus)
	//    lblStatus->Caption = String(done) + " / " + String(total);
}


