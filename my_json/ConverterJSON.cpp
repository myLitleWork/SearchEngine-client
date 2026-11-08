//
// Created by user on 04.02.2023.
//

#include "ConverterJSON.h"
#include "Telega.h"
#include "Request.h"
#include "Condition.h"

#define FROM_JSON(J, S) J.at(#S).get_to(val.S);
#define TO_JSON(J, S) J[#S] = val.S;

namespace
{
	bool jsonDeleted(const nh::json& j)
	{
		if (!j.contains("deleted"))
			return false;
		const auto& d = j.at("deleted");
		if (d.is_boolean())
			return d.get<bool>();
		if (d.is_number_integer())
			return d.get<int>() != 0;
		if (d.is_number_unsigned())
			return d.get<unsigned>() != 0;
		if (d.is_string()) {
			const auto s = d.get<std::string>();
			return s == "1" || s == "true" || s == "True";
		}
		return false;
	}
}


std::vector<std::string> ConverterJSON::getRequests(const std::string& jsonPath) {

    /**
    функция получения запросов из json файла, адресованных поисковому серверу.
    Если json файла - некорректен или отсутствует возвращается пустой вектор,
    работа сервера не будет прекращена. */

    std::vector<std::string> requests;
	nh::json jsonRequests;
	std::ifstream jsonFileRequests(jsonPath);

    try
    {
        jsonRequests = nh::json::parse(jsonFileRequests);
        jsonFileRequests.close();
    }
    catch (nh::json::parse_error& ex)
    {
        jsonFileRequests.close();
        return std::move(requests);
    }
    catch (...)
    {
        jsonFileRequests.close();
        return std::move(requests);
    }

    jsonRequests.at("Requests").get_to(requests);

    return requests;
}

std::string ConverterJSON::putAnswers(const listAnswers& answers, const std::string& jsonPath) {
	/**
     функция записи ответов сервера на запросы в json файл. */

	nh::json jsonAnswers;

    for(const auto& answer: answers)
    {
        if(!answer.first.empty())
        {
            jsonAnswers["Answers"][answer.second]["result"] = true;
            jsonAnswers["Answers"][answer.second]["relevance"] = answer.first;
        }
        else
            jsonAnswers["Answers"][answer.second]["result"] = false;
    }

	std::ofstream jsonFileSettings(jsonPath);

	jsonFileSettings << std::setw(2) << jsonAnswers;
    std::string ss = nh::to_string(jsonAnswers);
	jsonFileSettings.close();

    return move(ss);
}

std::vector<std::string> ConverterJSON::getRequestsFromString(const std::string &jsonString) {

    std::vector<std::string> requests;
    nh::json jsonRequests;

    try
    {
        jsonRequests = nh::json::parse(jsonString);
    }
    catch (nh::json::parse_error& ex)
    {
        return std::move(requests);
    }
    catch (...)
    {
        return std::move(requests);
    }

    jsonRequests.at("Requests").get_to(requests);

    return requests;
}

std::string ConverterJSON::getJsonTelega(const Telega &val) {

    nh::json jsonTelega;


    TO_JSON(jsonTelega, num)
    TO_JSON(jsonTelega, type)
    TO_JSON(jsonTelega, from_to)
    TO_JSON(jsonTelega, isp)
    TO_JSON(jsonTelega, podp_num)
    TO_JSON(jsonTelega, date)
    TO_JSON(jsonTelega, date_podp)
    TO_JSON(jsonTelega, dir)
    TO_JSON(jsonTelega, rel)
	TO_JSON(jsonTelega, kr)
	TO_JSON(jsonTelega, pril_name)
	TO_JSON(jsonTelega, pril_count)
	TO_JSON(jsonTelega, blank)
    TO_JSON(jsonTelega, gde_sht)
    TO_JSON(jsonTelega, last_mesto)
    TO_JSON(jsonTelega, deleted)

    return to_string(jsonTelega);
}

