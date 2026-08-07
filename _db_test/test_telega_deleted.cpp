#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include "../my_json/Telega.h"

int main()
{
	using json = nlohmann::json;

	const auto withDeleted = json::parse(
		R"([{"num":"12-34","type":0,"dir":"C:\\a.txt","rel":1.0,"deleted":true}])");
	auto v1 = withDeleted.get<std::vector<Telega>>();
	if (v1.size() != 1 || !v1[0].deleted)
		return 1;

	const auto withoutDeleted = json::parse(
		R"([{"num":"56","type":1,"dir":"C:\\b.txt","rel":0.5}])");
	auto v2 = withoutDeleted.get<std::vector<Telega>>();
	if (v2.size() != 1 || v2[0].deleted)
		return 2;

	std::cout << "ok\n";
	return 0;
}
