//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "FramePdtv.h"
#include "utils.h"
#include "utils.h"
#include "myAsioClient.h"
#include "myJson.h"
#include <System.Threading.hpp>
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
#pragma resource "*.dfm"

//TfrPdtv *frPdtv;

//---------------------------------------------------------------------------
__fastcall TfrPdtv::TfrPdtv(TComponent* _Owner, MultiServerManager& manager, TDBGridEh *_dbgMain, TDBGridEh *_dbgMenu)
  : TFrameDbg(_Owner, manager, _dbgMain, _dbgMenu)
{



}
//---------------------------------------------------------------------------

void TfrPdtv::InitDbgDS()
{
	if (mt) return;

	mt = new TMemTableEh(this);
	mt->FieldDefs->Clear();
	mt->FieldDefs->Add(L"kuda",       ftWideString, 30, false);
	mt->FieldDefs->Add(L"file",       ftWideString, 30, false);
	mt->FieldDefs->Add(L"ddate_time", ftWideString, 30, false);
	mt->FieldDefs->Add(L"dir",        ftWideString, 30, false);
	mt->CreateDataSet();

	ds = new TDataSource(this);
	ds->DataSet = mt;

	dbgPdtv->DataSource = ds;
}
//---------------------------------------------------------------------------

void TfrPdtv::FillDbg()
{

        // ������ ��� ����������� ������ ������ �� ����� �������
		auto safeShow = [](const String& msg)
		{
			TThread::Queue(nullptr, [m = String(msg)]() { ShowMessage(m); });
		};

		dbgPdtv->RowDetailPanel->Active = false;

		mt->EmptyTable();

		auto cmd = COMMAND::GET_ISH_PDTV;
		const std::string serverId = SourceServerId();

		String numberS, to;

		numberS = dbgMain->Fields[0]->AsString; // �����
		to      = dbgMain->Fields[8]->AsString; // ����������

		// ������ ������ ������ � GUI-������
		TCursor prev = Screen->Cursor;
		Screen->Cursor = crHourGlass;

		System::Threading::TTask::Run([this, numberS, to, cmd, prev, safeShow, serverId]()
		{
			std::string number;
			std::string jsonStr;
			bool ok = true;
			std::list<std::map<std::string, std::string>> result;

			try
			{
				number = AnsiString(numberS).c_str();

				std::string err;
				if (!serverManager.requestText(
						serverId, number, cmd, jsonStr, nullptr, &err)) {
					ok = false;
					safeShow(L"Ошибка запроса: " + AnsiString(err.c_str()));
				}



				if (ok)
				{
					try {
						result = JSON::get<std::list<std::map<std::string, std::string>>>(jsonStr);
					} catch (const std::exception& e) {
						ok = false;
						safeShow(L"������ �������� JSON (std): " + String(e.what()));
					} catch (const Exception& e) {
						ok = false;
						safeShow(L"������ �������� JSON: " + e.Message);
                    } catch (...) {
                        ok = false;
                        safeShow(L"����������� ������ �������� JSON");
                    }
                }
            }
            catch (const std::exception& e)
            {
                ok = false;
                safeShow(L"����������� ������ ������� ������: " + String(e.what()));
            }
            catch (const Exception& e)
            {
                ok = false;
                safeShow(L"����������� ������ ������� ������: " + e.Message);
            }
            catch (...)
			{
                ok = false;
                safeShow(L"����������� ����������� ������ ������� ������");
            }

            TThread::Queue(nullptr, [this, to, ok, result, prev, safeShow]()
            {
                // ������ ���������� ������ � GUI-������
                Screen->Cursor = prev;

                try
				{

                    // split-������ ������ GUI-�����
					static auto mySplit = [this](const String& str, wchar_t d)
                    {
                        auto sl = std::unique_ptr<TStringList>(new TStringList());
                        sl->StrictDelimiter = true;
						sl->Delimiter = d;
                        sl->DelimitedText = str;
                        std::vector<String> res;
                        res.reserve(sl->Count);
                        for (int i = 0; i < sl->Count; ++i)
                            res.push_back(sl->Strings[i]);
                        return res;
                    };

                    if (!ok)
                        return;

                    if (result.empty())
                    {
                        safeShow(L"PDTV: ������ ��������� �� �������");
                        return;
                    }

                    const auto& m = *result.begin();

                    auto get = [&](const char* k)->String
                    {
                        try {
                            auto it = m.find(k);
                            return (it != m.end()) ? String(conv::to_wstring(it->second).c_str()) : String();
                        } catch (const std::exception& e) {
                            safeShow(L"������ ������ ����� '" + String(k) + L"': " + String(e.what()));
                            return String();
                        } catch (const Exception& e) {
							safeShow(L"������ ������ ����� '" + String(k) + L"': " + e.Message);
                            return String();
                        } catch (...) {
                            safeShow(L"����������� ������ ������ ����� '" + String(k) + L"'");
                            return String();
                        }
                    };

                    String index   = get("Index");
                    String pdtv    = get("PDTV");
                    String allpdtv1= get("AllPDTV1");

                    std::vector<String> v_to;
                    std::vector<String> v_pdtv;

                    try {
                        v_to   = mySplit(to, L';');
                        v_pdtv = mySplit(allpdtv1, L'/');
					} catch (const Exception& e) {
                        safeShow(L"������ split: " + e.Message);
                    } catch (...) {
                        safeShow(L"����������� ������ split");
                    }

                    if (v_pdtv.empty())
                    {
                        // ����������� �� ���������� ��������� (����� ����������� ������, ���� ����)
                        int need = (int)v_to.size() - 1;
                        if (need < 0) need = 0;
                        try {
                            v_pdtv.assign(need, String());
                        } catch (...) {
                            safeShow(L"������ ���������� ������ PDTV �������");
                        }
                    }

					auto appendRow = [this, &safeShow](const String& kuda, const String& file, const String& date_time, const String& dir)
					{
						try {
							mt->DisableControls();
							mt->AppendRecord(ARRAYOFCONST((kuda, file, date_time, dir)));
							mt->EnableControls();
						} catch (const Exception& e) {
							mt->EnableControls();
							safeShow(L"������ ���������� ������ � mtPdtv: " + e.Message);
						} catch (...) {
							mt->EnableControls();
							safeShow(L"����������� ������ ���������� ������ � mtPdtv");
						}
					};



					if(v_to.size() > 0)
						dbgPdtv->RowDetailPanel->Active = true;

					if (v_to.size() == 2) // ���� �������
                    {
                        String date_time, file, kuda, dir;
						std::vector<String> v_pdtv_1;
                        try {
                            v_pdtv_1 = mySplit(pdtv, L'|');
                        } catch (...) {
                            safeShow(L"������ split PDTV ��� ���������� ��������");
                        }

                        kuda = v_to[0];

                        if (v_pdtv_1.size() == 3)
                        {
                            date_time = v_pdtv_1[1];
                            file      = v_pdtv_1[0];
                            dir       = v_pdtv_1[2];
                        }
                        else
                        {
                            file = pdtv;
						}

						appendRow(kuda, file, date_time, dir);
                    }
                    else
                    {
                        int count = (int)v_to.size() - 1;
                        if (count < 0) count = 0;

                        // ��������� �� ���������������� ��������
                        if ((int)v_pdtv.size() < count)
                            count = (int)v_pdtv.size();

                        for (int i = 0; i < count; ++i)
                        {
                            String date_time, file, kuda, dir;
                            std::vector<String> v_pdtv_1;
                            try {
								v_pdtv_1 = mySplit(v_pdtv[i], L'|');
                            } catch (...) {
                                safeShow(L"������ split �������� v_pdtv[" + String(i) + L"]");
                            }

                            try { kuda = v_to[i]; }
                            catch (...) { kuda = String(); }

                            if (v_pdtv_1.size() == 3)
                            {
                                date_time = v_pdtv_1[1];
                                file      = v_pdtv_1[0];
                                dir       = v_pdtv_1[2];
                            }
                            else
                            {
                                file = v_pdtv[i];
                            }

                            appendRow(kuda, file, date_time, dir);
                        }
                    }

                    try {
                        dbgPdtv->AutoFitColWidths = true;
                    } catch (const Exception& e) {
                        safeShow(L"������ ����������� ������ �������: " + e.Message);
                    } catch (...) {
                        safeShow(L"����������� ������ ����������� ������ �������");
                    }
                }
                catch (const Exception& e)
                {
                    safeShow(L"GUI-������ ��������� PDTV: " + e.Message);
                }
                catch (const std::exception& e)
				{
                    safeShow(L"GUI-������ ��������� PDTV (std): " + String(e.what()));
                }
                catch (...)
                {
                    safeShow(L"����������� GUI-������ ��������� PDTV");
                }
			});
		});

}
void __fastcall TfrPdtv::dbgPdtvRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow)

