//
// Created by Sg on 25.02.2024.
//
#include <map>
#include <vector>
#include <string>
#include "myJson.h"

#ifndef MY_JSON_MAPTYPE_H
#define MY_JSON_MAPTYPE_H




typedef std::vector<std::string> vecExt;
class mapType
{


    static std::map<std::string, vecExt> map;

public:
    static void loadDefaultMapType()
    {
        mapType::map["TELEGA"] =  {".atl", ".shp", ".ist", ".txt"};
		mapType::map["PRIL"] = {".doc", ".rar", ".zip", ".7z", ".ppt", ".xls", ".pdf", ".jpg", ".xml"};
        mapType::map["TVD"] = {".tvd"};
    }

    static void loadMapType (const std::map<std::string, std::vector<std::string>>& map_type)
    {
            map = map_type;
    }


    static std::string getType(const std::string& _ext)
    {
        for(const auto& m: map)
            for(const auto& ext: m.second)
                if(ext == _ext)
                    return m.first;

            return "UNKNOW";
    }

};
std::map<std::string, vecExt> mapType::map{};



#endif //MY_JSON_MAPTYPE_H
