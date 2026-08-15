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
#include <atomic>
#include <thread>
#include <cstdio>
#include <algorithm>
#include <System.JSON.hpp>
#include "stdint.h"
#include "formMain.h"
#include "AuthPaths.h"
#include "UserFileReplace.h"
#include "ClientFilePublish.h"
#include "Database\ResDbMigration.h"
#include "Database\SqliteDb.h"
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
#pragma comment(lib, "version.lib")
namespace {
String GetSearchClientProductVersion()
{
    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) == 0) {
        return String();
    }
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(modulePath, &handle);
    if (size == 0) {
        return String();
    }
    std::vector<unsigned char> data(size);
    if (!GetFileVersionInfoW(modulePath, 0, size, data.data())) {
        return String();
    }
    VS_FIXEDFILEINFO *fixedInfo = nullptr;
    UINT fixedInfoSize = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void **>(&fixedInfo), &fixedInfoSize) ||
        fixedInfo == nullptr || fixedInfoSize < sizeof(VS_FIXEDFILEINFO) ||
        fixedInfo->dwSignature != VS_FFI_SIGNATURE) {
        return String();
    }
    wchar_t *productVersion = nullptr;
    UINT productVersionSize = 0;
    if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040904E4\\ProductVersion",
            reinterpret_cast<void **>(&productVersion), &productVersionSize) ||
        productVersion == nullptr || productVersionSize == 0) {
        return String();
    }
    return String(productVersion);
}
}


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
#pragma link "frxDBSet"
#pragma link "frxPreview"
#pragma link "frxExportBaseDialog"
#pragma link "frxExportXLS"
#pragma link "DataDriverEh"
#pragma link "DBAxisGridsEh"
#pragma link "DBGridEh"
#pragma link "DBGridEhGrouping"
#pragma link "DBGridEhToolCtrls"
#pragma link "DynVarsEh"
#pragma link "EhLibVCL"
#pragma link "frxClass"
#pragma link "frxDBSet"
#pragma link "frxExportBaseDialog"
#pragma link "frxExportXLS"
#pragma link "GridsEh"
#pragma link "MemTableDataEh"
#pragma link "MemTableEh"
#pragma link "PrnDbgEh"
#pragma link "ToolCtrlsEh"
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

namespace {
struct InflightToken {
	std::atomic<int>* counter = nullptr;
	explicit InflightToken(std::atomic<int>* c) : counter(c)
	{
		if (counter)
			counter->fetch_add(1, std::memory_order_acq_rel);
	}
	~InflightToken()
	{
		if (counter)
			counter->fetch_sub(1, std::memory_order_acq_rel);
	}
	InflightToken(const InflightToken&) = delete;
	InflightToken& operator=(const InflightToken&) = delete;
};
} // namespace

bool TfMain::uiIsLive() const
{
	return uiLive_ && uiLive_->load(std::memory_order_acquire) && !shuttingDown_;
}

std::shared_ptr<void> TfMain::trackBackgroundOp()
{
	return std::shared_ptr<void>(std::make_shared<InflightToken>(&uiInflight_));
}







size_t getFileSize(const std::string& fs)
{
	struct stat stat_buf;
	int rc = stat(fs.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

//#include <my_json/ConverterJSON.h>
//#include <my_json/ConverterJSON.cpp>
#include <my_json/Telega.h>
using namespace std;
typedef std::map<std::string,std::map<std::string, Request>> mapTemplates;

namespace {
std::wstring TemplatesJsonPath()
{
	return auth_paths::fileBesideExecutable(L"Templates.json");
}

void LoadTemplatesFromDisk()
{
	const std::wstring path = TemplatesJsonPath();
	if (!auth_paths::isRegularFile(path))
		return;
	std::string bytes;
	if (!user_file_replace::ReadAll(path, bytes))
		return;
	myRequest::loadTemplates(JSON::get<mapTemplates>(bytes));
}

void SaveTemplatesToDisk()
{
	nh::json json = myRequest::getTemplates();
	const std::string text = json.dump(2);
	user_file_replace::WriteReplacing(TemplatesJsonPath(), text.data(), text.size());
}

const int MAIN_FORM_DESIGN_PPI = 192;
const int MAIN_CONTENT_MIN_WIDTH_AT_DESIGN_PPI = 1932;
const int MAIN_CONTENT_MIN_HEIGHT_AT_DESIGN_PPI = 1194;
const int RESPONSIVE_LAYOUT_INTERVAL_MS = 75;

void EqualizeSettingsColumnWidths(TDBGridEh* grid)
{
	if (!grid || grid->Columns->Count == 0)
		return;
	// AutoFitColWidths preserves the initial proportions of dynamic columns.
	// Those proportions differ for INTEGER and VARCHAR fields (notably in ports),
	// so give every settings column the same base width before fitting the grid.
	grid->AutoFitColWidths = false;
	for (int i = 0; i < grid->Columns->Count; ++i) {
		TColumnEh* column = grid->Columns->Items[i];
		column->AutoFitColWidth = true;
		column->Width = std::max(1,
			MulDiv(100, grid->CurrentPPI, MAIN_FORM_DESIGN_PPI));
	}
	grid->AutoFitColWidths = true;
	grid->Columns->FixupAutoFitColWidths(grid->ClientWidth);
}
const wchar_t* const MAIN_WINDOW_REGISTRY_KEY =
	L"Software\\GoodbyeYVO\\SearchClient\\MainWindow";
const DWORD MAIN_WINDOW_PLACEMENT_VERSION = 1;
bool ReadRegistryDword(HKEY key, const wchar_t* name, DWORD& value) noexcept
{
	DWORD type = 0;
	DWORD size = sizeof(value);
	return RegQueryValueExW(key, name, nullptr, &type,
		reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS
		&& type == REG_DWORD && size == sizeof(value);
}
bool ReadMainWindowPlacement(RECT& normalRect, bool& maximized) noexcept
{
	HKEY key = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, MAIN_WINDOW_REGISTRY_KEY, 0,
		KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
	{
		return false;
	}
	DWORD version = 0;
	DWORD left = 0;
	DWORD top = 0;
	DWORD right = 0;
	DWORD bottom = 0;
	DWORD maximizedValue = 0;
	const bool loaded =
		ReadRegistryDword(key, L"Version", version)
		&& version == MAIN_WINDOW_PLACEMENT_VERSION
		&& ReadRegistryDword(key, L"Left", left)
		&& ReadRegistryDword(key, L"Top", top)
		&& ReadRegistryDword(key, L"Right", right)
		&& ReadRegistryDword(key, L"Bottom", bottom)
		&& ReadRegistryDword(key, L"Maximized", maximizedValue);
	RegCloseKey(key);
	if (!loaded)
		return false;
	normalRect.left = static_cast<LONG>(left);
	normalRect.top = static_cast<LONG>(top);
	normalRect.right = static_cast<LONG>(right);
	normalRect.bottom = static_cast<LONG>(bottom);
	const LONGLONG width = static_cast<LONGLONG>(normalRect.right)
		- normalRect.left;
	const LONGLONG height = static_cast<LONGLONG>(normalRect.bottom)
		- normalRect.top;
	if (width < 320 || height < 240 || width > 100000 || height > 100000)
		return false;
	maximized = maximizedValue != 0;
	return true;
}
bool WriteRegistryDword(HKEY key, const wchar_t* name, DWORD value) noexcept
{
	return RegSetValueExW(key, name, 0, REG_DWORD,
		reinterpret_cast<const BYTE*>(&value), sizeof(value)) == ERROR_SUCCESS;
}
bool WriteMainWindowPlacement(const RECT& normalRect, bool maximized) noexcept
{
	HKEY key = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, MAIN_WINDOW_REGISTRY_KEY, 0,
		nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key,
		nullptr) != ERROR_SUCCESS)
	{
		return false;
	}
	// Version is the commit marker: an interrupted multi-value write is ignored
	// on the next launch instead of mixing old and new coordinates.
	bool written = WriteRegistryDword(key, L"Version", 0);
	written = WriteRegistryDword(
		key, L"Left", static_cast<DWORD>(normalRect.left)) && written;
	written = WriteRegistryDword(
		key, L"Top", static_cast<DWORD>(normalRect.top)) && written;
	written = WriteRegistryDword(
		key, L"Right", static_cast<DWORD>(normalRect.right)) && written;
	written = WriteRegistryDword(
		key, L"Bottom", static_cast<DWORD>(normalRect.bottom)) && written;
	written = WriteRegistryDword(
		key, L"Maximized", maximized ? 1U : 0U) && written;
	if (written)
		written = WriteRegistryDword(key, L"Version", MAIN_WINDOW_PLACEMENT_VERSION);
	RegCloseKey(key);
	return written;
}
void KeepNormalWindowReachable(HWND window, int currentPpi) noexcept
{
	RECT rect = {};
	if (!window || !GetWindowRect(window, &rect))
		return;
	HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = {};
	info.cbSize = sizeof(info);
	if (!monitor || !GetMonitorInfoW(monitor, &info))
		return;
	const LONG width = std::max<LONG>(1, rect.right - rect.left);
	const LONG height = std::max<LONG>(1, rect.bottom - rect.top);
	const int safePpi = currentPpi > 0 ? currentPpi : 96;
	const LONG visibleTitleWidth = std::min<LONG>(width,
		std::max(1, MulDiv(64, safePpi, 96)));
	const LONG visibleTitleHeight = std::min<LONG>(height,
		std::max(1, MulDiv(32, safePpi, 96)));
	const LONG minLeft = info.rcWork.left - width + visibleTitleWidth;
	const LONG maxLeft = info.rcWork.right - visibleTitleWidth;
	const LONG minTop = info.rcWork.top;
	const LONG maxTop = info.rcWork.bottom - visibleTitleHeight;
	const LONG left = std::max(minLeft, std::min(rect.left, maxLeft));
	const LONG top = std::max(minTop, std::min(rect.top, maxTop));
	if (left != rect.left || top != rect.top)
	{
		SetWindowPos(window, nullptr, left, top, width, height,
			SWP_NOACTIVATE | SWP_NOZORDER);
	}
}
void RealignControlTree(TWinControl* control)
{
	if (!control)
		return;
	control->Realign();
	const int children = control->ControlCount;
	for (int i = 0; i < children; ++i) {
		TWinControl* child = dynamic_cast<TWinControl*>(control->Controls[i]);
		if (child)
			RealignControlTree(child);
	}
}
}





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
	std::lock_guard<std::mutex> lk(log_mtx);
	pendingLog.push_back(s);
}