void to_json(nh::json& jsonTelega, const Telega &val)
{
    TO_JSON(jsonTelega, num)
    TO_JSON(jsonTelega, type)
    TO_JSON(jsonTelega, from_to)
    TO_JSON(jsonTelega, isp)
    TO_JSON(jsonTelega, podp_num)
    TO_JSON(jsonTelega, date)
    TO_JSON(jsonTelega, date_podp)
    TO_JSON(jsonTelega, dir)
    TO_JSON(jsonTelega, rel)
    TO_JSON(jsonTelega, tel_num)
    TO_JSON(jsonTelega, kr)
    TO_JSON(jsonTelega, pril_name)
    TO_JSON(jsonTelega, pril_count)
    TO_JSON(jsonTelega, blank)
    TO_JSON(jsonTelega, gde_sht)
    TO_JSON(jsonTelega, last_mesto)
    TO_JSON(jsonTelega, deleted)
}
void from_json(const nh::json& jsonTelega, Telega &val)
{
    FROM_JSON(jsonTelega, num)
    FROM_JSON(jsonTelega, type)
    FROM_JSON(jsonTelega, from_to)
    FROM_JSON(jsonTelega, isp)
    FROM_JSON(jsonTelega, podp_num)
    FROM_JSON(jsonTelega, date)
    FROM_JSON(jsonTelega, date_podp)
    FROM_JSON(jsonTelega, dir)
    FROM_JSON(jsonTelega, tel_num)
    FROM_JSON(jsonTelega, rel)
    FROM_JSON(jsonTelega, kr)
    FROM_JSON(jsonTelega, pril_name)
    FROM_JSON(jsonTelega, pril_count)
    FROM_JSON(jsonTelega, blank)
    FROM_JSON(jsonTelega, gde_sht)
    FROM_JSON(jsonTelega, last_mesto)
    val.deleted = jsonDeleted(jsonTelega);
}


void to_json(nh::json& jsonTelega, const Request &val)
{
    TO_JSON(jsonTelega, search_text)
    TO_JSON(jsonTelega, type)
    TO_JSON(jsonTelega, sql)
	TO_JSON(jsonTelega, field)
	TO_JSON(jsonTelega, exc)
    if (val.template_version >= 2) {
        jsonTelega["template_version"] = 2;
        jsonTelega["period"] = val.period;
        jsonTelega["date_field"] = val.date_field;
        jsonTelega["max_results"] = val.max_results;
        if (val.unlimited_results) jsonTelega["unlimited_results"] = true;
        jsonTelega["newest_first"] = val.newest_first;
        jsonTelega["filters_saved"] = val.filters_saved;
        jsonTelega["column_filters"] = val.column_filters;
    }
}

void from_json(const nh::json& jsonTelega, Request &val)
{
    FROM_JSON(jsonTelega, search_text)
    FROM_JSON(jsonTelega, type)
    FROM_JSON(jsonTelega, sql)
    FROM_JSON(jsonTelega, field)
	FROM_JSON(jsonTelega, exc)
    val.template_version = jsonTelega.value("template_version", 1);
    val.period = jsonTelega.value("period", 0);
    if (val.period < 0 || val.period > 5) val.period = 0;
    val.date_field = jsonTelega.value("date_field", std::string("DData"));
    if (val.date_field != "DataPodp") val.date_field = "DData";
    val.max_results = jsonTelega.value("max_results", 0);
    val.unlimited_results = jsonTelega.value("unlimited_results", false);
    val.newest_first = jsonTelega.value("newest_first", false);
    val.filters_saved = jsonTelega.value("filters_saved", false);
    val.column_filters = jsonTelega.value("column_filters", std::map<std::string, std::string>{});
}

void to_json(nh::json& jsonTelega, const Condition &val)
{
    TO_JSON(jsonTelega, condition_and)
    TO_JSON(jsonTelega, condition_or)
    TO_JSON(jsonTelega, condition_not)
    TO_JSON(jsonTelega, both_or_one)
}
void from_json(const nh::json& jsonTelega, Condition &val)
{
    FROM_JSON(jsonTelega, condition_and)
    FROM_JSON(jsonTelega, condition_or)
    FROM_JSON(jsonTelega, condition_not)
    FROM_JSON(jsonTelega, both_or_one)
}
