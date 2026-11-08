#pragma once
#include <string>

namespace template_search {
// AutoPad date columns are text. Accept dd.mm.yyyy, ISO and compact yyyyMMdd.
inline std::string dateSql(const std::string& field) {
    const std::string f = field == "DataPodp" ? "trim(DataPodp)" : "trim(DData)";
    return "date(CASE WHEN substr(" + f + ",3,1)='.' THEN substr(" + f +
        ",7,4)||'-'||substr(" + f + ",4,2)||'-'||substr(" + f +
        ",1,2) WHEN length(" + f + ")=8 THEN substr(" + f +
        ",1,4)||'-'||substr(" + f + ",5,2)||'-'||substr(" + f +
        ",7,2) ELSE substr(" + f + ",1,10) END)";
}
inline std::string condition(const std::string& predicate, const std::string& field,
    const std::string& from, const std::string& to, bool newest, int limit) {
    std::string result = "(" + predicate + ")";
    if (!from.empty()) result += " AND " + dateSql(field) + " BETWEEN '" + from + "' AND '" + to + "'";
    result += " ORDER BY ";
    if (newest) result += dateSql(field) + " DESC, ";
    result += "`index` DESC";
    if (limit > 0) result += " LIMIT " + std::to_string(limit);
    return result;
}
}
