//
// Created by Sg on 25.02.2024.
//

#include <map>
#include <vector>
#include <string>

#include <filesystem>

namespace fs = std::filesystem;

#ifndef MY_JSON_SORTMAKER_H
#define MY_JSON_SORTMAKER_H

void DeleteEmptyDirectoriesRecursively(const fs::path& path);

class Condition;
class SortMaker
{

	static const inline std::string path_Conditions = "Conditions\\";

protected:

    static inline std::map<std::string, std::string> mapTegsDirs{};
    static inline std::map<std::string, std::string> mapTegsJsons{};
    static inline std::map<std::string, std::vector<Condition>> mapTegsConditions{};

	static inline std::map<std::string, std::string> mapDefaultTypeDirs =
	{
		{"TELEGA", "D:\\BASES\\BUFFER"},
		{"PRIL",   "D:\\BASES\\FLASH_BUFFER\\ERROR"},
		{"TVD",    "D:\\ATLAS"},
	};

public:

	static void addConditions(const std::string& short_name);
    static void load();

};
#endif //MY_JSON_SORTMAKER_H
