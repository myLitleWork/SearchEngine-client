//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "FramePdtv.h"
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
TfrPdtv *frPdtv;

__fastcall TfrPdtv::TfrPdtv(TComponent* Owner) : TFrameDbg(Owner)
{

}
//---------------------------------------------------------------------------
__fastcall TfrPdtv::TfrPdtv(TComponent* _Owner, MyAsioClient* _client, TDBGridEh *_dbgMain, TDBGridEh *_dbgMenu)
  : TFrameDbg(_Owner, _client, _dbgMain, _dbgMenu)
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

        // Хелпер для безопасного показа ошибок из любых потоков
		auto safeShow = [](const String& msg)
		{
			TThread::Queue(nullptr, [m = String(msg)]() { ShowMessage(m); });
		};

		dbgPdtv->RowDetailPanel->Active = false;

		mt->EmptyTable();

		auto cmd = COMMAND::GET_ISH_PDTV;

		String numberS, to;

		numberS = dbgMain->Fields[0]->AsString; // номер
		to      = dbgMain->Fields[8]->AsString; // получатели

		// Меняем курсор только в GUI-потоке
		TCursor prev = Screen->Cursor;
		Screen->Cursor = crHourGlass;

		System::Threading::TTask::Run([this, numberS, to, cmd, prev, safeShow]()
		{
			std::string number;
			std::string jsonStr;
			bool ok = true;
			std::list<std::map<std::string, std::string>> result;

			try
			{
				number = AnsiString(numberS).c_str();

				std::string err;
				if (!client->socketWrite(number, cmd, &err)) {
					ok = false;
					safeShow(L"Ошибка socketWrite: " + AnsiString(err.c_str()));
				}


				if (ok) {
					std::string err;
					if (!client->socketRead(jsonStr, /*progress*/nullptr, &err)) {
						ok = false;
						safeShow(L"Ошибка socketRead: " + AnsiString(err.c_str()));
					}
				}



				if (ok)
				{
					try {
						result = JSON::get<std::list<std::map<std::string, std::string>>>(jsonStr);
					} catch (const std::exception& e) {
						ok = false;
						safeShow(L"Ошибка парсинга JSON (std): " + String(e.what()));
					} catch (const Exception& e) {
						ok = false;
						safeShow(L"Ошибка парсинга JSON: " + e.Message);
                    } catch (...) {
                        ok = false;
                        safeShow(L"Неизвестная ошибка парсинга JSON");
                    }
                }
            }
            catch (const std::exception& e)
            {
                ok = false;
                safeShow(L"Критическая ошибка фоновой задачи: " + String(e.what()));
            }
            catch (const Exception& e)
            {
                ok = false;
                safeShow(L"Критическая ошибка фоновой задачи: " + e.Message);
            }
            catch (...)
			{
                ok = false;
                safeShow(L"Неизвестная критическая ошибка фоновой задачи");
            }

            TThread::Queue(nullptr, [this, to, ok, result, prev, safeShow]()
            {
                // Всегда возвращаем курсор в GUI-потоке
                Screen->Cursor = prev;

                try
				{

                    // split-хелпер внутри GUI-части
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
                        safeShow(L"PDTV: пустой результат от сервера");
                        return;
                    }

                    const auto& m = *result.begin();

                    auto get = [&](const char* k)->String
                    {
                        try {
                            auto it = m.find(k);
                            return (it != m.end()) ? String(conv::to_wstring(it->second).c_str()) : String();
                        } catch (const std::exception& e) {
                            safeShow(L"Ошибка чтения ключа '" + String(k) + L"': " + String(e.what()));
                            return String();
                        } catch (const Exception& e) {
							safeShow(L"Ошибка чтения ключа '" + String(k) + L"': " + e.Message);
                            return String();
                        } catch (...) {
                            safeShow(L"Неизвестная ошибка чтения ключа '" + String(k) + L"'");
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
                        safeShow(L"Ошибка split: " + e.Message);
                    } catch (...) {
                        safeShow(L"Неизвестная ошибка split");
                    }

                    if (v_pdtv.empty())
                    {
                        // выравниваем по количеству адресатов (минус завершающий пустой, если есть)
                        int need = (int)v_to.size() - 1;
                        if (need < 0) need = 0;
                        try {
                            v_pdtv.assign(need, String());
                        } catch (...) {
                            safeShow(L"Ошибка подготовки пустых PDTV записей");
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
							safeShow(L"Ошибка добавления строки в mtPdtv: " + e.Message);
						} catch (...) {
							mt->EnableControls();
							safeShow(L"Неизвестная ошибка добавления строки в mtPdtv");
						}
					};



					if(v_to.size() > 0)
						dbgPdtv->RowDetailPanel->Active = true;

					if (v_to.size() == 2) // один адресат
                    {
                        String date_time, file, kuda, dir;
						std::vector<String> v_pdtv_1;
                        try {
                            v_pdtv_1 = mySplit(pdtv, L'|');
                        } catch (...) {
                            safeShow(L"Ошибка split PDTV для одиночного адресата");
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

                        // защитимся от рассинхронизации массивов
                        if ((int)v_pdtv.size() < count)
                            count = (int)v_pdtv.size();

                        for (int i = 0; i < count; ++i)
                        {
                            String date_time, file, kuda, dir;
                            std::vector<String> v_pdtv_1;
                            try {
								v_pdtv_1 = mySplit(v_pdtv[i], L'|');
                            } catch (...) {
                                safeShow(L"Ошибка split элемента v_pdtv[" + String(i) + L"]");
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
                        safeShow(L"Ошибка автоподбора ширины колонок: " + e.Message);
                    } catch (...) {
                        safeShow(L"Неизвестная ошибка автоподбора ширины колонок");
                    }
                }
                catch (const Exception& e)
                {
                    safeShow(L"GUI-ошибка обработки PDTV: " + e.Message);
                }
                catch (const std::exception& e)
				{
                    safeShow(L"GUI-ошибка обработки PDTV (std): " + String(e.what()));
                }
                catch (...)
                {
                    safeShow(L"Неизвестная GUI-ошибка обработки PDTV");
                }
			});
		});

}
void __fastcall TfrPdtv::dbgPdtvRowDetailPanelShow(TCustomDBGridEh *Sender, bool &CanShow)

{
	AnsiString ansi_dir = dbgPdtv->DataSource->DataSet->FieldValues["dir"];

	if(ansi_dir.IsEmpty())
		return;

	const int main_heigth = dbgMain->Height/5;
	dbgPdtv->RowDetailPanel->Height = dbgMenu->RowDetailPanel->Height -10;
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

	// 1) отправка
	if (!client->socketWrite(dir_str, COMMAND::FILETEXT, &err)) {
		ShowMessage("Ошибка отправки: " + AnsiString(err.c_str()));
		return;
	}

	// 2) чтение
	std::string str;
	if (!client->socketRead(str, /*progress*/nullptr, &err)) {
		ShowMessage("Ошибка чтения: " + AnsiString(err.c_str()));
		return;
	}

// дальше работаем со str


	TStringList* reg = new TStringList();
	reg->Text = str.c_str();
	my_utils::F_SL_OemToAnsi(reg);

	memPdtv->Text = reg->Text;
	delete reg;

}
//---------------------------------------------------------------------------
