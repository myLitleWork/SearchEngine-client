//---------------------------------------------------------------------------

#pragma hdrstop

#include "myRequest.h"
#include <string>
#include <cstring>
#include <iostream>
#include <codecvt>
#include <algorithm>


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
TemplateListOrder myRequest::mOrder{};
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
void myRequest::putTemplate(const String& group, const String& name, const Request& value)
{
    mTemplates[conv::to_string(group.c_str())][conv::to_string(name.c_str())] = value;
}
void myRequest::addPeriodTemplates()
{
    const wchar_t* names[] = {L"Сегодня", L"Вчера", L"Последние 7 дней", L"Последние 30 дней", L"Текущий месяц"};
    for (int type = 0; type < 2; ++type) {
        auto& group = mTemplates[conv::to_string(type == 0 ? L"Входящие" : L"Исходящие")];
        for (int i = 0; i < 5; ++i) {
            auto name = conv::to_string(names[i]);
            if (group.find(name) != group.end()) continue;
            Request value("1=1", "SQL", type, true, false);
            value.template_version = 2;
            value.period = i + 1;
            value.max_results = 300;
            value.newest_first = true;
            value.filters_saved = true;
            group.emplace(name, value);
        }
    }
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
    std::vector<std::string> defaults;
    for (const auto& entry : mTemplates) defaults.push_back(entry.first);
    std::vector<String> result;
    for (const auto& name : TemplateListOrder::apply(defaults, mOrder.groups))
        result.emplace_back(conv::to_wstring(name).c_str());
    return result;
}
std::vector<String> myRequest::groupTempaltes(const String& groupName)
{
    const auto key = conv::to_string(groupName.c_str());
    const auto& group = mTemplates.at(key);
    std::vector<std::string> defaults;
    for (const auto& entry : group) defaults.push_back(entry.first);
    // Rank by saved relative period, with a name fallback for legacy templates.
    const wchar_t* labels[] = {L"\u0421\u0435\u0433\u043e\u0434\u043d\u044f", L"\u0412\u0447\u0435\u0440\u0430", L"\u041f\u043e\u0441\u043b\u0435\u0434\u043d\u0438\u0435 7 \u0434\u043d\u0435\u0439", L"\u041f\u043e\u0441\u043b\u0435\u0434\u043d\u0438\u0435 30 \u0434\u043d\u0435\u0439", L"\u0422\u0435\u043a\u0443\u0449\u0438\u0439 \u043c\u0435\u0441\u044f\u0446"};
    std::vector<std::string> known;
    for (auto label : labels) known.push_back(conv::to_string(label));
    auto rank = [&group, &known](const std::string& name) {
        const int period = group.at(name).period;
        if (period >= 1 && period <= 5) return period - 1;
        return static_cast<int>(std::find(known.begin(), known.end(), name) - known.begin());
    };
    std::stable_sort(defaults.begin(), defaults.end(), [&rank](const std::string& a, const std::string& b) {
        return rank(a) < rank(b);
    });
    const auto saved = mOrder.templates.find(key);
    const auto names = saved == mOrder.templates.end() ? defaults : TemplateListOrder::apply(defaults, saved->second);
    std::vector<String> result;
    for (const auto& name : names) result.emplace_back(conv::to_wstring(name).c_str());
    return result;
}
