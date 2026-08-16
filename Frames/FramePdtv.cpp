//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "FramePdtv.h"
#include "utils.h"
#include "myAsioClient.h"
#include "myJson.h"
#include <System.Threading.hpp>
#include <memory>
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
	InitDbgDS();
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

		dbgPdtv->RowDetailPanel->Active = false;

		mt->EmptyTable();

		auto cmd = COMMAND::GET_ISH_PDTV;
		const std::string serverId = SourceServerId();

		String numberS, to;

		numberS = dbgMain->Fields[0]->AsString; // номер документа
		to      = dbgMain->Fields[8]->AsString; // получатели

		TCursor prev = Screen->Cursor;
		Screen->Cursor = crHourGlass;

		auto live = uiLive_;
		auto op = TrackOp();
		auto safeShow = [live](const String& msg)
		{
			TThread::Queue(nullptr, [live, m = String(msg)]() {
				if (!live || !live->load(std::memory_order_acquire))
					return;
				ShowMessage(m);
			});
		};

		System::Threading::TTask::Run([this, numberS, to, cmd, prev, safeShow, serverId, live, op]()
		{
			std::string number;
			std::string jsonStr;
			bool ok = true;
			std::list<std::map<std::string, std::string>> result;

			try
			{
				if (live && !live->load(std::memory_order_acquire))
					ok = false;

				number = AnsiString(numberS).c_str();

				std::string err;
				if (ok && !serverManager.requestText(
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
						safeShow(L"Ошибка разбора ответа ПДТВ (std): " + String(e.what()));
					} catch (const Exception& e) {
						ok = false;
						safeShow(L"Ошибка разбора ответа ПДТВ: " + e.Message);
                    } catch (...) {
                        ok = false;
                        safeShow(L"Неизвестная ошибка разбора ответа ПДТВ");
                    }
                }
            }
            catch (const std::exception& e)
            {
                ok = false;
                safeShow(L"Ошибка запроса данных ПДТВ (std): " + String(e.what()));
            }
            catch (const Exception& e)
            {
                ok = false;
                safeShow(L"Ошибка запроса данных ПДТВ: " + e.Message);
            }
            catch (...)
			{
                ok = false;
                safeShow(L"Неизвестная ошибка запроса данных ПДТВ");
            }

            TThread::Queue(nullptr, [this, to, ok, result, prev, safeShow, live, op]()
            {
				if (!live || !live->load(std::memory_order_acquire) ||
					detached_.load(std::memory_order_acquire))
					return;

                Screen->Cursor = prev;

                try
				{

                    // Разбор строк выполняется только в GUI-потоке.
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
                        safeShow(L"ПДТВ: сервер не вернул данные");
                        return;
                    }

                    const auto& m = *result.begin();

                    auto get = [&](const char* k)->String
                    {
                        try {
                            auto it = m.find(k);
                            return (it != m.end()) ? String(conv::to_wstring(it->second).c_str()) : String();
                        } catch (const std::exception& e) {
                            safeShow(L"Ошибка чтения поля ПДТВ '" + String(k) + L"': " + String(e.what()));
                            return String();
                        } catch (const Exception& e) {
							safeShow(L"Ошибка чтения поля ПДТВ '" + String(k) + L"': " + e.Message);
                            return String();
                        } catch (...) {
                            safeShow(L"Неизвестная ошибка чтения поля ПДТВ '" + String(k) + L"'");
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
                        safeShow(L"Ошибка разбора данных ПДТВ: " + e.Message);
                    } catch (...) {
                        safeShow(L"Неизвестная ошибка разбора данных ПДТВ");
                    }

                    if (v_pdtv.empty())
                    {
                        // Дополняем отсутствующие элементы пустыми значениями.
                        int need = (int)v_to.size() - 1;
                        if (need < 0) need = 0;
                        try {
                            v_pdtv.assign(need, String());
                        } catch (...) {
                            safeShow(L"Не удалось подготовить пустые элементы ПДТВ");
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
							safeShow(L"Ошибка добавления строки ПДТВ: " + e.Message);
						} catch (...) {
							mt->EnableControls();
							safeShow(L"Неизвестная ошибка добавления строки ПДТВ");
						}
					};



					if(v_to.size() > 0)
						dbgPdtv->RowDetailPanel->Active = true;

					if (v_to.size() == 2) // один получатель
                    {
                        String date_time, file, kuda, dir;
						std::vector<String> v_pdtv_1;
                        try {
                            v_pdtv_1 = mySplit(pdtv, L'|');
                        } catch (...) {
                            safeShow(L"Ошибка разбора ПДТВ для одного получателя");
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

                        // Ограничиваем число строк доступными получателями и записями ПДТВ.
                        if ((int)v_pdtv.size() < count)
                            count = (int)v_pdtv.size();

                        for (int i = 0; i < count; ++i)
                        {
                            String date_time, file, kuda, dir;
                            std::vector<String> v_pdtv_1;
                            try {
								v_pdtv_1 = mySplit(v_pdtv[i], L'|');
                            } catch (...) {
                                safeShow(L"Ошибка разбора элемента ПДТВ №" + String(i + 1));
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
                        safeShow(L"Ошибка настройки ширины колонок ПДТВ: " + e.Message);
                    } catch (...) {
                        safeShow(L"Неизвестная ошибка настройки ширины колонок ПДТВ");
                    }
                }
                catch (const Exception& e)
                {
                    safeShow(L"Ошибка отображения ПДТВ: " + e.Message);
                }
                catch (const std::exception& e)
				{
                    safeShow(L"Ошибка отображения ПДТВ (std): " + String(e.what()));
                }
                catch (...)
                {
                    safeShow(L"Неизвестная ошибка отображения ПДТВ");
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

	// Запрашиваем текст подтверждения.
	std::string str;
	if (!serverManager.requestText(
			SourceServerId(), dir_str, COMMAND::FILETEXT, str, nullptr, &err)) {
		ShowMessage("Ошибка запроса: " + AnsiString(err.c_str()));
		return;
	}

// Ответ сервера находится в str.


	std::unique_ptr<TStringList> reg(new TStringList());
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg.get());

	memPdtv->Text = reg->Text;

}
//---------------------------------------------------------------------------

