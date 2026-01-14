//---------------------------------------------------------------------------

#ifndef myJsonH
#define myJsonH

#include <my_json/ConverterJSON.h>
#include <my_json/ConverterJSON.cpp>

class JSON
{

	public:

	template<class T>
	static T get(const std::string& str)
	{
	   //	nh::json::parse(str);

		return std::move(nh::json::parse(str).get<T>());

	}

    template<class T>
	static T getFromFile(const std::string& str)
	{
	   //	nh::json::parse(str);
		std::ifstream jsonFile(str);
		T res = nh::json::parse(jsonFile).get<T>();
        jsonFile.close();
		return std::move(res);
	}

    template<class T>
	static void saveToJsonFile(const T& t, const std::string& str)
	{
		std::ofstream jsonFile(str);
		nh::json j = t;
		jsonFile << std::setw(2) << j;
		jsonFile.close();
	}

};



//---------------------------------------------------------------------------
#endif
