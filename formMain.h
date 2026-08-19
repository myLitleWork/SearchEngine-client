//---------------------------------------------------------------------------

#ifndef formMainH
#define formMainH

#include "FlashAuthSession.h"
#include "DeviceAuthSelect.h"
#include "WordBatchProcessor.h"
#include "DataDriverEh.hpp"
#include "DBAxisGridsEh.hpp"
#include "DBGridEh.hpp"
#include "DBGridEhGrouping.hpp"
#include "DBGridEhToolCtrls.hpp"
#include "DynVarsEh.hpp"
#include "EhLibVCL.hpp"
#include "frxClass.hpp"
#include "frxDBSet.hpp"
#include "frxExportBaseDialog.hpp"
#include "frxExportXLS.hpp"
#include "GridsEh.hpp"
#include "MemTableDataEh.hpp"
#include "MemTableEh.hpp"
#include "PrnDbgEh.hpp"
#include "ToolCtrlsEh.hpp"
#include <Data.DB.hpp>
#include <Datasnap.DBClient.hpp>
#include <Datasnap.Provider.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Comp.DataSet.hpp>
#include <FireDAC.DApt.hpp>
#include <FireDAC.DApt.Intf.hpp>
#include <FireDAC.DatS.hpp>
#include <FireDAC.Phys.hpp>
#include <FireDAC.Phys.Intf.hpp>
#include <FireDAC.Phys.SQLite.hpp>
#include <FireDAC.Phys.SQLiteDef.hpp>
#include <FireDAC.Phys.SQLiteWrapper.Stat.hpp>
#include <FireDAC.Stan.Async.hpp>
#include <FireDAC.Stan.Def.hpp>
#include <FireDAC.Stan.Error.hpp>
#include <FireDAC.Stan.ExprFuncs.hpp>
#include <FireDAC.Stan.Intf.hpp>
#include <FireDAC.Stan.Option.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <FireDAC.Stan.Pool.hpp>
#include <FireDAC.UI.Intf.hpp>
#include <FireDAC.VCLUI.Wait.hpp>
#include <System.Actions.hpp>
#include <System.Classes.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Samples.Gauges.hpp>
#include <Vcl.StdCtrls.hpp>
#include <memory>
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Generics.Collections.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.Samples.Gauges.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Data.DB.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Comp.DataSet.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <Vcl.DBGrids.hpp>
#include <Vcl.Grids.hpp>
#include "DataDriverEh.hpp"
#include "DBAxisGridsEh.hpp"
#include "DBGridEh.hpp"
#include "DBGridEhGrouping.hpp"
#include "DBGridEhToolCtrls.hpp"
#include "DynVarsEh.hpp"
#include "EhLibVCL.hpp"
#include "GridsEh.hpp"
#include "MemTableDataEh.hpp"
#include "MemTableEh.hpp"
#include "ToolCtrlsEh.hpp"
#include "EhLibMTE.hpp"
#include <Vcl.Menus.hpp>
#include "PrnDbgeh.hpp"
#include "frxClass.hpp"
#include "frxDBSet.hpp"
#include "frxPreview.hpp"
#include <Vcl.WinXPickers.hpp>
#include <Datasnap.DBClient.hpp>
#include <Datasnap.Provider.hpp>
#include <System.Actions.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.CheckLst.hpp>
#include "frxExportBaseDialog.hpp"
#include "frxExportXLS.hpp"
#include <FireDAC.DApt.hpp>
#include <FireDAC.DApt.Intf.hpp>
#include <FireDAC.DatS.hpp>
#include <FireDAC.Phys.hpp>
#include <FireDAC.Phys.Intf.hpp>
#include <FireDAC.Phys.SQLite.hpp>
#include <FireDAC.Phys.SQLiteDef.hpp>
#include <FireDAC.Phys.SQLiteWrapper.Stat.hpp>
#include <FireDAC.Stan.Async.hpp>
#include <FireDAC.Stan.Def.hpp>
#include <FireDAC.Stan.Error.hpp>
#include <FireDAC.Stan.ExprFuncs.hpp>
#include <FireDAC.Stan.Intf.hpp>
#include <FireDAC.Stan.Option.hpp>
#include <FireDAC.Stan.Pool.hpp>
#include <FireDAC.UI.Intf.hpp>
#include <FireDAC.VCLUI.Wait.hpp>
#include <boost/asio.hpp>
#include <cstring>
#include <iostream>
#include <map>
#include <tuple>
#include <utility>
#include <list>
#include <mutex>
#include <atomic>
#include <memory>
#include <my_json/Telega.h>
#include <myRequest.h>
#include "SocketFileSaver.h"
#include "RecentComboHistory.h"

