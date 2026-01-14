//---------------------------------------------------------------------------

#ifndef SqlConditionH
#define SqlConditionH
#include <map>
//---------------------------------------------------------------------------
class SqlCondition;

//---------------------------------------------------------------------------
class SqlCondition
{



	static String getOrArgEq(const String& field, const String& arg)
	{
		auto upper_arg = arg.UpperCase();
		auto lower_arg = arg.LowerCase();
		auto res = "(" + field + " = '" + upper_arg + "' or " + field + " = '" + lower_arg + "')";
		return res;
	};

	static String getOrArgLike(const String& field, const String& arg)
	{
		auto upper_arg = arg.UpperCase();
		auto lower_arg = arg.LowerCase();
		auto res = "(" + field + " LIKE ('%" + upper_arg + "%') or " + field + " LIKE ('%" + lower_arg + "%'))";
		return res;
	};


	public:

	enum class TYPE
	{
		VH,
		ISH
	};

	static String getFieldName(const String& field, TYPE t);

	static String getEqualVh(const String& field, const String arg);
	static String getEqualIsh(const String& field, const String arg);
	static String getLikeIsh(const String& field, const String arg);
	static String getLikeVh(const String& field, const String arg);

	static std::map<String, String> map;
	static std::map<String, String> mapVh;
	static std::map<String, String> mapIsh;

	static int max;
	static String sort;
	static String LIMIT(){return " LIMIT " + IntToStr(max);};





	typedef String (*funcCondition)(const String&, const String);

	static funcCondition getFuncCondition (TYPE t, bool ex)
	{

		if(t == TYPE::VH)
			return ex ? &getEqualVh : &getLikeVh;
		else
			return ex ? &getEqualIsh : &getLikeIsh;

	};

};
std::map<String, String> SqlCondition::map;
std::map<String, String> SqlCondition::mapVh;
std::map<String, String> SqlCondition::mapIsh;
int SqlCondition::max;
String SqlCondition::sort = " order by `Index`  desc";
#endif
