//---------------------------------------------------------------------------

#pragma hdrstop

#include "myRequest.h"
#include <string>
#include <cstring>
#include <iostream>
#include <codecvt>


namespace conv
{
	using convert_t = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr)                                 //Конструкция перевода в String из WString
	{
		return strconverter.to_bytes(wstr);
	}

std::wstring to_wstring(std::string str)                                 //Конструкция перевода в WString из String
	{
		return strconverter.from_bytes(str);
	}
}

//---------------------------------------------------------------------------
#pragma package(smart_init)
 myRequest::myRequest(String _field, String _text, int _type, bool _sql, bool _exc)
{
		search_text = conv::to_string(_text.c_str());
		field = conv::to_string(_field.c_str());
		type = _type;
		sql = _sql;
		exc = _exc;
}

myRequest::myRequest()
{
		search_text = conv::to_string(cmbText->Text.c_str());
		field = conv::to_string(cmbField->Text.c_str());
		type = rgTypeT->ItemIndex;
		sql = chkSql->Checked;
		exc = chkExc->Checked;
}

void myRequest::init(TComboBox *_cmbField, TComboBox *_text, TCheckBox *_sql, TCheckBox *_exc, TRadioGroup *_rgTypeT)
{
		cmbField = _cmbField;
		cmbText = _text;
		chkSql = _sql;
		chkExc = _exc;
		rgTypeT = _rgTypeT;
}
void myRequest::setTemplate(const String& _group, const String _name)
{
		auto getStr = [](std::string s)
		{
			auto w = conv::to_wstring(s);
			return String(w.c_str());
		};

		auto t = getTemplate(_group, _name);

		cmbField->Text = getStr(t.field);
		cmbText->Text = getStr(t.search_text);
		chkSql->Checked = t.sql;
		chkExc->Checked = t.exc;
		rgTypeT->ItemIndex = t.type;
}
void myRequest::saveTemplate(const String& _group, const String _name)
{
	mTemplates[conv::to_string(_group.c_str())][conv::to_string(_name.c_str())] = myRequest();
}
const std::map<std::string, Request>& myRequest::getGroups(const String& _group)
{
	return mTemplates.at(conv::to_string(_group.c_str()));
}
Request myRequest::getTemplate(const String& _group, const String _name)
{
	return  mTemplates.at(conv::to_string(_group.c_str())).at(conv::to_string(_name.c_str()));
}
void myRequest::removeGroup(const String& _group)
{
	mTemplates.erase(conv::to_string(_group.c_str()));
}
void myRequest::removeTemplate(const String& _group, const String _name)
{
	mTemplates.at(conv::to_string(_group.c_str())).erase(conv::to_string(_name.c_str()));
}
std::vector<String> myRequest::groupsNames()
{
	std::vector<String> res;
	for(const auto& p : mTemplates)
		res.emplace_back(conv::to_wstring(p.first).c_str());
	return res;
}
std::vector<String> myRequest::groupTempaltes(const String& _group_name)
{
	std::vector<String> res;
	for(const auto& p : mTemplates.at(conv::to_string(_group_name.c_str())))
		res.emplace_back(conv::to_wstring(p.first).c_str());
	return res;
}