#include "FrameWay.h"
#include "FrameAttachments.h"
#include "FramePdtv.h"

#include "Message.h"
#include "myAsioClient.h"
#include "MultiServerManager.h"
#include "TelegramAttachmentClient.h"
#include "utils.h"


#include <DBGridEhImpExp.hpp>

using boost::asio::ip::tcp;
class FlashPodpis;

struct DbgEhColumnLayoutItem
{
	String fieldName;
	int widthAtDesignPpi = 0;
	bool visible = true;
	bool autoFitColWidth = true;
};

//---------------------------------------------------------------------------
class TfMain : public TForm
{
__published:	// IDE-managed Components
	TFileSaveDialog *FileSaveDialog1;
	TFileOpenDialog *FileOpenDialog1;
	TPanel *Panel1;
	TEdit *Edit2;
	TLabel *lblServerEndpoints;
	TButton *btnConnect;
	TDataSource *srcSearch;
	TFDQuery *qryTelegi;
	TMemTableEh *memTbl;
	TIntegerField *qryTeleginum;
	TDateField *qryTelegidate;
	TDateField *qryTelegidate_podp;
	TWideStringField *qryTelegiisp;
	TWideStringField *qryTelegitype;
	TWideStringField *qryTelegipodp_num;
	TWideStringField *qryTelegifrom_to;
	TFloatField *qryTelegirel;
	TWideStringField *qryTelegidir;
	TIntegerField *qryTelegitel_num;
	TFDConnection *ConnectionRes;
	TFDConnection *ConnectionSettings;
	TPopupMenu *popDbg;
	TPrintDBGridEh *dbp;
	TPanel *pnStatusBar;
	TLabel *lbStatus;
	TComboBox *cmbGod;
	TFDQuery *qrySettings;
	TIntegerField *memTblnum;
	TDateField *memTbldate;
	TDateField *memTbldate_podp;
	TWideStringField *memTblisp;
	TWideStringField *memTbltype;
	TWideStringField *memTblpodp_num;
	TWideStringField *memTblfrom_to;
	TFloatField *memTblrel;
	TWideStringField *memTbldir;
	TIntegerField *memTbltel_num;
	TWideStringField *qryTelegikr;
	TWideStringField *memTblkr;
	TWideStringField *qryTelegipril_name;
	TWideStringField *qryTelegipril_count;
	TWideStringField *memTblpril_name;
	TWideStringField *memTblpril_count;
	TButton *btnUpdate;
	TPopupMenu *popMenu;
	TMenuItem *N2;
	TPopupMenu *PopupMenu2;
	TMenuItem *N3;
	TGauge *ggSaveProgress;
	TfrxReport *frxReport;
	TfrxDBDataset *frxDBDataset1;
	TActionList *myActiomList;
	TDataSetDriverEh *drvEh;
	TAction *actDoSearch;
	TAction *actSetVh;
	TTimer *tmLoadText;
	TAction *actSetIsh;
	TAction *actSetField;
	TAction *actSave;
	TAction *actSetPril;
	TAction *actSetText;
	TPopupMenu *popEdt;
	TMenuItem *N6;
	TAction *actShowRowPanel;
	TTimer *tmFilterEnableAlarm;
	TfrxReport *frxReport1;
	TTimer *tmLoginIn;
	TTimer *tmLoginOut;
	TLabel *lblUser;
	TPopupMenu *popListFilesLoad;
	TMenuItem *popItemClearFilesOnLoad;
	TMenuItem *delFileLoad;
	TDataSource *srcSettings;
	TPageControl *PageControl1;
	TTabSheet *TabSheet1;
	TLabel *Label1;
	TSplitter *Splitter1;
	TPanel *Panel7;
	TListBox *ListBox1;
	TDBGridEh *dbgEh;
	TPanel *searchOptPanel;
	TLabel *lbFilter;
	TGroupBox *grpSaveOptions;
	TCheckBox *chkPril;
	TCheckBox *chkText;
	TGroupBox *GroupBox3;
	TEdit *edtSaveDir;
	TButton *btnSave;
	TButton *btnCancelSave;
	TPanel *Panel8;
	TPanel *Panel9;
	TRadioGroup *rgTypeT;
	TGroupBox *grpSqlParam;
	TGroupBox *grpSql;
	TCheckBox *chkExc;
	TCheckBox *chkSQL;
	TGroupBox *GroupBox1;
	TButton *Button1;
	TButton *Button3;
	TPanel *pnShablons;
	TLabel *Label5;
	TLabel *Label6;
	TListBox *lstShGroups;
	TListBox *lstTemplates;
	TPanel *Panel11;
	TButton *btnSearch;
	TCheckBox *chkOEM;
	TCheckBox *chkUpper;
	TComboBox *cmbReq;
	TRichEdit *rchEdt;
	TTabSheet *tbLoad;
	TPanel *Panel10;
	TLabel *Label7;
	TButton *btnLoadOnServerFiles;
	TMemo *memLogLoad;
	TListBox *lstFiles;
	TButton *btnAddFilesLoad;
	TPanel *pnlRaznoska;
	TButton *btnSaveOpisBase;
	TButton *btnLoadRazn;
	TButton *btnAttachSave;
	TTabSheet *tbSettings;
	TTabControl *tbcSettings;
	TDBGridEh *dbghSettings;
	TDataSetProvider *DataSetProvider1;
	TClientDataSet *ClientDataSet1;
	TGridPanel *grdParam;
	TLabel *Label2;
	TLabel *Label3;
	TComboBox *cmbMaxReq;
	TComboBox *cmbField;
	TCheckBox *chkDocx;
	TButton *btnDisconnect;
	TWideStringField *qryTelegilast_mesto;
	TWideStringField *qryTelegigde_sht;
	TWideStringField *qryTelegiblank;
	TWideStringField *memTbllast_mesto;
	TWideStringField *memTblgde_sht;
	TWideStringField *memTblblank;
	TIntegerField *qryTelegideleted;
	TIntegerField *memTbldeleted;
	TWideStringField *qryTelegisource_server_id;
	TWideStringField *memTblsource_server_id;
	TButton *Button2;
	TButton *btnEditColumns;
	TEdit *edtLatency;
	TDBGridEh *dbgMenu;
	TPopupMenu *popExportButton;
	TMenuItem *excel1;
	TfrxXLSExport *frxXLSExport1;
	TPopupMenu *popSaveButton;
	TMenuItem *chkMakeDir;
	TTimer *tmSaveProgress;

