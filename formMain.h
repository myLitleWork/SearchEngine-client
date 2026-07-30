//---------------------------------------------------------------------------

#ifndef formMainH
#define formMainH

#include "WordBatchProcessor.h"
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
#include <set>
#include <list>
#include <mutex>
#include <my_json/Resolution.h>
#include <my_json/Telega.h>
#include <myRequest.h>
#include "SocketFileSaver.h"
#include "RecentComboHistory.h"

#include "FrameWay.h"
#include "FrameAttachments.h"
#include "FramePdtv.h"

#include "Message.h"
#include "myAsioClient.h"
#include "utils.h"


#include <DBGridEhImpExp.hpp>

using boost::asio::ip::tcp;
class FlashPodpis;

//---------------------------------------------------------------------------
class TfMain : public TForm
{
__published:	// IDE-managed Components
	TFileSaveDialog *FileSaveDialog1;
	TFileOpenDialog *FileOpenDialog1;
	TPanel *Panel1;
	TEdit *Edit2;
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
	TButton *Button2;
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
	void __fastcall Button6Click(TObject *Sender);
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
	void __fastcall chkSQLClick(TObject *Sender);
	void __fastcall btnUpdateClick(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall lstShGroupsClick(TObject *Sender);
	void __fastcall lstTemplatesClick(TObject *Sender);
	void __fastcall N2Click(TObject *Sender);
	void __fastcall N3Click(TObject *Sender);
	void __fastcall FormMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y);
	void __fastcall FormMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
	void __fastcall FormMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y);
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





private:	// User declarations

	enum class ROW_MENU
	{
		WAY,
		PDTV,
		ATTACHMENTS
	};
	

	void __fastcall GridColumnMenuClick(TObject *Sender);
	void FillPopUpMenuWithGridColumns(TPopupMenu *popDbg, TDBGridEh *grid);
	void __fastcall updateFileLoadUI();
	void __fastcall deleteFilesFromUI();

    friend class ComponentsGuard;

	std::map<String,String> getMapFromQry(TFDQuery* qry, const String& table_name);

	std::map<std::string,std::map<std::string, Request>> mTemplates{};

	std::map<String, int> mapFieldInd;

	int offsetX, offsetY;
	bool useF12Control();
	void __fastcall finishSearch();


	bool searchSuccess = false;
	std::string searchError;
	std::vector<Telega> telegi_async;
	TObject* lastSender = nullptr;

    int saveResultCount = 0;
    int saveTotalCount = 0;
	TStringList* saveLog = nullptr;

	std::vector<String> filesToDeleteUI;

	UnicodeString uiFilePath;
	bool uiOk;




	TMemTableEh *mtMenu = nullptr;
	TDataSource *dsMenu = nullptr;





	void InitDbgMenuDS();








public:		// User declarations


	MyAsioClient client_;

	TfrAttachments *myFrAttachments;
	TfrPdtv *myFrPdtv;
	TfrWay *myFrWay;


	// ????????
	std::atomic_bool     saveRunning{false};
	std::atomic_uint32_t saveFilesDone{0};
	std::atomic_uint32_t saveFilesTotal{0};


    struct SaveTaskContext
	{
		std::map<std::string, std::string> pril_vh;
		std::map<std::string, std::string> text_map;
		std::map<String, std::vector<String>> pril_ish;
		std::string out_dir;
		bool makeDocx = false;
	};



	void pushLog(const String& s);

	// ??????? ???????????????? ??????? ?????
	std::mutex log_mtx;
	std::vector<String> pendingLog;   // ??????, ??????? ??? ??????????, GUI ????????


	void __fastcall componentsEnable(bool b);

	Resolution r;
	std::map<String,double> myMap;

	std::string requests;
	void clear();
	std::map<String,std::map<String,double>> mapAnswers;
	std::vector<Telega> telegi;
	std::list<std::pair<std::string, int>> telegs;
	void convTelegs(std::vector<Telega>& t);
	void setColor(String sS);
	std::map<String, String> mapPorts;
	const String f12_dir = "D:\\F12\\";
	bool saveToFile();
	void loadTextToEditFromServer();
	std::vector<String> SelectFiles();

	int SaveFileVh(const std::string& out_dir, const std::pair<std::string, std::string>& f, bool makeDocx = false,
		bool flushStreamToOs = false);
	int SaveFileIsh(const std::string& out_dir, const std::pair<String, std::vector<String>>& f,
		bool flushStreamToOs = false);
    int SaveFileVh2(const std::string& out_dir, const std::pair<std::string, std::string>& f,
		bool flushStreamToOs = false);

	bool save_pocessing = false;

	void disConnect();
	void disconnectTransportOnly();
	void disconnectUiFull();
	bool reconnectTransport(bool reregister = false);
	bool tryConnect();

	std::mutex clickMutex;
	std::mutex soketMutex;

	std::map<std::string, std::set<std::string>> grp;
	std::map<std::string, std::string> name_gr;
	std::map<std::string, std::string> name_solo;

	RecentComboHistory ComboHistory{nullptr,0};


	std::vector<uint8_t> loadFileToVector2(const std::string& filePath, const std::wstring& file_name);
	void __fastcall onSuccessfulAuthorization(const std::string& user_name, const std::string& filePath);

	void makeErrorWay(bool error);
	void registryOnServer(const std::string& user_name);


	bool checkUser();

	 Message message_;
	 std::string user_;
	 bool auth_ = false;
	 std::string token_dir_;
	 String last_loaded_num_;

     std::mutex logMtx;
	 std::deque<String> logQueue;

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
