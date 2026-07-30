//
// Created by Sg on 09.02.2024.
//
#include <utility>
#include "string"
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