	void __fastcall btnSearchClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall btnConnectClick(TObject *Sender);
	void __fastcall cmbReqChange(TObject *Sender);
	void __fastcall cmbReqKeyUp(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall tmFilterEnableAlarmTimer(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall dbgEhColumns0CellButtons0Click(TObject *Sender, bool &Handled);
	void __fastcall PageControl1Change(TObject *Sender);
	void __fastcall N1Click(TObject *Sender);
	void __fastcall dbgEhColumns6CellButtons0Click(TObject *Sender, bool &Handled);
	void __fastcall dbgEhCellClick(TColumnEh *Column);
	void __fastcall dbgEhDrawColumnCell(TObject *Sender, const TRect &Rect, int DataCol,
		TColumnEh *Column, Gridseh::TGridDrawState State);
	void __fastcall cmbGodChange(TObject *Sender);
	void __fastcall btnSaveClick(TObject *Sender);
	void __fastcall btnCancelSaveClick(TObject *Sender);
	void __fastcall chkSQLClick(TObject *Sender);
	void __fastcall btnUpdateClick(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall lstShGroupsClick(TObject *Sender);
	void __fastcall lstTemplatesClick(TObject *Sender);
	void __fastcall N2Click(TObject *Sender);
	void __fastcall N3Click(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall FormAfterMonitorDpiChanged(TObject* Sender, int OldDPI, int NewDPI);
	void __fastcall lstTemplatesDblClick(TObject *Sender);
	void __fastcall edtSaveDirDblClick(TObject *Sender);
	void __fastcall N5Click(TObject *Sender);
	void __fastcall dbgEhKeyPress(TObject *Sender, System::WideChar &Key);
	void __fastcall srcSearchDataChange(TObject *Sender, TField *Field);
	void __fastcall actDoSearchExecute(TObject *Sender);
	void __fastcall tmLoadTextTimer(TObject *Sender);
	void __fastcall actSetVhExecute(TObject *Sender);
	void __fastcall actSetIshExecute(TObject *Sender);
	void __fastcall actSetFieldExecute(TObject *Sender);
	void __fastcall actSaveExecute(TObject *Sender);
	void __fastcall actSetPrilExecute(TObject *Sender);
	void __fastcall actSetTextExecute(TObject *Sender);
	void __fastcall N6Click(TObject *Sender);
	void __fastcall actShowRowPanelExecute(TObject *Sender);
	void __fastcall tmLoginInTimer(TObject *Sender);
	void __fastcall tmLoginOutTimer(TObject *Sender);
	void __fastcall btnAddFilesLoadClick(TObject *Sender);
	void __fastcall popItemClearFilesOnLoadClick(TObject *Sender);
	void __fastcall delFileLoadClick(TObject *Sender);
	void __fastcall tbLoadShow(TObject *Sender);
	void __fastcall btnSaveOpisBaseClick(TObject *Sender);
	void __fastcall btnLoadRaznClick(TObject *Sender);
	void __fastcall btnAttachSaveClick(TObject *Sender);
	void __fastcall tbcSettingsChange(TObject *Sender);
	void __fastcall chkDocxClick(TObject *Sender);
	void __fastcall chkTextClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall btnDisconnectClick(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall btnLoadOnServerFilesClick(TObject *Sender);
	void __fastcall rchEdtClick(TObject *Sender);
	void __fastcall dbgEhRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow);
	void __fastcall dbgMenuRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow);
	void __fastcall dbgMenuRowDetailPanelHide(TCustomDBGridEh *Sender, bool &CanHide);
	void __fastcall excel1Click(TObject *Sender);
	void __fastcall tmSaveProgressTimer(TObject *Sender);
	void __fastcall popDbgPopup(TObject *Sender);
	void __fastcall dbgEhColWidthsChanged(TObject *Sender);
	void __fastcall btnEditColumnsClick(TObject *Sender);





private:	// User declarations

	enum class ROW_MENU
	{
		WAY,
		PDTV,
		ATTACHMENTS
	};

	void __fastcall GridColumnMenuClick(TObject *Sender);
	void __fastcall SaveDbgEhColumnLayoutClick(TObject *Sender);
	void __fastcall ResetDbgEhColumnWidthsClick(TObject *Sender);
	void FillPopUpMenuWithGridColumns(TPopupMenu *popDbg, TDBGridEh *grid);
	void CaptureDbgEhDefaultColumnLayout();
	void ApplyDbgEhColumnLayout(
		const std::vector<DbgEhColumnLayoutItem>& items,
		bool autoFit,
		bool applyVisibility);
	void CollectDbgEhColumnLayout(
		std::vector<DbgEhColumnLayoutItem>& items,
		bool& autoFit) const;
	void SaveDbgEhColumnLayout() noexcept;
	void RestoreDbgEhColumnLayout();
	void ResetDbgEhColumnWidths();
	void __fastcall updateFileLoadUI(const String& filePath, bool ok);
	void __fastcall deleteFilesFromUI(const std::vector<String>& files);
	bool uiIsLive() const;
	std::shared_ptr<void> trackBackgroundOp();

    friend class ComponentsGuard;

	std::map<String,String> getMapFromQry(TFDQuery* qry, const String& table_name);

	std::map<std::string,std::map<std::string, Request>> mTemplates{};
	std::map<String, int> mapFieldInd;
	bool useF12Control();
	void __fastcall finishSearch();
	void __fastcall WndProc(TMessage &Message);
	void __fastcall ResponsiveLayoutTimerTimer(TObject *Sender);
	void CreateResponsiveScrollHost();
	void ScheduleResponsiveLayout();
	void UpdateResponsiveLayout();
	void RestoreMainWindowPlacement();
	void SaveMainWindowPlacement() noexcept;
	std::vector<DbgEhColumnLayoutItem> dbgEhDefaultColumns_;
	bool dbgEhDefaultAutoFit_ = true;
	bool dbgEhLayoutBusy_ = false;
	bool dbgEhLayoutReady_ = false;
	TTimer* responsiveLayoutTimer_ = nullptr;
	TScrollBox* responsiveScrollHost_ = nullptr;
	int responsiveLayoutPassesRemaining_ = 0;
	bool updatingResponsiveLayout_ = false;
	bool windowMoveSizeActive_ = false;
	bool shuttingDown_ = false;
	std::shared_ptr<std::atomic_bool> uiLive_ =
		std::make_shared<std::atomic_bool>(true);
	std::atomic<int> uiInflight_{0};
	bool windowPlacementRestored_ = false;
	bool searchSuccess = false;
	std::string searchError;
	std::vector<Telega> telegi_async;
	struct ServerSearchFailure
	{
		std::string serverId;
		std::string displayName;
		std::string error;
	};
	std::vector<ServerSearchFailure> searchFailures_async;
	int searchServerSuccessCount = 0;
	int searchServerTotalCount = 0;
	TObject* lastSender = nullptr;
	void ensureServerCatalogSeeded(const String& fallbackHost);
	void reloadAvailableYears();
	void updateServerEndpointsLabel();
	bool configureServersForCurrentYear(String* errorMessage = nullptr);
	std::string currentSourceServerId() const;
	std::string primaryServerId() const;

    int saveResultCount = 0;
    int saveTotalCount = 0;
	TStringList* saveLog = nullptr;

	TMemTableEh *mtMenu = nullptr;
	TDataSource *dsMenu = nullptr;





	void InitDbgMenuDS();








public:		// User declarations


	MultiServerManager serverManager_;

	TfrAttachments *myFrAttachments;
	TfrPdtv *myFrPdtv;
	TfrWay *myFrWay;


	// ????????
	std::atomic_bool     saveRunning{false};
	std::atomic_bool     saveCancelRequested{false};
	std::atomic_uint32_t saveFilesDone{0};
	std::atomic_uint32_t saveFilesTotal{0};


	struct TelegramTextRequest
	{
		std::string serverId;
		std::string outputName;
		int telegramId{0};
		int telegramType{0};
	};
	struct TelegramAttachmentRequest
	{
		std::string serverId;
		int telegramId{0};
		int telegramType{0};
		std::string fileName;
		String outputName;
	};
	struct TelegramAttachmentArchiveRequest
	{
		std::string serverId;
		int telegramId{0};
		int telegramType{1};
		std::vector<String> filesToExtract;
	};
	struct AttachmentListCacheEntry
	{
		bool attempted{false};
		bool ok{false};
		std::vector<TelegramAttachmentInfo> items;
		std::string error;
	};
	struct SaveTaskContext
	{
		std::vector<TelegramAttachmentRequest> pril_vh;
		std::vector<TelegramTextRequest> text_files;
		std::vector<TelegramAttachmentArchiveRequest> pril_ish;
		std::map<TelegramAttachmentListKey, AttachmentListCacheEntry> attachmentLists;
		std::string out_dir;
		bool makeDocx = false;
		std::shared_ptr<DocxConversionSession> docxSession;
	};



	void pushLog(const String& s);

	// ??????? ???????????????? ??????? ?????
	std::mutex log_mtx;
	std::vector<String> pendingLog;   // ??????, ??????? ??? ??????????, GUI ????????


	void __fastcall componentsEnable(bool b);

	std::map<String,double> myMap;

	std::string requests;
	void clear();
	std::map<String,std::map<String,double>> mapAnswers;
	std::vector<Telega> telegi;
	std::list<std::pair<std::string, int>> telegs;
	void convTelegs(std::vector<Telega>& t);
	void applyRchEdtText(const String& telegramText, bool highlightQuery);
	void setColor(String sS);
	String searchSaveReport_;
	int searchSaveAttempted_ = 0;
	int searchSaveInserted_ = 0;
	int searchSaveFailed_ = 0;
	bool searchSavePartial_ = false;
	std::map<String, String> mapPorts;
	const String f12_dir = "D:\\F12\\";
	bool saveToFile();
	void loadTextToEditFromServer();
	std::vector<String> SelectFiles();

	int SaveFileVh(const std::string& out_dir, const TelegramTextRequest& f, bool makeDocx = false,
		bool flushStreamToOs = false, DocxConversionSession* docxSession = nullptr);
	int SaveFileIsh(const std::string& out_dir, const TelegramAttachmentArchiveRequest& f,
		bool flushStreamToOs = false);
	int SaveFileVh2(const std::string& out_dir, const TelegramAttachmentRequest& f,
		bool flushStreamToOs, SaveTaskContext& ctx);

	bool save_pocessing = false;

	void disConnect();
	void disconnectTransportOnly();
	void disconnectUiFull();
	bool reconnectTransport(const std::string& serverId = std::string{});
	bool tryConnect();
	bool hasServerAuthorization() const;
	void applyServerAuthUiStatus();

	std::mutex clickMutex;
	std::mutex soketMutex;

	RecentComboHistory ComboHistory{nullptr,0};


	std::vector<uint8_t> loadFileToVector2(const std::string& filePath, const std::wstring& file_name,
		bool showErrors = true);
	void __fastcall onSuccessfulAuthorization(const std::string& user_name, const std::string& filePath);
	void applyLocalAuthSources();
	void queueApplyLocalAuthSources();
	void logoutUnauthorizedUi();
	std::optional<DeviceAuthIdentity> resolveLiveDeviceIdentity();
	DeviceAuthIdentity currentDeviceIdentity() const;
	void applyResolvedDeviceIdentity(const DeviceAuthIdentity& identity);
	bool isDeviceAuthSource() const;

	void makeErrorWay(bool error);
	void registryOnServer(const std::string& user_name);


	 Message message_;
	 std::string user_;
	 bool auth_ = false; // local flash/admin UI only
	 std::string token_dir_;
	 std::string client_id_;
	 std::string device_type_;
	 std::string device_id_;
	 std::string auth_signature_;
	 enum class LocalAuthSource { None, Usb, Computer, Admin };
	 LocalAuthSource auth_source_ = LocalAuthSource::None;
	 std::unique_ptr<FlashAuthSession> flashAuth_;
	 String last_loaded_num_;

	 std::atomic_bool gLogInBusy{false};
	 std::atomic_bool gLogoutBusy{false};
	 std::atomic_bool gComponentsBusy{false};
	 std::atomic_bool gSearchProcessing{false};
	 std::atomic_bool gSaveProcessing{false};

	 std::mutex connectionParamsMtx_;
	 std::string lastHost_;
	 std::string lastPort_;



	 static inline std::unordered_map<std::string,uint64_t> gTokens = {};
	 static inline std::string  gCurrentPath = {};
	 static inline uint64_t     gTick = 0;


	__fastcall TfMain(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfMain *fMain;
//---------------------------------------------------------------------------
#endif
