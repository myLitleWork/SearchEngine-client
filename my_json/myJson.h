//---------------------------------------------------------------------------

#ifndef myJsonH
#define myJsonH

#include <ConverterJSON.h>
#include <ConverterJSON.cpp>

#define TO_STR(J,S) #J#S
#define FROM_JSON_TYPE(J) J = JSON::getFromFile<decltype(J)>(TO_STR(J,.json));
#define TO_JSON_TYPE(J) JSON::saveToJsonFile<decltype(J)>(J, TO_STR(J,.json));

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
