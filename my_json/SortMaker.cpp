//
// Created by Sg on 28.02.2024.
//
#include "SortMaker.h"
#include "myJson.h"
#include "Condition.h"

void DeleteEmptyDirectoriesRecursively(const fs::path& path) {
    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_directory(entry)) {
            DeleteEmptyDirectoriesRecursively(entry.path());

            // Удаляем пустую директорию
            if (fs::is_empty(entry.path())) {
                fs::remove(entry.path());
            }
        }
    }
}


void SortMaker::addConditions(const std::string& short_name)
{
    auto full_name = mapTegsJsons.at(short_name);
    mapTegsConditions[short_name] = JSON::getFromFile<std::vector<Condition>>(full_name);
}

void  SortMaker::load()
{

    if(std::filesystem::exists(TO_STR(mapDefaultTypeDirs,.json)))
        FROM_JSON_TYPE(mapDefaultTypeDirs)

    FROM_JSON_TYPE(mapTegsDirs)
    FROM_JSON_TYPE(mapTegsJsons)

    for(const auto& item : mapTegsJsons)
        mapTegsConditions[item.first] = JSON::getFromFile<std::vector<Condition>>(path_Conditions + item.second);

}