{
	if (dbgMain && dbgMain->DataSource && dbgMain->DataSource->DataSet &&
		isDeletedRow(dbgMain->DataSource->DataSet)) {
		ShowMessage(deletedFileMessage());
		CanShow = false;
		return;
	}

	memPdtv->Text = "";
	memPdtv->ParentFont = true;
	memPdtv->Font->Size -= 3 ;
	AnsiString ansi_dir = dbgPdtv->DataSource->DataSet->FieldValues["dir"];

	if(ansi_dir.IsEmpty())
		return;


	const int main_heigth = dbgMain->Height/5;
	dbgPdtv->RowDetailPanel->Height = dbgMenu->RowDetailPanel->Height - 20;
	memPdtv->Align = alClient;
	memPdtv->Clear();

	std::string dir_str{ansi_dir.c_str()};

	if(auto it = dir_str.find(';'); it != std::string::npos)
	{
		dir_str = dir_str.erase(it);
	}

	if(auto it = dir_str.find('/'); it == dir_str.length() - 1)
	{
		dir_str = dir_str.erase(it);
	}

	std::string err;

	// 1) ��������
	std::string str;
	if (!serverManager.requestText(
			SourceServerId(), dir_str, COMMAND::FILETEXT, str, nullptr, &err)) {
		ShowMessage("Ошибка запроса: " + AnsiString(err.c_str()));
		return;
	}

// ������ �������� �� str


	TStringList* reg = new TStringList();
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg);

	memPdtv->Text = reg->Text;
	delete reg;

}
//---------------------------------------------------------------------------