//---------------------------------------------------------------------------
__fastcall TfMain::TfMain(TComponent* Owner)
	: TForm(Owner)
{
	ClientDb::EnsureReady();
	const String exeDir = ExtractFileDir(Application->ExeName);
	String dbError;
	if (!ensureLocalDatabases(exeDir, &dbError)) {
		ShowMessage(dbError);
		throw Exception(dbError);
	}
	const LocalDbPaths dbPaths = makeLocalDbPaths(exeDir);
	ClientDb::Open(ConnectionSettings, dbPaths.settingsDb);
	ClientDb::Open(ConnectionRes, dbPaths.searchResDb);
	ClientDb::PrepareForLocalSort(qrySettings);
	ClientDb::PrepareForLocalSort(qryTelegi);
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
std::vector<uint8_t> TfMain::loadFileToVector2(const std::string& filePath, const std::wstring& file_name,
	bool showErrors) {
	if (filePath.empty()) {
		if (showErrors)
			ShowMessage("File path is empty. Cannot open file.");
		return {};
    }

	std::wstring wfilePath = String(filePath.c_str()).c_str(); // Преобразуем std::string в std::wstring
	TFileStream* fileStream = nullptr;

    try {
        fileStream = new TFileStream(wfilePath.c_str(), fmOpenRead | fmShareDenyWrite);
	}
	catch (...) {
		if (showErrors)
			ShowMessage("Failed to open the file.");
		delete fileStream;
        return {};
	}

	if (!fileStream) {
		if (showErrors)
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

void TfMain::applyRchEdtText(const String& telegramText, bool highlightQuery)
{
	String text = telegramText;
	if (!searchSaveReport_.IsEmpty()) {
		text = searchSaveReport_ +
			L"\r\n\r\n——— текст телеграммы ———\r\n\r\n" +
			telegramText;
	}

	rchEdt->Lines->BeginUpdate();
	try {
		rchEdt->Lines->Text = text;
		if (highlightQuery) {
			std::unique_ptr<TStringList> sl(new TStringList());
			sl->DelimitedText = chkUpper->Checked ? cmbReq->Text : cmbReq->Text.UpperCase();
			for (const auto& s : sl.get())
				setColor(s);
		}
	}
	__finally {
		rchEdt->Lines->EndUpdate();
	}
}

void TfMain::loadTextToEditFromServer()
{
	if (isDeletedRow(dbgEh->DataSource->DataSet)) {
		ShowMessage(deletedFileMessage());
		return;
	}

	AnsiString ss;

	try{
		ss = dbgEh->DataSource->DataSet->FieldValues["dir"];
	}
	catch(...){}

	if(ss == "")
		return;


	std::string err;
	std::string str;
	if (!serverManager_.requestText(
			currentSourceServerId(), std::string(AnsiString(ss).c_str()),
			COMMAND::FILETEXT, str, nullptr, &err)) {
		ShowMessage("Ошибка запроса: " + String(err.c_str()));
		return;
	}


	auto reg = new TStringList();
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg);
	applyRchEdtText(reg->Text, true);
	delete reg;
}
//---------------------------------------------------------------------------
std::map<String,String> TfMain::getMapFromQry(TFDQuery* qry, const String& table_name)
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
void TfMain::ensureServerCatalogSeeded(const String& fallbackHost)
{
	std::unique_ptr<TFDQuery> q(new TFDQuery(nullptr));
	q->Connection = ConnectionSettings;
	q->SQL->Text = L"SELECT COUNT(*) AS cnt FROM servers";
	q->Open();
	if (q->FieldByName(L"cnt")->AsInteger > 0)
		return;
	TFDCustomConnection* connection = q->Connection;
	connection->StartTransaction();
	try {
		q->Close();
		q->SQL->Text =
			L"INSERT INTO servers (server_id, display_name, host, enabled, sort_order) "
			L"VALUES ('default', 'Основной сервер', :host, 1, 0)";
		q->ParamByName(L"host")->AsString = fallbackHost;
		q->ExecSQL();
		q->SQL->Text =
			L"INSERT OR IGNORE INTO server_ports (server_id, god, port, enabled) "
			L"SELECT 'default', god, port, 1 FROM ports";
		q->ExecSQL();
		connection->Commit();
	}
	catch (...) {
		connection->Rollback();
		throw;
	}
}
void TfMain::reloadAvailableYears()
{
	cmbGod->Items->BeginUpdate();
	try {
		const String selected = cmbGod->Text;
		cmbGod->Items->Clear();
		mapPorts.clear();
		qrySettings->Close();
		qrySettings->SQL->Text =
			L"SELECT DISTINCT god FROM server_ports WHERE enabled <> 0 ORDER BY god DESC";
		qrySettings->Open();
		while (!qrySettings->Eof) {
			const String year = qrySettings->FieldByName(L"god")->AsString;
			cmbGod->Items->Add(year);
			qrySettings->Next();
		}
		const int oldIndex = cmbGod->Items->IndexOf(selected);
		cmbGod->ItemIndex = oldIndex >= 0 ? oldIndex : (cmbGod->Items->Count > 0 ? 0 : -1);
	}
	__finally {
		cmbGod->Items->EndUpdate();
	}
}
bool TfMain::configureServersForCurrentYear(String* errorMessage)
{
	if (cmbGod->Text.IsEmpty()) {
		if (errorMessage) *errorMessage = L"Не выбран год поиска";
		return false;
	}
	std::vector<ServerEndpoint> endpoints;
	qrySettings->Close();
	qrySettings->SQL->Text =
		L"SELECT s.server_id, s.display_name, s.host, p.port "
		L"FROM servers s JOIN server_ports p ON p.server_id = s.server_id "
		L"WHERE s.enabled <> 0 AND p.enabled <> 0 AND p.god = :god "
		L"ORDER BY s.sort_order, s.display_name, s.server_id";
	qrySettings->ParamByName(L"god")->AsString = cmbGod->Text;
	qrySettings->Open();
	while (!qrySettings->Eof) {
		ServerEndpoint endpoint;
		endpoint.id = AnsiString(qrySettings->FieldByName(L"server_id")->AsString).c_str();
		endpoint.displayName = UTF8String(qrySettings->FieldByName(L"display_name")->AsString).c_str();
		endpoint.host = AnsiString(qrySettings->FieldByName(L"host")->AsString).c_str();
		endpoint.port = AnsiString(qrySettings->FieldByName(L"port")->AsString).c_str();
		endpoint.enabled = true;
		endpoints.push_back(endpoint);
		qrySettings->Next();
	}
	if (endpoints.empty()) {
		if (errorMessage)
			*errorMessage = L"Для " + cmbGod->Text + L" года не настроено ни одного сервера";
		return false;
	}
	serverManager_.configure(endpoints, endpoints.front().id);
	serverManager_.setUser(user_);
	Edit2->Text = String(endpoints.front().host.c_str());
	return true;
}
std::string TfMain::primaryServerId() const
{
	return serverManager_.primaryServerId();
}
std::string TfMain::currentSourceServerId() const
{
	try {
		if (dbgEh && dbgEh->DataSource && dbgEh->DataSource->DataSet &&
			dbgEh->DataSource->DataSet->Active) {
			TField* field = dbgEh->DataSource->DataSet->FindField(L"source_server_id");
			if (field && !field->AsString.IsEmpty())
				return AnsiString(field->AsString).c_str();
		}
	}
	catch (...) {
	}
	return primaryServerId();
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
    try {
		serverManager_.setUser(user_name);
		std::string err;
		if (!serverManager_.connectPrimary(&err))
			ShowMessage("Ошибка регистрации на сервере: " + String(err.c_str()));
    }
    catch (const std::exception& e) {
        ShowMessage("Ошибка регистрации: " + String(e.what()));
    }
    catch (...) {
        ShowMessage("Неизвестная ошибка при регистрации на сервере");
    }
}

void __fastcall TfMain::finishSearch()
{
	if (!uiIsLive())
		return;
	if (!searchSuccess) {
		std::unique_ptr<TStringList> message(new TStringList());
		message->Add(String(searchError.c_str()));
		for (const auto& failure : searchFailures_async) {
			const String name = UTF8String(failure.displayName.c_str());
			message->Add(L"• " + name + L": " + String(failure.error.c_str()));
		}
		lbStatus->Caption = L"Поиск не выполнен: 0/" + String(searchServerTotalCount);
		ShowMessage(message->Text);
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

	EnsureResResultTextFieldSizes(qryTelegi, memTbl);
	try {
		qryTelegi->SQL->Text = ("select * from res");
		qryTelegi->Open();

		memTbl->Active = false;
		memTbl->Active = true;
	}
	catch (const Exception& e) {
		const String openErr =
			L"Не удалось открыть локальные результаты поиска: " + e.Message;
		if (searchSaveReport_.IsEmpty())
			searchSaveReport_ = openErr;
		else
			searchSaveReport_ = searchSaveReport_ + L"\r\n\r\n" + openErr;
		applyRchEdtText(L"", false);
		lbStatus->Caption = L"Ошибка отображения результатов";
		btnSearch->Enabled = true;
		return;
	}

    dbgEh->Columns->Items[4]->Title->SortMarker = smDownEh;
    dbgEh->DefaultApplySorting();

	int gg = dbgEh->RowCount;

	if(dbgEh->RowCount == 2) {
		dbgEh->SelectedRows->CurrentRowSelected = true;
		dbgEh->OnCellClick((TColumnEh*)lastSender);
	}
	if (TButton* button = dynamic_cast<TButton*>(lastSender))
		button->Enabled = true;
	dbgEh->Enabled = true;

	const int savedCount = (telegi.empty() ? 0 : searchSaveInserted_);
	const int attemptedCount = (telegi.empty() ? 0 : searchSaveAttempted_);
	const bool savePartial = searchSavePartial_;
	String statusServers = String(searchServerSuccessCount) + L"/" + String(searchServerTotalCount);

	if (!searchFailures_async.empty()) {
		std::unique_ptr<TStringList> failures(new TStringList());
		failures->Add(
			L"Получены ответы от " + String(searchServerSuccessCount) + L" из " +
			String(searchServerTotalCount) + L" серверов.");
		for (const auto& failure : searchFailures_async) {
			const String name = UTF8String(failure.displayName.c_str());
			failures->Add(L"• " + name + L": " + String(failure.error.c_str()));
		}
		if (savePartial) {
			failures->Add(L"");
			failures->Add(
				L"Локально сохранено " + String(savedCount) + L" из " +
				String(attemptedCount) +
				L" строк. Подробности ошибок записи — в панели текста.");
			lbStatus->Caption = L"Поиск завершён частично: серверов " + statusServers +
				L", сохранено " + String(savedCount) + L" из " + String(attemptedCount);
		}
		else {
			lbStatus->Caption = L"Поиск завершён частично: " + statusServers;
		}
		ShowMessage(failures->Text);
	}
	else if (savePartial) {
		lbStatus->Caption = L"Поиск завершён частично: сохранено " +
			String(savedCount) + L" из " + String(attemptedCount);
	}
	else {
		lbStatus->Caption = L"Поиск завершён: серверов " + String(searchServerSuccessCount) +
			L", результатов " + String(savedCount);
	}

	if(cmbReq->CanFocus())
	cmbReq->SetFocus();
}





void __fastcall TfMain::btnSearchClick(TObject *Sender)
{

	auto tok = std::make_shared<ComponentsGuard>(gSearchProcessing, this);
	if (!*tok) return;
	if (!uiIsLive())
		return;


   //	ComponentsGuard guard2(gSaveProcessing, this);
  //	if (!guard2) return;


	last_loaded_num_ = "";
	const bool isChkSQL = chkSQL->Checked;
	const bool isChkExc = chkExc->Checked;

	if (cmbReq->Text == "" && !isChkSQL) {
        return;
	}

	ComboHistory.add(AnsiString{cmbReq->Text}.c_str());
	clear();
	searchFailures_async.clear();
	searchServerSuccessCount = 0;
	searchServerTotalCount = 0;

	if (TButton* button = dynamic_cast<TButton*>(Sender))
		button->Enabled = false;
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
    const int rgTypeTIndex = rgTypeT->ItemIndex;
	int cmbMaxReqInt = 0;
    try { cmbMaxReqInt = StrToInt(cmbMaxReq->Text); } catch (...) {}
	std::string requestBody;
	COMMAND searchCommand = COMMAND::GETSQLJSONANSWEAR;
	AnsiString requestText = s;
	if (isChkSQL) {
		if (cmbReq->Text.IsEmpty()) {
			cmbField->Text = L"SQL";
			cmbReq->Text = L"1=1";
			cmbFieldText = L"SQL";
			requestText = AnsiString(cmbReq->Text);
		}
		SqlCondition::TYPE type;
		if (rgTypeTIndex == 0) {
			searchCommand = COMMAND::GET_VH_TELEGI_FROM_SQL;
			type = SqlCondition::TYPE::VH;
		}
		else {
			searchCommand = COMMAND::GET_ISH_TELEGI_FROM_SQL;
			type = SqlCondition::TYPE::ISH;
		}
		SqlCondition::max = cmbMaxReqInt;
		if (cmbFieldText == L"SQL") {
			requestText += " order by `index` desc limit " + IntToStr(SqlCondition::max);
			requestBody = conv::to_string(String(requestText).c_str());
		}
		else {
			auto buildCondition = SqlCondition::getFuncCondition(type, isChkExc);
			requestBody = conv::to_string(buildCondition(cmbFieldText, requestText).c_str());
		}
	}
	else {
		requestBody = std::string(requestText.c_str());
	}
	const std::vector<std::string> serverIds = serverManager_.serverIds();
	if (serverIds.empty()) {
		searchSuccess = false;
		searchError = "Для выбранного года нет настроенных серверов";
		lastSender = Sender;
		finishSearch();
		return;
	}
    // --- Запуск фоновой задачи ---
	auto live = uiLive_;
	auto op = trackBackgroundOp();
	System::Threading::TTask::Run([this, tok, requestBody, searchCommand, serverIds, Sender, live, op]() {
		struct Outcome {
			std::string serverId;
			std::string displayName;
			std::vector<Telega> hits;
			std::string error;
			bool success{false};
		};
		try {
		std::vector<Outcome> outcomes(serverIds.size());
		std::vector<std::thread> workers;
		workers.reserve(serverIds.size());
		for (std::size_t i = 0; i < serverIds.size(); ++i) {
			workers.emplace_back([this, &outcomes, &serverIds, &requestBody, searchCommand, i, live]() {
				if (live && !live->load(std::memory_order_acquire))
					return;
				Outcome& outcome = outcomes[i];
				outcome.serverId = serverIds[i];
				outcome.displayName = serverManager_.displayName(outcome.serverId);
				try {
					std::string response;
					if (!serverManager_.requestText(
							outcome.serverId, requestBody, searchCommand, response, nullptr,
							&outcome.error))
						return;
					if (response == "update") {
						outcome.error = "Идёт обновление базы";
						return;
					}
					outcome.hits = JSON::get<std::vector<Telega>>(response);
					for (auto& hit : outcome.hits)
						hit.source_server_id = outcome.serverId;
					outcome.success = true;
				}
				catch (const std::exception& e) {
					outcome.error = e.what();
				}
				catch (...) {
					outcome.error = "Ошибка разбора ответа";
				}
			});
		}
		for (auto& worker : workers)
			if (worker.joinable()) worker.join();
		std::vector<Telega> merged;
		std::vector<ServerSearchFailure> failures;
		int successes = 0;
		for (auto& outcome : outcomes) {
			if (outcome.success) {
				++successes;
				merged.insert(merged.end(),
					std::make_move_iterator(outcome.hits.begin()),
					std::make_move_iterator(outcome.hits.end()));
			}
			else {
				failures.push_back({outcome.serverId, outcome.displayName, outcome.error});
			}
		}
		this->telegi_async = std::move(merged);
		this->searchFailures_async = std::move(failures);
		this->searchServerSuccessCount = successes;
		this->searchServerTotalCount = static_cast<int>(outcomes.size());
		this->searchSuccess = successes > 0;
		this->searchError = successes > 0
			? std::string{}
			: std::string("Ни один сервер не ответил успешно");
		this->lastSender = Sender;
		} catch (...) {
			this->searchSuccess = false;
			this->searchError = "Ошибка выполнения поиска";
		}
		TThread::Queue(nullptr, [this, tok, live, op]() {
			if (live && live->load(std::memory_order_acquire) && !shuttingDown_)
				finishSearch();
		});
	});
}


//---------------------------------------------------------------------------
void __fastcall TfMain::FormClose(TObject *Sender, TCloseAction &Action)
{
	if (saveRunning.load(std::memory_order_acquire)) {
		Action = caNone;
		ShowMessage("Дождитесь завершения сохранения файлов или нажмите \"Отмена\".");
		return;
	}

	if (uiLive_)
		uiLive_->store(false, std::memory_order_release);
	if (tmLoadText)
		tmLoadText->Enabled = false;
	if (responsiveLayoutTimer_)
		responsiveLayoutTimer_->Enabled = false;

	for (int spin = 0; spin < 300 && uiInflight_.load(std::memory_order_acquire) > 0; ++spin) {
		Application->ProcessMessages();
		Sleep(20);
	}
	if (uiInflight_.load(std::memory_order_acquire) > 0) {
		if (uiLive_)
			uiLive_->store(true, std::memory_order_release);
		Action = caNone;
		ShowMessage(L"Дождитесь завершения фоновых операций (поиск, детали, загрузка, авторизация).");
		return;
	}

	shuttingDown_ = true;
	SaveMainWindowPlacement();
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


	EnsureResResultTextFieldSizes(qryTelegi, memTbl);
	qryTelegi->SQL->Text = ("select * from res");
	qryTelegi->Open();

	memTbl->Active = false;
	memTbl->Active = true;

	searchSaveReport_ = L"";
	searchSaveAttempted_ = 0;
	searchSaveInserted_ = 0;
	searchSaveFailed_ = 0;
	searchSavePartial_ = false;
	rchEdt->Text = "";
	ggSaveProgress->Progress = 0;
	Label1->Caption = "Relative index:";
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void __fastcall TfMain::btnConnectClick(TObject *Sender)
{
	if (saveRunning.load(std::memory_order_acquire)) {
		ShowMessage("Дождитесь завершения сохранения файлов.");
		return;
	}
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

	try {
		lbStatus->Caption = "Статус: подключение к серверам, " + cmbGod->Text + " год ...";


		qryTelegi->SQL->Text = ("delete from res");
		qryTelegi->ExecSQL();


		qryTelegi->SQL->Text = ("select * from res");
		qryTelegi->Open();

		memTbl->Active = false;
		memTbl->Active = true;


		clear();

		if(tryConnect() && auth_)
		{
			registryOnServer(user_);
		}
	}
	catch (const Exception& e) {
		ShowMessage("Ошибка подключения: " + e.Message);
		lbStatus->Caption = "Статус: ошибка подключения";
	}
	catch (...) {
		ShowMessage("Неизвестная ошибка при подключении");
		lbStatus->Caption = "Статус: ошибка подключения";
	}

	Application->ProcessMessages();
}
//---------------------------------------------------------------------------

void TfMain::disConnect()
{
	disconnectUiFull();
}

void TfMain::disconnectTransportOnly()
{
	serverManager_.disconnectAll();
}
void TfMain::disconnectUiFull()
{
	disconnectTransportOnly();
	lbStatus->Caption = "Статус: отключено от сервера";
	PageControl1->ActivePageIndex = 0;
	componentsEnable(false);
}

bool TfMain::reconnectTransport(const std::string& requestedServerId)
{
	const std::string serverId = requestedServerId.empty() ? primaryServerId() : requestedServerId;
	std::string err;
	if (!serverManager_.reconnect(serverId, &err)) {
		pushLog("Авто-реконнект не выполнен: " + String(err.c_str()));
		return false;
	}
	pushLog("Авто-реконнект выполнен: " + String(serverId.c_str()));
	return true;
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
		btnCancelSave->Enabled = !b && saveRunning.load(std::memory_order_acquire);
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


// JSON received from the server stores text as UTF-8.  Converting these bytes
// through AnsiString makes FireDAC persist mojibake (for example, "Р..." in
// place of Cyrillic text) because its string parameters are Unicode.
inline UnicodeString US(const std::string& s)
{
	return UTF8String(s.c_str());
}

// Match persistent TWideStringField.Size on qryTelegi/memTbl (formMain.dfm).
// Values longer than this are skipped at insert so FireDAC Open does not abort
// the whole result set with [DatS]-32 overflow.
static const int kResUiTextMaxChars = 2048;

static void EnsureResResultTextFieldSizes(TFDQuery* qry, TMemTableEh* mem)
{
	if (qry != nullptr) {
		for (int i = 0; i < qry->FieldCount; ++i) {
			if (auto* f = dynamic_cast<TWideStringField*>(qry->Fields->Fields[i]))
				f->Size = kResUiTextMaxChars;
		}
	}
	if (mem == nullptr)
		return;

	const bool wasActive = mem->Active;
	if (wasActive)
		mem->Active = false;

	for (int i = 0; i < mem->FieldDefs->Count; ++i) {
		TFieldDef* def = mem->FieldDefs->Items[i];
		if (def->DataType == ftWideString || def->DataType == ftString)
			def->Size = kResUiTextMaxChars;
	}
	for (int i = 0; i < mem->FieldCount; ++i) {
		if (auto* f = dynamic_cast<TWideStringField*>(mem->Fields->Fields[i]))
			f->Size = kResUiTextMaxChars;
	}

	if (wasActive)
		mem->Active = true;
}


//---------------------------------------------------------------------------
void TfMain::convTelegs(std::vector<Telega>& telegi)
{
    static const String SQL_INSERT_RES =
        "insert into res ("
        "num, date, podp_num, date_podp, tel_num, isp, from_to, rel, dir, type, kr, "
        "pril_name, pril_count, last_mesto, gde_sht, blank, deleted, source_server_id"
        ") values ("
        ":num, :date, :podp_num, :date_podp, :tel_num, :isp, :from_to, :rel, :dir, :type, :kr, "
        ":pril_name, :pril_count, :last_mesto, :gde_sht, :blank, :deleted, :source_server_id"
        ")";

	EnsureResResultTextFieldSizes(qryTelegi, memTbl);

    qryTelegi->Close();
    qryTelegi->SQL->Text = SQL_INSERT_RES;

    // типы параметров
    auto P = qryTelegi->Params;
    P->ParamByName("num")->DataType        = ftWideString;
    P->ParamByName("podp_num")->DataType   = ftWideString;
    P->ParamByName("tel_num")->DataType    = ftWideString;
    P->ParamByName("isp")->DataType        = ftWideString;
    P->ParamByName("from_to")->DataType    = ftWideString;
	P->ParamByName("dir")->DataType        = ftWideString;
	P->ParamByName("type")->DataType 	   = ftWideString;
	P->ParamByName("kr")->DataType         = ftWideString;
    P->ParamByName("pril_name")->DataType  = ftWideString;
    P->ParamByName("pril_count")->DataType = ftWideString;
    P->ParamByName("last_mesto")->DataType = ftWideString;
    P->ParamByName("gde_sht")->DataType    = ftWideString;
    P->ParamByName("blank")->DataType      = ftWideString;
    P->ParamByName("deleted")->DataType    = ftInteger;
	P->ParamByName("source_server_id")->DataType = ftWideString;

    P->ParamByName("rel")->DataType        = ftFloat;
    P->ParamByName("date")->DataType       = ftDate;
    P->ParamByName("date_podp")->DataType  = ftDate;

    qryTelegi->Prepared = true;

    ggSaveProgress->MaxValue = telegi.size();
    ggSaveProgress->Progress = 0;

    searchSaveAttempted_ = static_cast<int>(telegi.size());
	searchSaveInserted_ = 0;
    searchSaveFailed_ = 0;
    searchSavePartial_ = false;
	searchSaveReport_ = L"";

    std::unique_ptr<TStringList> report(new TStringList());
    TFDCustomConnection* conn = qryTelegi->Connection;
    bool tranStarted = false;

    try {
        conn->StartTransaction();
        tranStarted = true;
    }
    catch (const Exception& e) {
        report->Add(L"Не удалось начать транзакцию: " + e.Message);
        report->Add(L"Запись продолжена без транзакции; при сбое часть строк могла остаться в базе.");
        report->Add(L"");
    }

    auto normalize_gde_sht = [](std::string s)
    {
        if (std::none_of(s.begin(), s.end(), ::isdigit))
            s.clear();
        return s;
    };

    auto rowLabel = [](int index1, const Telega& t, const String& type) -> String {
        String label = L"Строка " + String(index1) + L" [" + type + L"]";
        if (!t.num.empty())
            label += L", № " + US(t.num);
        if (!t.podp_num.empty())
            label += L", подп. " + US(t.podp_num);
        if (!t.source_server_id.empty())
            label += L", сервер " + US(t.source_server_id);
        return label;
    };

    int index1 = 0;
    for (auto& t : telegi)
    {
        ++index1;
        // не модифицируй исходные поля без нужды, но раз уж делаешь:
        std::replace(t.from_to.begin(), t.from_to.end(), '\'', '"');
        std::replace(t.kr.begin(),      t.kr.end(),      '\'', '"');

        String type;
        switch (t.type) {
        case Telega::TYPE::ISHOD: type = L"Исх."; break;
        case Telega::TYPE::NOTTG: type = L"Не тг."; break;
        default: type = L"Вх."; break;
        }

        String oversizedDetails;
        auto noteLongField = [&](const wchar_t* fieldName, const UnicodeString& value) {
            if (value.Length() <= kResUiTextMaxChars)
                return;
            if (!oversizedDetails.IsEmpty())
                oversizedDetails += L"; ";
            oversizedDetails +=
                String(fieldName) + L" (" + String(value.Length()) + L" симв.)";
        };
        noteLongField(L"isp", US(t.isp));
        noteLongField(L"type", type);
        noteLongField(L"podp_num", US(t.podp_num));
        noteLongField(L"from_to", US(t.from_to));
        noteLongField(L"dir", US(t.dir));
        noteLongField(L"kr", US(t.kr));
        noteLongField(L"pril_name", US(t.pril_name));
        noteLongField(L"pril_count", US(t.pril_count));
        noteLongField(L"last_mesto", US(t.last_mesto));
        noteLongField(L"gde_sht", US(normalize_gde_sht(t.gde_sht)));
        noteLongField(L"blank", US(t.blank));
        noteLongField(L"source_server_id", US(t.source_server_id));
        if (!oversizedDetails.IsEmpty()) {
            ++searchSaveFailed_;
            report->Add(
                rowLabel(index1, t, type) +
                L": слишком длинные текстовые поля — " + oversizedDetails +
                L"; лимит " + String(kResUiTextMaxChars) +
                L" символов. Строка пропущена.");
            ggSaveProgress->AddProgress(1);
            Application->ProcessMessages();
            continue;
        }

        try
        {
            // date_podp
            try {
				TDateTime d2 = StrToDate(US(t.date_podp));
                P->ParamByName("date_podp")->Value = d2;
            } catch (const Exception& e) {
                P->ParamByName("date_podp")->Value = Null();
                if (!t.date_podp.empty()) {
                    report->Add(
                        rowLabel(index1, t, type) +
                        L": не разобрана дата подписи \"" + US(t.date_podp) +
                        L"\" — " + e.Message + L". Записано пустое значение.");
                }
            } catch (...) {
                P->ParamByName("date_podp")->Value = Null();
                if (!t.date_podp.empty()) {
                    report->Add(
                        rowLabel(index1, t, type) +
                        L": не разобрана дата подписи \"" + US(t.date_podp) +
                        L"\". Записано пустое значение.");
                }
            }

            // date
            try {
                TDateTime d1 = StrToDate(US(t.date));
                P->ParamByName("date")->Value = d1;
            } catch (const Exception& e) {
                P->ParamByName("date")->Value = Null();
                if (!t.date.empty()) {
                    report->Add(
                        rowLabel(index1, t, type) +
                        L": не разобрана дата \"" + US(t.date) +
                        L"\" — " + e.Message + L". Записано пустое значение.");
                }
            } catch (...) {
                P->ParamByName("date")->Value = Null();
                if (!t.date.empty()) {
                    report->Add(
                        rowLabel(index1, t, type) +
                        L": не разобрана дата \"" + US(t.date) +
                        L"\". Записано пустое значение.");
                }
            }

            // строки — БЕЗ c_str()
            P->ParamByName("num")->Value        = US(t.num);
            P->ParamByName("podp_num")->Value   = US(t.podp_num);
            P->ParamByName("tel_num")->Value    = US(t.tel_num);
            P->ParamByName("isp")->Value        = US(t.isp);
            P->ParamByName("from_to")->Value    = US(t.from_to);
            P->ParamByName("rel")->Value        = t.rel;
            P->ParamByName("dir")->Value        = US(t.dir);
			P->ParamByName("type")->Value       = type;
            P->ParamByName("kr")->Value         = US(t.kr);
            P->ParamByName("pril_name")->Value  = US(t.pril_name);
            P->ParamByName("pril_count")->Value = US(t.pril_count);
            P->ParamByName("last_mesto")->Value = US(t.last_mesto);
            P->ParamByName("gde_sht")->Value    = US(normalize_gde_sht(t.gde_sht));
            P->ParamByName("blank")->Value      = US(t.blank);
            P->ParamByName("deleted")->Value    = t.deleted ? 1 : 0;
			P->ParamByName("source_server_id")->Value = US(t.source_server_id);

            qryTelegi->ExecSQL();
            ++searchSaveInserted_;
        }
        catch (const Exception& e)
        {
            ++searchSaveFailed_;
            report->Add(
                rowLabel(index1, t, type) +
                L": ошибка вставки — " + e.Message
            );
        }
        catch (...)
        {
            ++searchSaveFailed_;
            report->Add(
                rowLabel(index1, t, type) +
                L": ошибка вставки — неизвестное исключение"
            );
        }

        ggSaveProgress->AddProgress(1);
        Application->ProcessMessages();
    }

    if (tranStarted)
    {
        try { conn->Commit(); }
        catch (const Exception& e) {
            try { conn->Rollback(); } catch (...) {}
            report->Add(L"Ошибка фиксации транзакции: " + e.Message + L". Выполнен откат.");
            searchSaveInserted_ = 0;
            searchSaveFailed_ = searchSaveAttempted_;
        }
        catch (...) {
            try { conn->Rollback(); } catch (...) {}
            report->Add(L"Ошибка фиксации транзакции. Выполнен откат.");
            searchSaveInserted_ = 0;
            searchSaveFailed_ = searchSaveAttempted_;
        }
    }

    ggSaveProgress->Progress = ggSaveProgress->MaxValue;

    // Soft date-parse warnings keep inserted==attempted; still show the report.
    // Partial means lost rows and/or writing without a started transaction.
    searchSavePartial_ =
        (searchSaveFailed_ > 0) ||
        (searchSaveInserted_ < searchSaveAttempted_) ||
        (!tranStarted && searchSaveAttempted_ > 0);

    if (!report->Text.IsEmpty() || searchSavePartial_) {
        std::unique_ptr<TStringList> header(new TStringList());
        if (searchSavePartial_) {
            header->Add(
                L"Локальное сохранение результатов частичное: " +
                String(searchSaveInserted_) + L" из " + String(searchSaveAttempted_) +
                L" строк."
            );
        }
        else {
            header->Add(
                L"Локальное сохранение результатов: " +
                String(searchSaveInserted_) + L" из " + String(searchSaveAttempted_) +
                L" строк (есть предупреждения)."
            );
        }
        if (searchSaveFailed_ > 0)
            header->Add(L"Не сохранено строк: " + String(searchSaveFailed_) + L".");
        header->Add(L"Ниже — где и что не разобралось или не записалось:");
        header->Add(L"");
        if (!report->Text.IsEmpty())
            header->AddStrings(report.get());
        searchSaveReport_ = header->Text;
        rchEdt->Lines->Text = searchSaveReport_;
    }
}




void __fastcall TfMain::FormCreate(TObject *Sender)
{
	(void)Sender;
	Align = alNone;
	AlignWithMargins = false;
	AutoSize = false;
	ScreenSnap = false;
	DragMode = dmManual;
	Position = poDesigned;
	const HWND mainWindowHandle = Handle;
	(void)mainWindowHandle;
	RestoreMainWindowPlacement();
	CreateResponsiveScrollHost();
	responsiveLayoutTimer_ = new TTimer(this);
	responsiveLayoutTimer_->Enabled = false;
	responsiveLayoutTimer_->Interval = RESPONSIVE_LAYOUT_INTERVAL_MS;
	responsiveLayoutTimer_->OnTimer = ResponsiveLayoutTimerTimer;

	const String productVersion = GetSearchClientProductVersion();
	Caption = productVersion.IsEmpty() ? L"SearchClient_asio" : L"SearchClient_asio v" + productVersion;
	InitDbgMenuDS();
	EnsureResResultTextFieldSizes(qryTelegi, memTbl);

	ComboHistory = RecentComboHistory(cmbReq,9);
	myRequest::init(cmbField, cmbReq, chkSQL, chkExc, rgTypeT);

	SqlCondition::map =    getMapFromQry(qrySettings, "map");
	SqlCondition::mapVh =  getMapFromQry(qrySettings, "map_vh");
	SqlCondition::mapIsh = getMapFromQry(qrySettings, "map_ish");

	Edit2->Text = L"localhost";
	auto sl = new TStringList();
	try
	{
		const String legacyIpPath =
			ExtractFilePath(Application->ExeName) + L"SERVER_IP.INI";
		if (FileExists(legacyIpPath)) {
			sl->LoadFromFile(legacyIpPath);
			if (sl->Count > 0) {
				const String legacyHost = Trim(sl->Strings[0]);
				if (!legacyHost.IsEmpty())
					Edit2->Text = legacyHost;
			}
		}
	}
	catch(...)
	{
		// SERVER_IP.INI is an optional legacy fallback. The server catalog in
		// Settings.db3 is the primary configuration source.
		Edit2->Text = L"localhost";
	}
	delete sl;

	ensureServerCatalogSeeded(Edit2->Text);
	reloadAvailableYears();
	String serverConfigError;
	if (!configureServersForCurrentYear(&serverConfigError) && !serverConfigError.IsEmpty())
		lbStatus->Caption = L"Статус: " + serverConfigError;



	LoadTemplatesFromDisk();

	for(const auto& i : myRequest::groupsNames())
		lstShGroups->Items->Add(i);


	if(!useF12Control())
		dbgEh->RowDetailPanel->Active = false;


   //	return;

	myFrAttachments = new TfrAttachments(nullptr, serverManager_, dbgEh, dbgMenu);
	myFrAttachments->Parent = dbgMenu->RowDetailPanelControl;
	myFrAttachments->setHeigthMulti(0.7);
	myFrAttachments->Align = alClient;
	myFrAttachments->Visible = false;



	myFrWay = new TfrWay(nullptr, serverManager_, dbgEh, dbgMenu);
	myFrWay->Parent = dbgMenu->RowDetailPanelControl;
	myFrWay->setHeigthMulti(1.6);
	myFrWay->Align = alClient;
	myFrWay->Visible = false;


	myFrPdtv  = new TfrPdtv(nullptr, serverManager_, dbgEh, dbgMenu);
	myFrPdtv->Parent = dbgMenu->RowDetailPanelControl;
	myFrPdtv->setHeigthMulti(1.4);
	myFrPdtv->Align = alClient;
	myFrPdtv->Visible = false;

	auto bindFrameLife = [this](TFrameDbg* frame) {
		if (!frame)
			return;
		frame->BindLife(uiLive_, [this]() { return trackBackgroundOp(); });
	};
	bindFrameLife(myFrAttachments);
	bindFrameLife(myFrWay);
	bindFrameLife(myFrPdtv);
	ScheduleResponsiveLayout();
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

    if (isDeletedRow(dbgEh->DataSource->DataSet)) {
        ShowMessage(deletedFileMessage());
        Handled = true;
        return;
    }

    const String ss = dbgEh->Fields[9]->AsString;
    if (ss.IsEmpty()) {
        Handled = true;
        return;
    }

    std::string err;

	std::string str;
	if (!serverManager_.requestText(
			currentSourceServerId(), std::string(AnsiString(ss).c_str()),
			COMMAND::FILETEXT, str, nullptr, &err)) {
		ShowMessage("Ошибка запроса: " + AnsiString(err.c_str()));
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
	else {
		// Newly added server/year rows become selectable without restarting the client.
		reloadAvailableYears();
	}
	ScheduleResponsiveLayout();
}




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

void __fastcall TfMain::dbgEhDrawColumnCell(TObject *Sender, const TRect &Rect,
	int DataCol, TColumnEh *Column, Gridseh::TGridDrawState State)
{
	TDBGridEh* grid = static_cast<TDBGridEh*>(Sender);
	TDataSet* ds = grid->DataSource ? grid->DataSource->DataSet : nullptr;
	const bool deleted = isDeletedRow(ds);

	if (deleted) {
		grid->Canvas->Font->Color = clGrayText;
		grid->Canvas->Font->Style = grid->Canvas->Font->Style << fsItalic;
	}

	if (Column && Column->FieldName == "deleted") {
		grid->Canvas->FillRect(Rect);
		const String text = deleted ? L"удалён" : L"";
		grid->Canvas->TextRect(Rect, Rect.Left + 4, Rect.Top + 2, text);
		return;
	}

	grid->DefaultDrawColumnCell(Rect, DataCol, Column, State);
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
					   const RemoteFileRequest& f,
					   bool flushStreamToOs)
{
	std::string file_name = f.outputName;
    std::string full_file_name = out_dir + "\\" + file_name;
	std::string pref;
    int try_limit = 1;

    // Проверка на существование файла
	if (FileExists(String{full_file_name.c_str()})) {


        do {
            pref = "(" + std::to_string(try_limit++) + ") ";
			file_name = pref + f.outputName;
            full_file_name = out_dir + "\\" + file_name;
        }
        while (FileExists(String{full_file_name.c_str()}) && try_limit <= 30);

        if (try_limit > 30) {
			pushLog("Ошибка при генерации имени файла (слишком много коллизий имён)!");
            return 0;
        }
    }
	constexpr int MAX_TRIES = 5;
	constexpr int RETRY_DELAY_MS = 500;
	std::string err;
	for (int attempt = 1; attempt <= MAX_TRIES; ++attempt) {
		std::uint64_t receivedSize = 0;
		if (serverManager_.requestFileToPath(
				f.serverId,
				f.remotePath,
				COMMAND::GETBINFILE,
				full_file_name,
				nullptr,
				&receivedSize,
				&err,
				flushStreamToOs)) {
			std::string result;
			if (file_name == f.outputName)
				result = "\"" + f.outputName + "\"";
			else
				result = "\"" + f.outputName + "\" (\"" + file_name + "\")";
			pushLog(("Файл " + result + ", " + std::to_string(receivedSize) +
				" байт - успешно сохранен!").c_str());
			return 1;
		}
		if (err == busyServerErrorUserMessage() && attempt < MAX_TRIES) {
			pushLog(
				"Сервер занят при запросе файла \"" + String(f.remotePath.c_str()) +
				"\", повтор " + String(attempt + 1) + " из " + String(MAX_TRIES)
			);
			if (!reconnectTransport(f.serverId))
				break;
			std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
			continue;
		}
		break;
	}
	pushLog("Ошибка сохранения файла \"" + String(f.outputName.c_str()) + "\": " +
		String(err.c_str()));
	// После неполной сетевой передачи поток протокола больше нельзя продолжать.
	// requestFileToPath закрывает такой сокет; восстанавливаем транспорт для
	// следующих файлов пакета и для последующих действий пользователя.
	if (!serverManager_.isConnected(f.serverId))
		reconnectTransport(f.serverId);
	return 0;
}



int TfMain::SaveFileVh(const std::string& out_dir,
					   const RemoteFileRequest& f,
	bool makeDocx,
					   bool flushStreamToOs,
					   DocxConversionSession* docxSession)
{
	(void)flushStreamToOs; // WriteNewFile всегда делает FlushFileBuffers перед публикацией

	std::vector<uint8_t> str;
	constexpr int MAX_TRIES = 5;
	constexpr int RETRY_DELAY_MS = 500;

	std::string err;
	bool ok = false;

	for (int attempt = 1; attempt <= MAX_TRIES; ++attempt)
	{
		if (saveCancelRequested.load(std::memory_order_acquire))
			return 0;
		str.clear();
		err.clear();



		ok = serverManager_.requestBinFile(f.serverId, f.remotePath, str, nullptr, &err);

		if (ok && !str.empty())
			break;

		// проверка по строке
		if (err == busyServerErrorUserMessage() && attempt < MAX_TRIES)
		{
			if (!reconnectTransport(f.serverId))
				break;
			std::this_thread::sleep_for(
				std::chrono::milliseconds(RETRY_DELAY_MS)
			);
            pushLog(
			"Запрос файла \"" + String(f.remotePath.c_str()) +
			"\", повтор " + String(attempt + 1) + " из " + String(MAX_TRIES)
			);


			continue;
		}

		// любая другая ошибка — не повторяем
		break;
	}

	if (!ok || str.empty())
	{
		pushLog(
			"Файл \"" + String(f.remotePath.c_str()) +
			"\" не получен: " + String(err.c_str())
		);
		if (!serverManager_.isConnected(f.serverId))
			reconnectTransport(f.serverId);
		return 0;
	}




	std::string file_name = f.outputName;
	namespace fs = std::filesystem;
	const fs::path baseDir(String(out_dir.c_str()).c_str());
	fs::path absolutePath;
	std::wstring pathError;
	if (!client_file_publish::ResolveUnderBase(
			baseDir, fs::path(String(file_name.c_str()).c_str()), absolutePath, &pathError)) {
		pushLog(L"Небезопасное имя файла \"" + String(file_name.c_str()) + L"\": " +
			String(pathError.c_str()));
		return 0;
	}

	if (makeDocx)
    {
        try {
			if (saveCancelRequested.load(std::memory_order_acquire))
				return 0;
            String shtPath = ExtractFilePath(Application->ExeName) + "sht.docm";
			fs::path docxPath;
			const String docxRel = ChangeFileExt(String(f.outputName.c_str()), L".docx");
			if (!client_file_publish::ResolveUnderBase(
					baseDir, fs::path(docxRel.c_str()), docxPath, &pathError)) {
				pushLog(L"Небезопасное имя DOCX \"" + docxRel + L"\": " + String(pathError.c_str()));
				return 0;
			}
			if (!client_file_publish::EnsureUniqueFilePath(docxPath)) {
				pushLog("Не удалось подобрать свободное имя DOCX для файла \"" +
					String(f.outputName.c_str()) + "\"");
				return 0;
			}
			const String outDocx = String(docxPath.c_str());

            // Преобразуем байты в текст, применяя OemToAnsi
            AnsiString raw((const char*)&str[0], str.size());
            OemToAnsi(raw.c_str(), raw.c_str());
            raw = raw.UpperCase();

            TStringList* sl = new TStringList();
            sl->Text = raw;
            String text = sl->Text;
			delete sl;

			pushLog("DOCX: запуск обработки файла \"" + String(f.outputName.c_str()) + "\"");
			DocxConversionResult conversion;
			if (docxSession) {
				conversion = docxSession->Convert(
					text,
					outDocx,
					DOCX_CONVERSION_TIMEOUT_MS,
					&saveCancelRequested);
			}
			else {
				conversion = ConvertPlainTextToDocxWithTimeout(
					text,
					L"Обработка_ШТ_2022",
					outDocx,
					shtPath,
					DOCX_CONVERSION_TIMEOUT_MS,
					&saveCancelRequested);
			}

			if (conversion.status == DocxConversionStatus::Cancelled) {
				pushLog("Сохранение DOCX отменено во время обработки файла \"" +
					String(f.outputName.c_str()) + "\".");
				return 0;
			}
			if (conversion.status == DocxConversionStatus::TimedOut) {
				pushLog("Файл \"" + String(f.outputName.c_str()) +
					"\" не удалось обработать: " + conversion.message +
					". Файл пропущен, обработка продолжается.");
				return 0;
			}
			if (conversion.status != DocxConversionStatus::Success) {
				pushLog("Ошибка при создании DOCX для файла \"" +
					String(f.outputName.c_str()) + "\": " + conversion.message);
				return 0;
			}

			pushLog("DOCX создан: " + conversion.outputPath + " (" +
				String(conversion.elapsedMs) + " мс)");

		}
        catch (const Exception &e) {
			pushLog("Ошибка при создании DOCX: " + e.Message);
            return 0;
		}
    }
    else
	{
		if (!client_file_publish::EnsureUniqueFilePath(absolutePath)) {
			pushLog(L"Не удалось подобрать свободное имя для \"" +
				String(f.outputName.c_str()) + L"\"");
			return 0;
		}

		if (!client_file_publish::WriteNewFile(
				absolutePath, str.data(), str.size(), &pathError)) {
			pushLog(L"Ошибка записи файла \"" + String(absolutePath.c_str()) +
				L"\": " + String(pathError.c_str()));
			return 0;
		}

		const String publishedName = String(absolutePath.filename().c_str());
        std::string result;
		if (publishedName == String(f.outputName.c_str()))
			result = "\"" + f.outputName + "\"";
		else
			result = "\"" + f.outputName + "\" (\"" + std::string(AnsiString(publishedName).c_str()) + "\")";

		pushLog(("Файл " + result + ", " + std::to_string(str.size()) + " байт - успешно сохранен!").c_str());
    }

	return 1;
}


int TfMain::SaveFileIsh(const std::string& out_dir, const RemoteArchiveRequest& f,
	bool flushStreamToOs)
{
	constexpr int MAX_TRIES = 5;
	constexpr int RETRY_DELAY_MS = 500;

	AnsiString zip_dir = f.remotePath;
	std::string err;
	bool ok = false;

	std::vector<uint8_t> str;

	for (int attempt = 1; attempt <= MAX_TRIES; ++attempt)
	{
		err.clear();
		str.clear();
		ok = serverManager_.requestBinFile(
			f.serverId, std::string(zip_dir.c_str()), str, nullptr, &err);
		if (ok && !str.empty())
			break;
		if (err == busyServerErrorUserMessage() && attempt < MAX_TRIES)
		{
			if (!reconnectTransport(f.serverId))
				break;
			pushLog(
				"Сервер занят при запросе файла \"" + zip_dir +
				"\", повтор " + String(attempt + 1) + " из " + String(MAX_TRIES)
			);
			std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
			continue;
		}
		break;
	}

	if (!ok || str.empty())
	{
		pushLog(
			"Файл \"" + zip_dir +
			"\" не получен. Ошибка: " +
			String(err.c_str())
		);
		if (!serverManager_.isConnected(f.serverId))
			reconnectTransport(f.serverId);
		return 0;
	}



	static int cc = 0;
	++cc;

	AnsiString temp_zip_dir = "ZipExtractTemp" + IntToStr(cc) + "\\";
	AnsiString zip_name = ExtractFileName(f.remotePath);
	AnsiString tempDir = TPath::GetTempPath() + temp_zip_dir;
	AnsiString fullDirZip = tempDir + zip_name;
	TDirectory::CreateDirectory(tempDir);

	FILE* file = fopen((fullDirZip).c_str(), "wb");

	if (file)
	{
		const std::size_t written = fwrite(str.data(), sizeof(BYTE), str.size(), file);
		if (written != str.size())
		{
			fclose(file);
			TFile::Delete(fullDirZip);
			pushLog(("Ошибка записи временного ZIP: " + String(fullDirZip)).c_str());
			return 0;
		}
		if (flushStreamToOs && fflush(file) != 0)
		{
			fclose(file);
			TFile::Delete(fullDirZip);
			pushLog(("Ошибка сброса буфера временного ZIP: " + String(fullDirZip)).c_str());
			return 0;
		}
		if (fclose(file) != 0)
		{
			TFile::Delete(fullDirZip);
			pushLog(("Ошибка закрытия временного ZIP: " + String(fullDirZip)).c_str());
			return 0;
		}
	}
	else
	{
		pushLog("Не удалось открыть файл для записи: " + tempDir);
		return 0; // Неудача
	}

	if(cc > 99999)
		cc = 0;

	const auto extractStats = copyFile(f.files, fullDirZip, String{out_dir.c_str()});

	TDirectory::Delete(tempDir, true);

	if (!extractStats.fullyOk()) {
		pushLog(
			L"ZIP \"" + String(zip_name) + L"\": распаковано " +
			String(extractStats.extracted) + L" из " + String(extractStats.expected) +
			L" файлов."
		);
		for (const auto& item : extractStats.errors)
			pushLog(String(item.c_str()));
		return 0;
	}

	pushLog(("Файл " + std::string{zip_name.c_str()} + ", " + std::to_string(str.size()) + " байт - успешно сохранен и распакован!").c_str());

	return 1; // Успех
}

//---------------------------------------------------------------------------


void __fastcall TfMain::btnSaveClick(TObject *Sender)
{

	ComponentsGuard compGuard(gSaveProcessing, this);
    if (!compGuard) return;
	saveCancelRequested.store(false, std::memory_order_release);



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

	if (chkDocx->Checked) {
		const String shtPath = ExtractFilePath(Application->ExeName) + "sht.docm";
		if (!FileExists(shtPath)) {
			ShowMessage("Не найден шаблон DOCX-макроса: " + shtPath);
			return;
		}
	}

	// ===== подготовка данных (в UI потоке) =====

	std::vector<RemoteFileRequest> pril_map_vh;
	std::vector<RemoteFileRequest> text_files;
	std::map<std::string, RemoteArchiveRequest> pril_map_ish;

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
		const std::string sourceServerId = currentSourceServerId();

		if (isDeletedRow(dbgEh->DataSource->DataSet)) {
			preLog->Add("Шт. " + num + " - файл удалён (только след в индексе)");
			continue;
		}

        if (chkPril->Checked && pril_count == 0) {
            preLog->Add("Шт. " + num + " - не содержит приложений!");
            if (!(chkText->Checked || chkDocx->Checked))
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
					pril_map_vh.push_back({sourceServerId, pn, dir_file_name_pril});
				else {
					auto ish_zip_dir = ExtractFileDir(dd) + "\\" + num + ".zip";
					const std::string archiveKey = sourceServerId + "\n" +
						std::string(AnsiString(ish_zip_dir).c_str());
					auto& archive = pril_map_ish[archiveKey];
					archive.serverId = sourceServerId;
					archive.remotePath = ish_zip_dir;
					archive.files.push_back(String{pn.c_str()});
				}
            }
        }

        if (chkText->Checked || chkDocx->Checked) {
			fs::path filePath = fs::path{dir_file_name.c_str()}.filename();
            if (filePath.extension().empty())
                filePath.replace_extension(".tlg");
			text_files.push_back({sourceServerId, filePath.string(), dir_file_name});
        }
	}

	const int total_file_count = static_cast<int>(
		pril_map_vh.size() + pril_map_ish.size() + text_files.size());
    if (!total_file_count) {
		// Фоновая задача не запускается, поэтому обычный перенос preLog
		// через pendingLog/tmSaveProgress здесь невозможен. Показываем
		// подготовленные сообщения сразу в области журнала.
		for (int i = 0; i < preLog->Count; ++i)
			rchEdt->Lines->Add(preLog->Strings[i]);
        delete preLog;
        return;
    }

    // ===== старт прогресса (UI поток) =====
	saveRunning.store(true, std::memory_order_release);
	btnCancelSave->Enabled = true;
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

	const std::string out_dir_copy = out_dir_async;

    auto ctx = std::make_shared<SaveTaskContext>();

	ctx->text_files = std::move(text_files);
	ctx->pril_vh  = std::move(pril_map_vh);
	for (auto& archive : pril_map_ish)
		ctx->pril_ish.push_back(std::move(archive.second));

	ctx->out_dir  = out_dir_copy;
	ctx->makeDocx = chkDocx->Checked;
	if (ctx->makeDocx) {
		ctx->docxSession = std::make_shared<DocxConversionSession>(
			L"Обработка_ШТ_2022",
			ExtractFilePath(Application->ExeName) + "sht.docm");
		pushLog("DOCX: технический журнал " + GetDocxDiagnosticLogPath());
	}

	// ===== фон =====
	// Освобождение guard должно происходить в UI-потоке, а не в деструкторе
	// фоновой задачи.
	compGuard.dismiss();
	System::Threading::TTask::Run([this, ctx]()
	{
		int count_suc_files = 0;
		bool cancelled = false;
		const int totalFiles = static_cast<int>(
			ctx->text_files.size() + ctx->pril_vh.size() + ctx->pril_ish.size());
		int fileOrdinal = 0;
		try
		{
			for (const auto& f : ctx->text_files) {
				if (saveCancelRequested.load(std::memory_order_acquire)) {
					cancelled = true;
					break;
				}
				++fileOrdinal;
				pushLog("Файл " + String(fileOrdinal) + " из " + String(totalFiles) +
					(ctx->makeDocx ? ": скачивание и DOCX-конвертация \"" : ": скачивание \"") +
					String(f.outputName.c_str()) + "\"");
				const bool isLastInBatch = (fileOrdinal == totalFiles);
				count_suc_files += SaveFileVh(
					ctx->out_dir, f, ctx->makeDocx, isLastInBatch, ctx->docxSession.get());
				saveFilesDone.fetch_add(1, std::memory_order_relaxed);
				if (saveCancelRequested.load(std::memory_order_acquire)) {
					cancelled = true;
					break;
				}
			}

			if (!cancelled) {
				for (const auto& f : ctx->pril_vh) {
					if (saveCancelRequested.load(std::memory_order_acquire)) {
						cancelled = true;
						break;
					}
					++fileOrdinal;
					pushLog("Файл " + String(fileOrdinal) + " из " + String(totalFiles) +
						": скачивание приложения \"" + String(f.outputName.c_str()) + "\"");
					const bool isLastInBatch = (fileOrdinal == totalFiles);
					count_suc_files += SaveFileVh2(ctx->out_dir, f, isLastInBatch);
					saveFilesDone.fetch_add(1, std::memory_order_relaxed);
					if (saveCancelRequested.load(std::memory_order_acquire)) {
						cancelled = true;
						break;
					}
				}
			}

			if (!cancelled) {
				for (const auto& f : ctx->pril_ish) {
					if (saveCancelRequested.load(std::memory_order_acquire)) {
						cancelled = true;
						break;
					}
					++fileOrdinal;
					pushLog("Файл " + String(fileOrdinal) + " из " + String(totalFiles) +
						": скачивание архива \"" + ExtractFileName(f.remotePath) + "\"");
					const bool isLastInBatch = (fileOrdinal == totalFiles);
					count_suc_files += SaveFileIsh(ctx->out_dir, f, isLastInBatch);
					saveFilesDone.fetch_add(1, std::memory_order_relaxed);
					if (saveCancelRequested.load(std::memory_order_acquire)) {
						cancelled = true;
						break;
					}
				}
			}
		}
		catch (...)
		{
			if (!saveCancelRequested.load(std::memory_order_acquire))
				pushLog("Ошибка в процессе сохранения");
		}
		cancelled = cancelled || saveCancelRequested.load(std::memory_order_acquire);

		if (ctx->docxSession) {
			ctx->docxSession->Close();
			ctx->docxSession.reset();
		}

		if (cancelled) {
			pushLog("Сохранение отменено. Успешно сохранено файлов: " +
				String(count_suc_files) + ", обработано: " + String(fileOrdinal) +
				" из " + String(totalFiles) + ".");
		}
		else {
			pushLog(
				String(count_suc_files) + " из " +
				String((int)saveFilesTotal.load()) +
				" файлов успешно сохранены!"
			);
		}
		TThread::Queue(nullptr, [this, cancelled]() {
			saveRunning.store(false, std::memory_order_release);
			btnCancelSave->Enabled = false;
			tmSaveProgressTimer(nullptr);
			if (!cancelled)
				ggSaveProgress->Progress = ggSaveProgress->MaxValue;
			componentsEnable(true);
			saveCancelRequested.store(false, std::memory_order_release);
			gSaveProcessing.store(false, std::memory_order_release);
		});
	});


}
//---------------------------------------------------------------------------

void __fastcall TfMain::btnCancelSaveClick(TObject *Sender)
{
	if (!saveRunning.load(std::memory_order_acquire))
		return;
	if (saveCancelRequested.exchange(true, std::memory_order_acq_rel))
		return;

	btnCancelSave->Enabled = false;
	pushLog("Запрошена отмена сохранения. Текущая операция завершается безопасно...");
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
	std::string str;
	if (!serverManager_.requestText(
			primaryServerId(), "START update!", COMMAND::START_UPDATE_BASE,
			str, nullptr, &err)) {
		ShowMessage("Ошибка запроса: " + AnsiString(err.c_str()));
		return;
	}

	// дальше используем str

}
//---------------------------------------------------------------------------


void __fastcall TfMain::Button3Click(TObject *Sender)
{
	pnShablons->Visible = !pnShablons->Visible;
	ScheduleResponsiveLayout();
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

	SaveTemplatesToDisk();

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
	SaveTemplatesToDisk();

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

	SaveTemplatesToDisk();

	lstShGroups->Items->Delete(lstShGroups->ItemIndex);
	lstTemplates->Items->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::WndProc(TMessage &Message)
{
	if (Message.Msg == WM_ENTERSIZEMOVE) {
		windowMoveSizeActive_ = true;
		if (responsiveLayoutTimer_)
			responsiveLayoutTimer_->Enabled = false;
		TForm::WndProc(Message);
		return;
	}
	if (Message.Msg == WM_EXITSIZEMOVE) {
		TForm::WndProc(Message);
		windowMoveSizeActive_ = false;
		SaveMainWindowPlacement();
		ScheduleResponsiveLayout();
		return;
	}
	TForm::WndProc(Message);
}

void TfMain::CreateResponsiveScrollHost()
{
	if (responsiveScrollHost_ || !PageControl1)
		return;

	const int pageTabOrder = PageControl1->TabOrder;
	const int pageMarginLeft = PageControl1->Margins->Left;
	const int pageMarginTop = PageControl1->Margins->Top;
	const int pageMarginRight = PageControl1->Margins->Right;
	const int pageMarginBottom = PageControl1->Margins->Bottom;
	PageControl1->Align = alNone;
	PageControl1->AlignWithMargins = false;

	responsiveScrollHost_ = new TScrollBox(this);
	responsiveScrollHost_->Parent = this;
	responsiveScrollHost_->Align = alClient;
	responsiveScrollHost_->AlignWithMargins = true;
	responsiveScrollHost_->Margins->Left = pageMarginLeft;
	responsiveScrollHost_->Margins->Top = pageMarginTop;
	responsiveScrollHost_->Margins->Right = pageMarginRight;
	responsiveScrollHost_->Margins->Bottom = pageMarginBottom;
	responsiveScrollHost_->AutoScroll = true;
	responsiveScrollHost_->BorderStyle = bsNone;
	responsiveScrollHost_->HorzScrollBar->Tracking = true;
	responsiveScrollHost_->VertScrollBar->Tracking = true;
	responsiveScrollHost_->TabOrder = pageTabOrder;
	responsiveScrollHost_->TabStop = false;

	PageControl1->Parent = responsiveScrollHost_;
	PageControl1->TabOrder = 0;
	PageControl1->SetBounds(0, 0,
		std::max(1, responsiveScrollHost_->ClientWidth),
		std::max(1, responsiveScrollHost_->ClientHeight));
}

void TfMain::ScheduleResponsiveLayout()
{
	if (shuttingDown_ || updatingResponsiveLayout_ ||
		!responsiveLayoutTimer_ || !HandleAllocated())
	{
		return;
	}

	responsiveLayoutPassesRemaining_ = 2;
	if (windowMoveSizeActive_)
		return;

	responsiveLayoutTimer_->Enabled = false;
	responsiveLayoutTimer_->Enabled = true;
}

void __fastcall TfMain::ResponsiveLayoutTimerTimer(TObject *Sender)
{
	(void)Sender;
	if (!responsiveLayoutTimer_)
		return;

	responsiveLayoutTimer_->Enabled = false;
	if (shuttingDown_ || windowMoveSizeActive_ ||
		responsiveLayoutPassesRemaining_ <= 0)
	{
		return;
	}

	--responsiveLayoutPassesRemaining_;
	UpdateResponsiveLayout();
	if (!shuttingDown_ && !windowMoveSizeActive_ &&
		responsiveLayoutPassesRemaining_ > 0)
	{
		responsiveLayoutTimer_->Enabled = true;
	}
}

void TfMain::UpdateResponsiveLayout()
{
	if (updatingResponsiveLayout_ || !HandleAllocated() ||
		ComponentState.Contains(csLoading))
	{
		return;
	}
	updatingResponsiveLayout_ = true;
	DisableAlign();
	try
	{
		if (responsiveScrollHost_ && PageControl1)
		{
			const int ppi = PageControl1->CurrentPPI > 0
				? PageControl1->CurrentPPI : MAIN_FORM_DESIGN_PPI;
			const int minimumWidth = std::max(1, MulDiv(
				MAIN_CONTENT_MIN_WIDTH_AT_DESIGN_PPI, ppi,
				MAIN_FORM_DESIGN_PPI));
			const int minimumHeight = std::max(1, MulDiv(
				MAIN_CONTENT_MIN_HEIGHT_AT_DESIGN_PPI, ppi,
				MAIN_FORM_DESIGN_PPI));
			PageControl1->SetBounds(0, 0,
				std::max(minimumWidth, responsiveScrollHost_->ClientWidth),
				std::max(minimumHeight, responsiveScrollHost_->ClientHeight));
		}
		RealignControlTree(this);
		Invalidate();
	}
	__finally
	{
		EnableAlign();
		updatingResponsiveLayout_ = false;
	}
}
void TfMain::RestoreMainWindowPlacement()
{
	if (windowPlacementRestored_)
		return;

	const HWND windowHandle = Handle;
	if (!windowHandle)
		return;
	windowPlacementRestored_ = true;

	RECT normalRect = {};
	bool maximized = true;
	bool placementApplied = false;
	if (ReadMainWindowPlacement(normalRect, maximized))
	{
		WINDOWPLACEMENT placement = {};
		placement.length = sizeof(placement);
		if (GetWindowPlacement(windowHandle, &placement))
		{
			placement.flags = 0;
			placement.showCmd = SW_SHOWNORMAL;
			placement.rcNormalPosition = normalRect;
			if (SetWindowPlacement(windowHandle, &placement))
			{
				KeepNormalWindowReachable(windowHandle, CurrentPPI);
				WINDOWPLACEMENT adjustedPlacement = {};
				adjustedPlacement.length = sizeof(adjustedPlacement);
				if (GetWindowPlacement(windowHandle, &adjustedPlacement))
				{
					adjustedPlacement.flags = 0;
					adjustedPlacement.showCmd = maximized
						? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
					placementApplied = SetWindowPlacement(
						windowHandle, &adjustedPlacement) != FALSE;
				}
			}
		}
	}

	if (!placementApplied)
	{
		WINDOWPLACEMENT defaultPlacement = {};
		defaultPlacement.length = sizeof(defaultPlacement);
		if (GetWindowPlacement(windowHandle, &defaultPlacement))
		{
			defaultPlacement.flags = 0;
			defaultPlacement.showCmd = SW_SHOWMAXIMIZED;
			placementApplied = SetWindowPlacement(
				windowHandle, &defaultPlacement) != FALSE;
		}
		if (!placementApplied)
			WindowState = wsMaximized;
	}

	SaveMainWindowPlacement();
}

void TfMain::SaveMainWindowPlacement() noexcept
{
	if (!windowPlacementRestored_ || !HandleAllocated())
		return;
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	if (!GetWindowPlacement(Handle, &placement))
		return;
	const RECT& normalRect = placement.rcNormalPosition;
	const LONGLONG width = static_cast<LONGLONG>(normalRect.right)
		- normalRect.left;
	const LONGLONG height = static_cast<LONGLONG>(normalRect.bottom)
		- normalRect.top;
	if (width < 320 || height < 240 || width > 100000 || height > 100000)
		return;
	const bool maximized = placement.showCmd == SW_SHOWMAXIMIZED
		|| (placement.showCmd == SW_SHOWMINIMIZED
			&& (placement.flags & WPF_RESTORETOMAXIMIZED) != 0);
	WriteMainWindowPlacement(normalRect, maximized);
}
void __fastcall TfMain::FormResize(TObject *Sender)
{
	(void)Sender;
	ScheduleResponsiveLayout();
}
void __fastcall TfMain::FormAfterMonitorDpiChanged(TObject* Sender, int OldDPI, int NewDPI)
{
	(void)Sender;
	(void)OldDPI;
	(void)NewDPI;
	ScheduleResponsiveLayout();
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



	if (!uiIsLive())
		return;

	if (!auth_ || !dbgEh || !dbgEh->DataSource || !dbgEh->DataSource->DataSet ||
		!dbgEh->DataSource->DataSet->Active  || dbgEh->Fields[0]->AsString.IsEmpty())
		return;

	 if(saveRunning.load(std::memory_order_acquire))
	{
		return;
	}

	if (isDeletedRow(dbgEh->DataSource->DataSet)) {
		tmLoadText->Enabled = false;
		return;
	}

	std::lock_guard<std::mutex> lock(clickMutex);
	String new_num = dbgEh->Fields[0]->AsString;
	const std::string sourceServerId = currentSourceServerId();
	const String rowKey = String(sourceServerId.c_str()) + L"|" + new_num;
	if (last_loaded_num_ == rowKey)
		return;
	last_loaded_num_ = rowKey;
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
	std::string dir_str = AnsiString(dir).c_str();
	auto live = uiLive_;
	auto op = trackBackgroundOp();

System::Threading::TTask::Run([this, sourceServerId, dir_str, rowKey, live, op]() {
    std::lock_guard<std::mutex> lock(clickMutex);
	std::string str;
	std::string err;

	if (live && !live->load(std::memory_order_acquire))
		return;

	// 1) отправка
	if (!serverManager_.requestText(
			sourceServerId, dir_str, COMMAND::FILETEXT, str, nullptr, &err)) {
		TThread::Queue(nullptr, [this, live, op, err]() {
			if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
				return;
			ShowMessage("Ошибка запроса: " + AnsiString(err.c_str()));
		});
		return;
	}

	TStringList* reg = new TStringList();
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg);
	String text = reg->Text;
	delete reg;

    TThread::Queue(nullptr, [this, live, op, rowKey, text]() {
		if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
			return;
		if (last_loaded_num_ != rowKey)
            return;

        // --- Freeze: отключить перерисовку ---
        SendMessage(rchEdt->Handle, WM_SETREDRAW, FALSE, 0);


        // --- Bulk‑обновление ---
        applyRchEdtText(text, true);

        // Каретка в начало
        CHARRANGE cr{0, 0};
        SendMessage(rchEdt->Handle, EM_EXSETSEL, 0, (LPARAM)&cr);

        // Жёстко ставим скролл в самый верх
        POINT top{0, 0};
        SendMessage(rchEdt->Handle, EM_SETSCROLLPOS, 0, (LPARAM)&top);

		rchEdt->Enabled = true;

		if(dbgEh->CanFocus())
			dbgEh->SetFocus();

        tmLoadText->Enabled = true;
		SendMessage(rchEdt->Handle, WM_SETREDRAW, TRUE, 0);
		rchEdt->Invalidate();
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
	ScheduleResponsiveLayout();
}
//---------------------------------------------------------------------------


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
	ScheduleResponsiveLayout();
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
	if (!uiIsLive())
		return;

    // Переносим владение гардом в shared_ptr, чтобы RAII дожил до конца задачи
    auto guard_holder = std::make_shared<BusyGuardAtomic>(std::move(guard));
	auto live = uiLive_;
	auto op = trackBackgroundOp();
    System::Threading::TTask::Run([this, guard_holder, live, op]() {
		const std::wstring adminFile =
			auth_paths::fileBesideExecutable(L"admin984.bin");
		// Temporary simplified authorization: the marker must be a regular file
		// located beside the running EXE. The path stays Unicode throughout.
		if (!auth_ && user_ != "admin" && auth_paths::isRegularFile(adminFile)) {
			const std::string adminFileUtf8 = auth_paths::toUtf8(adminFile);
			TThread::Queue(nullptr, [this, adminFileUtf8, live, op]() {
				if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
					return;
				onSuccessfulAuthorization("admin", adminFileUtf8);
				if (tryConnect()) {
					registryOnServer("admin");
				}
				gCurrentPath = adminFileUtf8;
			});
			return; // guard_holder разрушится здесь -> флаг освободится
		}
		// Signed removable/local tokens are temporarily disabled while the
		// authorization design is being revisited. Keep this implementation for
		// a possible return; do not delete it and do not generate RSA keys.
#if 0
		auto rootOf = [](const std::string& p)->std::string { return p.substr(0,2); };
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

		std::wstring wLocal = auth_paths::fileBesideExecutable(L"token-HS.bin");
		if (SafeExistsWin(wLocal)) {
			std::string local = auth_paths::toUtf8(wLocal);
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
            TThread::Queue(nullptr, [this, fp, desired, live, op]() {
				if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
					return;
                onSuccessfulAuthorization(fp.fio, desired);
                gCurrentPath = desired;
                disconnectTransportOnly();
                if (tryConnect()) registryOnServer(fp.fio);
                else gTokens.erase(desired);
            });
		} catch (...) {
			gTokens.erase(desired);
		}
#endif
		// guard_holder выходит из области видимости -> освобождает busy-флаг
    });
}





//---------------------------------------------------------------------------
void __fastcall TfMain::updateFileLoadUI(const String& filePath, bool ok)
{
    if (!ok)
		memLogLoad->Lines->Add(filePath + " - не удалось загрузить :(");
	else
		memLogLoad->Lines->Add(filePath + " - успешно загружен :)");
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
	const String yearText = Trim(cmbGod->Text);
	bool yearOk = !yearText.IsEmpty();
	for (int i = 1; yearOk && i <= yearText.Length(); ++i) {
		const wchar_t ch = yearText[i];
		if (ch < L'0' || ch > L'9')
			yearOk = false;
	}
	if (!yearOk) {
		ShowMessage("Ошибка при сохранении базы!");
		return;
	}

	const std::wstring tokenPath = toW(token_dir_);
	const std::wstring dir = user_file_replace::ParentDirectory(tokenPath);
	const std::wstring fullFileName = dir + L"\\" + std::wstring(yearText.c_str()) + L".db";

	componentsEnable(false);

	auto live = uiLive_;
	auto op = trackBackgroundOp();
	System::Threading::TTask::Run([this, fullFileName, live, op]() {
		try {
			std::vector<BYTE> str;
			std::string err;
			if (!serverManager_.requestBinary(
					primaryServerId(), std::vector<uint8_t>{0}, COMMAND::GET_OPIS_BASE,
					str, nullptr, &err) || str.empty()) {
				TThread::Queue(nullptr, [this, live, op]() {
					(void)live;
					if (shuttingDown_)
						return;
					ShowMessage("Ошибка получения базы!");
					componentsEnable(true);
				});
				return;
			}

			std::wstring saveError;
			if (!user_file_replace::WriteReplacing(
					fullFileName, str.data(), str.size(), &saveError)) {
				TThread::Queue(nullptr, [this, live, op]() {
					(void)live;
					if (shuttingDown_)
						return;
					ShowMessage("Не удалось открыть файл для записи!");
					componentsEnable(true);
				});
				return;
			}

			TThread::Queue(nullptr, [this, fullFileName, live, op]() {
				(void)live;
				if (shuttingDown_)
					return;
				ShowMessage("База успешно сохранена - " + String(fullFileName.c_str()));
				componentsEnable(true);
			});
		} catch (...) {
			TThread::Queue(nullptr, [this, live, op]() {
				(void)live;
				if (shuttingDown_)
					return;
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

			std::vector<uint8_t> str;
			if (!serverManager_.requestBinary(
					primaryServerId(), file, COMMAND::LOAD_RAZN, str, nullptr, &err)) {
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

static String getAttachmentsRequestErrorMessage(
	const search_protocol::ServerErrorInfo& serverError,
	const std::string& fallback)
{
	using search_protocol::ErrorCode;
	if (serverError.hasCode) {
		switch (static_cast<ErrorCode>(serverError.rawCode)) {
		case ErrorCode::InvalidRequest:
			return L"Сервер отклонил запрос вложений: некорректное имя оператора.";
		case ErrorCode::OperatorNotRegistered:
			return L"Оператор не зарегистрирован на сервере для получения вложений.";
		case ErrorCode::AttachmentNotFound:
			return L"На сервере нет подготовленных вложений для этого оператора.";
		case ErrorCode::ConfigurationError:
			return L"На сервере неверно настроен каталог вложений. Обратитесь к администратору.";
		case ErrorCode::FileMetadataFailed:
		case ErrorCode::FileOpenFailed:
		case ErrorCode::FileReadFailed:
			return L"Сервер не смог прочитать подготовленные вложения. Повторите попытку позже.";
		case ErrorCode::SerializationFailed:
			return L"Сервер не смог сформировать пакет вложений.";
		default:
			break;
		}
	}

	if (!serverError.hasCode)
		return String(AnsiString(legacyServerErrorUserMessage().c_str()));
	return String(AnsiString(fallback.c_str()));
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
	std::string err;
	search_protocol::ServerErrorInfo serverError;

	auto data_request = std::vector<uint8_t>(user_.begin(), user_.end());
	std::vector<uint8_t> str;
	if (!serverManager_.requestBinary(
			primaryServerId(), data_request, COMMAND::GET_ATTACHMENTS, str,
			nullptr, &err, &serverError)) {
		ShowMessage(getAttachmentsRequestErrorMessage(serverError, err));
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
	progressForm->DefaultMonitor = dmActiveForm;
	progressForm->PopupParent = this;
	progressForm->Position = poOwnerFormCenter;
	progressForm->Font->Assign(Font);
	auto label = std::make_unique<TLabel>(progressForm.get());
	label->Parent = progressForm.get();
	label->Caption = L"Идет сохранение, пожалуйста, подождите...";
	label->Align = alClient;
	label->AlignWithMargins = true;
	label->Alignment = taCenter;
	label->Layout = tlCenter;
	label->AutoSize = false;
	label->WordWrap = true;
	label->Font->Style = TFontStyles() << fsBold;
	const HWND progressWindowHandle = progressForm->Handle;
	(void)progressWindowHandle;
	const int progressPpi = progressForm->CurrentPPI > 0
		? progressForm->CurrentPPI : MAIN_FORM_DESIGN_PPI;
	const int labelMargin = std::max(1,
		MulDiv(16, progressPpi, MAIN_FORM_DESIGN_PPI));
	label->Margins->Left = labelMargin;
	label->Margins->Top = labelMargin;
	label->Margins->Right = labelMargin;
	label->Margins->Bottom = labelMargin;
	const int desiredWidth = std::max(1,
		MulDiv(600, progressPpi, MAIN_FORM_DESIGN_PPI));
	const int desiredHeight = std::max(1,
		MulDiv(200, progressPpi, MAIN_FORM_DESIGN_PPI));
	if (progressForm->Monitor != nullptr) {
		const auto workArea = progressForm->Monitor->WorkareaRect;
		const int margin = std::max(1,
			MulDiv(16, progressPpi, MAIN_FORM_DESIGN_PPI));
		const int maxWidth = std::max(1, workArea.Width() - 2 * margin);
		const int maxHeight = std::max(1, workArea.Height() - 2 * margin);
		const int formWidth = std::min(desiredWidth, maxWidth);
		const int formHeight = std::min(desiredHeight, maxHeight);
		progressForm->SetBounds(
			workArea.Left + std::max(0, (workArea.Width() - formWidth) / 2),
			workArea.Top + std::max(0, (workArea.Height() - formHeight) / 2),
			formWidth, formHeight);
	} else {
		progressForm->SetBounds(
			std::max(0, (Screen->Width - desiredWidth) / 2),
			std::max(0, (Screen->Height - desiredHeight) / 2),
			desiredWidth, desiredHeight);
	}
	progressForm->Position = poDesigned;
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
		EqualizeSettingsColumnWidths(dbghSettings);
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
	(void)Sender;
	UpdateResponsiveLayout();
	ScheduleResponsiveLayout();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::btnDisconnectClick(TObject *Sender)
{
	disconnectUiFull();
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
	if (!uiIsLive())
		return;

    componentsEnable(false);
    ggSaveProgress->Progress = 0;
    memLogLoad->Lines->Clear();

    std::vector<String> fileList;
    for (int i = 0; i < lstFiles->Items->Count; ++i)
		fileList.push_back(lstFiles->Items->Strings[i]);

    ggSaveProgress->MaxValue = fileList.size();
	ggSaveProgress->Progress = 0;

	auto live = uiLive_;
	auto op = trackBackgroundOp();
	System::Threading::TTask::Run([this, fileList, live, op]() {
		std::vector<String> toDelete;
		auto finishUi = [this, live, op, &toDelete]() {
			const std::vector<String> files = toDelete;
			TThread::Queue(nullptr, [this, live, op, files]() {
				if (shuttingDown_)
					return;
				deleteFilesFromUI(files);
			});
		};

        for (const auto &filePath : fileList)
		{
			if (live && !live->load(std::memory_order_acquire)) {
				finishUi();
				return;
			}

			std::wstring baseFileNameWStr = ExtractFileName(filePath).w_str();  // начальное имя
			baseFileNameWStr = conv::to_wstring(user_) + L"\\" + baseFileNameWStr;  // полное имя

			std::string err;
			auto file = loadFileToVector2(AnsiString(filePath).c_str(), baseFileNameWStr, false);

			std::vector<uint8_t> str;
			bool ok = serverManager_.requestBinary(
				primaryServerId(), file, COMMAND::LOAD_TLG_TO_SEND, str, nullptr, &err);

			if (!ok) {
				const String pathCopy = filePath;
				const String errCopy = String(err.c_str());
				TThread::Queue(nullptr, [this, live, op, pathCopy, errCopy]() {
					if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
						return;
					updateFileLoadUI(pathCopy, false);
					ShowMessage("Ошибка чтения: " + errCopy);
				});
				finishUi();
				return;
			}

			toDelete.push_back(filePath);
			const String pathCopy = filePath;
			TThread::Queue(nullptr, [this, live, pathCopy]() {
				if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
					return;
				updateFileLoadUI(pathCopy, true);
			});
        }

		finishUi();
    });
}

void __fastcall TfMain::deleteFilesFromUI(const std::vector<String>& files)
{
    for (auto it = files.rbegin(); it != files.rend(); ++it) {
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
	ScheduleResponsiveLayout();
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
	const int menuPpi = dbgMenu->CurrentPPI > 0
		? dbgMenu->CurrentPPI : MAIN_FORM_DESIGN_PPI;
	if (rowHeight <= 0)
		rowHeight = -dbgMenu->Font->Height + std::max(1,
			MulDiv(6, menuPpi, MAIN_FORM_DESIGN_PPI));

	int titleHeight = dbgMenu->TitleHeight;

	// +1 строка сверху, чтобы последняя помещалась, и небольшой запас
	int newHeight = std::max(1,
		MulDiv(15, menuPpi, MAIN_FORM_DESIGN_PPI))
		+ titleHeight + (rowCount + 1) * rowHeight;

	dbgEh->RowDetailPanel->Height   = newHeight;
	dbgMenu->RowDetailPanel->Height = newHeight;
	dbgMenu->DataSource->DataSet->First();
	ScheduleResponsiveLayout();
}


//---------------------------------------------------------------------------


void __fastcall TfMain::tmLoginOutTimer(TObject *Sender)
{
	// не запускаем второй выход, пока предыдущий не завершён
	BusyGuardAtomic guard(gLogoutBusy);
	if (!guard) return;

	if (user_ == "admin")
		return;

	// если токен (каталог/файл) «есть» — выходим (ничего не делаем)
	// заменили std::filesystem::exists(token_dir_) на безопасную проверку
	if (SafeExistsWin(toW(token_dir_)))
		return;

	if (!uiIsLive())
		return;

	auto live = uiLive_;
	auto op = trackBackgroundOp();
	// Очищаем данные в фоне (если вдруг очистка attachments может быть долгой)
	System::Threading::TTask::Run([this, live, op]() {
		message_.attachments.clear();
		user_.clear();

		// Возврат в главный поток для UI и остальных операций
		TThread::Queue(nullptr, [this, live, op]() {
			if (!live || !live->load(std::memory_order_acquire) || shuttingDown_)
				return;
			clear();
			//tmLoginOut->Enabled = false;
			tmLoginIn->Enabled = true;
			pnShablons->Visible = false;
			lblUser->Caption = "Нет авторизации!";
			lblUser->Font->Color = clMaroon;
			auth_ = false;
			disconnectUiFull();
			componentsEnable(false);
			ScheduleResponsiveLayout();
		});
	});
}


bool TfMain::tryConnect()
{
    try {
		// Edit2 remains the compatibility editor for the primary host.
		qrySettings->Close();
		qrySettings->SQL->Text =
			L"UPDATE servers SET host = :host WHERE server_id = 'default' "
			L"AND (SELECT COUNT(*) FROM servers WHERE enabled <> 0) = 1";
		qrySettings->ParamByName(L"host")->AsString = Edit2->Text;
		qrySettings->ExecSQL();
		String configError;
		if (!configureServersForCurrentYear(&configError)) {
			ShowMessage(configError);
			lbStatus->Caption = "Статус: ошибка конфигурации";
			componentsEnable(false);
			return false;
		}
		std::string err;
		if (!serverManager_.connectPrimary(&err)) {
			ShowMessage("Ошибка подключения: " + String(err.c_str()));
			lbStatus->Caption = "Статус: нет подключения";
			componentsEnable(false);
			return false;
		}
		const int serverCount = static_cast<int>(serverManager_.serverIds().size());
		lbStatus->Caption = "Статус: настроено серверов " + String(serverCount) +
			", основной подключён, " + cmbGod->Text + " год";
        componentsEnable(true);
        return true;
    }
    catch (const Exception& e) {
        ShowMessage("Ошибка подключения: " + e.Message);
        lbStatus->Caption = "Статус: ошибка подключения";
        componentsEnable(false);
        return false;
    }
    catch (...) {
        ShowMessage("Неизвестная ошибка при подключении");
        lbStatus->Caption = "Статус: ошибка подключения";
        componentsEnable(false);
        return false;
    }
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
		ScheduleResponsiveLayout();
}
//---------------------------------------------------------------------------
void __fastcall TfMain::dbgMenuRowDetailPanelHide(TCustomDBGridEh *Sender, bool &CanHide)
{
    dbgEh->RowDetailPanel->Height -= dbgMenu->RowDetailPanel->Height;
	ScheduleResponsiveLayout();
}
//---------------------------------------------------------------------------

void __fastcall TfMain::excel1Click(TObject *Sender)
{
    if (!frxReport->PrepareReport())
        return;

    TfrxXLSExport *xls = frxXLSExport1;

    TSaveDialog *dlg = new TSaveDialog(this);
    try
    {
        dlg->Filter = "Excel (*.xlsx)|*.xlsx";
        dlg->DefaultExt = "xlsx";

        if (dlg->Execute())
        {
			const String previousFileName = xls->FileName;
			const bool previousShowDialog = xls->ShowDialog;
			const bool previousOpenAfterExport = xls->OpenAfterExport;
			const bool previousOverwritePrompt = xls->OverwritePrompt;
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
				xls->FileName = previousFileName;
				xls->ShowDialog = previousShowDialog;
				xls->OpenAfterExport = previousOpenAfterExport;
				xls->OverwritePrompt = previousOverwritePrompt;
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


