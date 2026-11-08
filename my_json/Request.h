//
// Created by Sg on 09.02.2024.
//
#include <utility>
#include "string"
#include <map>
#ifndef REQUEST_REQUEST_H
#define REQUEST_REQUEST_H

class Request {

public:
    Request() = default;
   std::string  search_text;
   std::string  field;
   int type{};
   bool sql{};
   bool exc{};
   int template_version = 1;
   int period = 0;
   std::string date_field = "DData";
   int max_results = 0;
   bool unlimited_results = false;
   bool newest_first = false;
   bool filters_saved = false;
   std::map<std::string, std::string> column_filters;

    Request(std::string _search_text, std::string _field, int _type, bool _sql, bool _exc)
    {
        search_text  = std::move(_search_text);
        field = std::move(_field);
        type = _type;
        sql = _sql;
		exc = _exc;
    }

};

#endif //REQUEST_REQUEST_H
