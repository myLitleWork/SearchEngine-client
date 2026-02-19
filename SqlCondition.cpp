//---------------------------------------------------------------------------

#pragma hdrstop

#include "SqlCondition.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

std::map<String, String> SqlCondition::map;
std::map<String, String> SqlCondition::mapVh;
std::map<String, String> SqlCondition::mapIsh;
int SqlCondition::max;
String SqlCondition::sort = L" order by `Index`  desc";

String SqlCondition::getFieldName(const String& field, TYPE t)
{
	auto res = map.at(field);

	if(t == TYPE::ISH)
		res =  mapIsh.find(res) == mapIsh.end() ? res : mapIsh.at(res);
	else
		res = mapVh.find(res) == mapVh.end() ? res : mapVh.at(res);

	return res;
}

String SqlCondition::getEqualIsh(const String& field, const String arg)
{
	return  getOrArgEq(getFieldName(field, TYPE::ISH), arg) + sort + LIMIT();
}

String SqlCondition::getEqualVh(const String& field, const String arg)
{
	return  getOrArgEq(getFieldName(field, TYPE::VH), arg) + sort + LIMIT();
}

String SqlCondition::getLikeVh(const String& field, const String arg)
{
	return  getOrArgLike(getFieldName(field, TYPE::VH),  arg) + sort + LIMIT();
   //	return  getFieldName(field, TYPE::VH) + " GLOB '*[?-??-?]*'";
}

String SqlCondition::getLikeIsh(const String& field, const String arg)
{
	return  getOrArgLike(getFieldName(field, TYPE::ISH), arg) + sort + LIMIT();
}