//
// Created by Sg on 27.02.2024.
//
#include <codecvt>
#include <locale>
#include <map>
#include <fstream>
#include "wchar.h"
#include "windows.h"

#ifndef MY_JSON_CONVERTER_H
#define MY_JSON_CONVERTER_H

class Converter
{


    using convert_t = std::codecvt_utf8<wchar_t>;







public:
	inline static std::map<char,char> mapOemChar = {};
	inline static std::map<wchar_t,wchar_t> mapWChar = {};

	static void load (const std::string p)
    {
        std::ifstream file(p, std::ios::binary);

        if(!file.is_open())
            return;

		std::string s1;
		std::string s2;
		std::wstring w1;
		std::wstring w2;
		getline(file,s1);
        getline(file,s2);
        file.close();

		for(int i =0;i<33;i++)
			mapOemChar.insert(std::make_pair(s1[i],s2[i]));

		wchar_t wt1[33];
		wchar_t wt2[33];

		OemToCharBuffW(s1.c_str(),wt1,33);
		OemToCharBuffW(s2.c_str(),wt2,33);

		w1 = std::wstring {wt1};
		w2 = std::wstring {wt2};

		for(int i =0;i<33;i++)
			mapWChar.insert(std::make_pair(w1[i],w2[i]));
    }
    static bool iS_not_a_Oem(char c)
    {
        static auto a_oem = mapOemChar.begin()->first;
        return c != a_oem;
    }
    static char getUpperCharOem(char c)
    {
        if(auto i = mapOemChar.find(c);i!=mapOemChar.end())
            return i->second;
        else
            return c;
    };

	static std::string upperOemStr(const std::string& str)
    {
		std::string res;
		for(auto& c:str)
			res.push_back(getUpperCharOem(c));
		return std::move(res);
    }

	static wchar_t getUpperWChar(wchar_t c)
    {
        if(auto i = mapWChar.find(c);i!=mapWChar.end())
            return i->second;
        else
            return c;
    };
	static std::wstring upperWStr(const std::wstring& str)
    {
		std::wstring res;
		for(auto& c:str)
			res.push_back(getUpperWChar(c));
		return std::move(res);
	}

    static std::string to_string(std::wstring wstr)                                 //Конструкция перевода в String из WString
    {
        std::wstring_convert<convert_t, wchar_t> strconverter;
        return strconverter.to_bytes(wstr);
	}

    static std::wstring to_wstring(std::string str)                                 //Конструкция перевода в WString из String
    {
        std::wstring_convert<convert_t, wchar_t> strconverter;
        return strconverter.from_bytes(str);
    }

	static std::wstring fromAnsiToWString(const std::string& str)
	{
		size_t count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), NULL, 0);
		std::wstring wstr(count,0);
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], count);


		return std::move(wstr);
    }


};

#endif //MY_JSON_CONVERTER_H
