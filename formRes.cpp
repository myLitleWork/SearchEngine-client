//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "formRes.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <windows.h>
#include <my_json/ConverterJSON.h>
#include <my_json/ConverterJSON.cpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfRes *fRes;

namespace conv
{
	using namespace std;
	using convert_t = std::codecvt_utf8<wchar_t>;
	wstring_convert<convert_t, wchar_t> strconverter;

	string to_string(std::wstring wstr)                                 //Конструкция перевода в String из WString
	{
		return strconverter.to_bytes(wstr);
	}

wstring to_wstring(std::string str)                                 //Конструкция перевода в WString из String
	{
		return strconverter.from_bytes(str);
	}
}

//---------------------------------------------------------------------------
__fastcall TfRes::TfRes(TComponent* Owner)
	: TForm(Owner)
{
	dp->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TfRes::bAddClick(TObject *Sender)
{
	add_res_press = true;
   //	TaskToForm::setData(getJsonRes().c_str());
}
//---------------------------------------------------------------------------
void __fastcall TfRes::FormClose(TObject *Sender, TCloseAction &Action)
{
   //if(!add_res_press)
   //	TaskToForm::setData("#Close");
}
//---------------------------------------------------------------------------
std::string __fastcall TfRes::getJsonRes()
{


	auto getSize_t = [this](auto& set)
	{
		std::set<std::string> res;
		for(auto i:set)
		{
			auto s =((AnsiString) myExcs.at(i)).c_str() ;
			res.insert(s);
		}
	   return res;
	};

	nh::json jr;

	jr["isp"] = sIsp;
	jr["n_sht"] = n_sht;
	jr["isp"] = getSize_t(sIsp);
	jr["so_isp"] = getSize_t(sSoIsp);
	jr["dead_line"] = dp->Date.toAnsiString();
	jr["text"] = conv::to_string(memo->Text.c_str());

	return std::move(to_string(jr));
}
void __fastcall TfRes::FormShow(TObject *Sender)
{
	//chkExcs->Clear();

	dp->Visible = false;
	chkSrok->Checked = false;
	chkExcs->Clear();
	memo->Clear();
	lstIsp->Clear();
	lstSoIsp->Clear();
	sIsp.clear();
	sSoIsp.clear();

    add_res_press = false;

	for(const auto& s:myExcs)
		chkExcs->Items->Add(s.first);

}
//---------------------------------------------------------------------------

void __fastcall TfRes::chkSrokClick(TObject *Sender)
{
	if(chkSrok->Checked == true)
	{
		dp->Visible = true;
	}
	else
	{
		dp->Visible = false;
	}
}
//---------------------------------------------------------------------------


void __fastcall TfRes::chkExcsClick(TObject *Sender)
{
	bAddRem->Caption = "Добавить";
	lstSoIsp->MultiSelect = false;
	lstIsp->MultiSelect = false;
	lstIsp->ItemIndex = -1;
	lstSoIsp->ItemIndex = -1;

}
//---------------------------------------------------------------------------

void __fastcall TfRes::lstIspClick(TObject *Sender)
{
	lstIsp->MultiSelect = true;
	bAddRem->Caption = "Убрать";
	chkExcs->ItemIndex = -1;
	lstSoIsp->MultiSelect = false;
	lstSoIsp->ItemIndex = -1;
	rgIS->ItemIndex = 0;
}
//---------------------------------------------------------------------------

void __fastcall TfRes::lstSoIspClick(TObject *Sender)
{
	lstSoIsp->MultiSelect = true;
	bAddRem->Caption = "Убрать";
	chkExcs->ItemIndex = -1;
	lstIsp->MultiSelect = false;
	lstIsp->ItemIndex = -1;
	rgIS->ItemIndex = 1;
}
//---------------------------------------------------------------------------

void __fastcall TfRes::bAddRemClick(TObject *Sender)
{
	if(rgIS->ItemIndex == -1)
        return;
	if(chkExcs->ItemIndex == -1 && lstIsp->ItemIndex == -1 && lstSoIsp->ItemIndex == -1)
		return;

	auto newLst = [this](auto& s1, auto& s2)
	{
		lstIsp->Clear();
		lstSoIsp->Clear();
		for(const auto& i: s1)
				lstIsp->Items->Add(i);
		for(const auto& i: s2)
				lstSoIsp->Items->Add(i);
	};

	auto change = [this](auto& set, auto& lst)
	{

		if(chkExcs->ItemIndex != -1)
		{
			for (int i = 0; i < chkExcs->Items->Count; i++)
				if(chkExcs->Checked[i])
					set.insert(chkExcs->Items->Strings[i]);
		}

		auto n = lst->Items->Count;

		for (int i = 0; i < n; i++)
		if(lst->Selected[i])
			set.erase(lst->Items->Strings[i]);

	};

	auto setDif = [this](auto& s1, auto& s2)
	{
		for(auto i:s2)
			if(s1.find(i) != s1.end())
				s1.erase(i);
	};

	if(rgIS->ItemIndex == 0)
	{
		change(sIsp,lstIsp);
		setDif(sSoIsp,sIsp);
	}
	else
	{
		change(sSoIsp,lstSoIsp);
		setDif(sIsp,sSoIsp);
	}

	newLst(sIsp,sSoIsp);

	for (int i = 0; i < chkExcs->Items->Count; i++)
		chkExcs->Checked[i] = false;

	chkExcs->ItemIndex = -1;
	rgIS->ItemIndex = -1;
    bAddRem->Caption = "Добавить/Убрать";
}
//---------------------------------------------------------------------------


