//---------------------------------------------------------------------------

#ifndef SqlConditionH
#define SqlConditionH
#include <map>
//---------------------------------------------------------------------------
class SqlCondition;

//---------------------------------------------------------------------------
class SqlCondition
{



	static String sqlEscapeLiteral(const String& arg)
	{
		String out;
		out.SetLength(0);
		for (int i = 1; i <= arg.Length(); ++i) {
			const WideChar c = arg[i];
			if (c == L'\'')
				out += L"''";
			else
				out += c;
		}
		return out;
	}

	static String getOrArgEq(const String& field, const String& arg)
	{
		auto upper_arg = sqlEscapeLiteral(arg.UpperCase());
		auto lower_arg = sqlEscapeLiteral(arg.LowerCase());
		auto res = "(" + field + " = '" + upper_arg + "' or " + field + " = '" + lower_arg + "')";
		return res;
	};

	static String getOrArgLike(const String& field, const String& arg)
	{
		auto upper_arg = sqlEscapeLiteral(arg.UpperCase());
		auto lower_arg = sqlEscapeLiteral(arg.LowerCase());
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

    static String predicate(const String& field, const String& arg, TYPE type, bool exact) {
        return exact ? getOrArgEq(getFieldName(field, type), arg) : getOrArgLike(getFieldName(field, type), arg);
    }
	static int max;
	static String sort;
	static String LIMIT(){return max > 0 ? String(" LIMIT ") + IntToStr(max) : String();};





	typedef String (*funcCondition)(const String&, const String);

	static funcCondition getFuncCondition (TYPE t, bool ex)
	{

		if(t == TYPE::VH)
			return ex ? &getEqualVh : &getLikeVh;
		else
			return ex ? &getEqualIsh : &getLikeIsh;

	};

};
#endif
