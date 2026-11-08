#pragma once
#include "Net/SearchRequestControl.h"
#include "my_json/ConverterJSON.h"
#include "my_json/Telega.h"
#include <vector>

struct SearchResponseParser {
    std::vector<Telega>* result;
    const std::string* serverId;
    SearchRequestControl* control;
    // Convert and discard each completed top-level record. This avoids holding
    // a second DOM copy of the complete result, and makes parsing cancellable.
    bool operator()(int depth, nh::json::parse_event_t event, nh::json& value) const {
        control->checkpoint();
        if (depth == 1 && event == nh::json::parse_event_t::object_end) {
            control->record();
            auto hit = value.get<Telega>();
            hit.source_server_id = *serverId;
            result->push_back(std::move(hit));
            return false;
        }
        if (depth == 1 && event == nh::json::parse_event_t::value)
            throw std::runtime_error("Expected a document object in search response");
        return true;
    }
};

inline std::vector<Telega> parseSearchResponse(const std::string& response,
    const std::string& serverId, SearchRequestControl& control) {
    std::vector<Telega> result;
    const auto root = nh::json::parse(response, SearchResponseParser{&result, &serverId, &control});
    if (!root.is_array()) throw std::runtime_error("Expected a search result array");
    control.checkpoint();
    return result;
}
