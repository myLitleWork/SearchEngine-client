//
// Created by Sg on 25.02.2024.
//

#include <vector>
#include <regex>
#include <algorithm>
#include <map>
#include "Converter.h"

#ifndef MY_JSON_CONDITION_H
#define MY_JSON_CONDITION_H




struct Condition
{
	std::vector<std::string> condition_and;
	std::vector<std::string> condition_or;
	std::vector<std::string> condition_not;

    bool both_or_one = true;

	static bool myFunc(const std::wstring& str, const std::string& con)
	{
	   std::wstring w_con = Converter::upperWStr(Converter::to_wstring(con));


	   std::wregex r{w_con};
	   std::wsmatch wm;

		if(std::regex_search(str,wm,r))
			return true;

		return false;
	}

public:

	bool check(const std::wstring& str) const
	{

	   if(condition_and.empty() && condition_or.empty())
			return false;

		bool b_not = std::any_of(condition_not.cbegin(),condition_not.cend(),  [&str](const auto& c){return myFunc(str,c);});

		if(b_not)
			return false;

		bool b_and = false, b_or = false;

		b_and = std::all_of(condition_and.cbegin(),condition_and.cend(),  [&str](const auto& c){return myFunc(str,c);});

		b_or = std::any_of(condition_or.cbegin(),condition_or.cend(), [&str](const auto& c){return myFunc(str,c);});

		if(condition_and.empty())
			return b_or;
		else if(condition_or.empty())
			return b_and;
		else if(both_or_one)
			return b_or && b_and;
		else
			return b_or || b_and;

	}




};
#endif //MY_JSON_CONDITION_H
