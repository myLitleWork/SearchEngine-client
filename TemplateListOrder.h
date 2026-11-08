#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <vector>

// Only lists explicitly reordered by the user have an entry here.
struct TemplateListOrder {
    std::vector<std::string> groups;
    std::map<std::string, std::vector<std::string>> templates;

    static std::vector<std::string> apply(const std::vector<std::string>& defaults,
        const std::vector<std::string>& saved) {
        std::vector<std::string> result;
        for (const auto& name : saved) {
            if (std::find(defaults.begin(), defaults.end(), name) != defaults.end() &&
                std::find(result.begin(), result.end(), name) == result.end())
                result.push_back(name);
        }
        // Newly added entries retain their default order after the manual part.
        for (const auto& name : defaults) {
            if (std::find(result.begin(), result.end(), name) == result.end())
                result.push_back(name);
        }
        return result;
    }
};
