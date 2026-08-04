#pragma once

#include <nlohmann/json.hpp>
#include <string>

struct Telega
{
	enum class TYPE : int
	{
		VHOD = 0,
		ISHOD = 1,
		NOTTG = 2
	};

	std::string num;
	std::string from_to;
	std::string isp;
	std::string podp_num;
	std::string date;
	std::string date_podp;
	std::string dir;
	float rel = 0.f;
	std::string tel_num;
	std::string kr;
	std::string pril_name;
	std::string pril_count;
	std::string blank;
	std::string gde_sht;
	std::string last_mesto;
	TYPE type = TYPE::VHOD;
	bool deleted = false;
	// Client-side provenance.  It is assigned after a server response is
	// decoded and deliberately is not part of the wire JSON format.
	std::string source_server_id;
};
