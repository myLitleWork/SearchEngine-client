//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "TelegramAttachmentClient.h"

#include <nlohmann/json.hpp>

#include <filesystem>

//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace
{
	nlohmann::json makeListRequest(int telegramId, int telegramType)
	{
		return nlohmann::json{
			{"id", telegramId},
			{"type", telegramType}
		};
	}

	nlohmann::json makeSingleRequest(
		int telegramId,
		int telegramType,
		const std::string& fileName)
	{
		return nlohmann::json{
			{"id", telegramId},
			{"type", telegramType},
			{"file_name", fileName}
		};
	}

	bool parseAttachmentList(
		const std::string& jsonStr,
		std::vector<TelegramAttachmentInfo>& result,
		std::string& error)
	{
		nlohmann::json parsed;
		try {
			parsed = nlohmann::json::parse(jsonStr);
		}
		catch (const std::exception& ex) {
			error = std::string("Некорректный JSON списка приложений: ") + ex.what();
			return false;
		}

		if (!parsed.is_array()) {
			error = "Ответ списка приложений не является JSON-массивом";
			return false;
		}

		result.clear();
		result.reserve(parsed.size());
		try {
			for (const auto& item : parsed) {
				if (!item.is_object()) {
					error = "Элемент списка приложений не является JSON-объектом";
					result.clear();
					return false;
				}

				TelegramAttachmentInfo info;
				info.name = item.value("name", std::string{});
				info.exists = item.value("exists", false);
				info.size = 0;
				if (item.contains("size") &&
					(item["size"].is_number_unsigned() || item["size"].is_number_integer())) {
					const auto raw = item["size"].get<std::uint64_t>();
					info.size = raw;
				}
				result.push_back(std::move(info));
			}
		}
		catch (const std::exception& ex) {
			error = std::string("Некорректный элемент списка приложений: ") + ex.what();
			result.clear();
			return false;
		}

		return true;
	}
}

std::string telegramAttachmentBaseName(const std::string& name)
{
	if (name.empty())
		return {};
	try {
		return std::filesystem::path(name).filename().string();
	}
	catch (...) {
		return {};
	}
}

const TelegramAttachmentInfo* findAdvertisedTelegramAttachment(
	const std::vector<TelegramAttachmentInfo>& items,
	const std::string& fileName)
{
	const std::string wanted = telegramAttachmentBaseName(fileName);
	if (wanted.empty())
		return nullptr;
	for (const auto& item : items) {
		if (telegramAttachmentBaseName(item.name) == wanted)
			return &item;
	}
	return nullptr;
}

bool requestTelegramAttachmentList(
	MultiServerManager& manager,
	const std::string& serverId,
	int telegramId,
	int telegramType,
	std::vector<TelegramAttachmentInfo>& result,
	std::string& error)
{
	error.clear();
	result.clear();

	const std::string request = makeListRequest(telegramId, telegramType).dump();
	std::string jsonStr;
	if (!manager.requestText(
			serverId, request, COMMAND::GET_TELEGA_ATACHMENTS, jsonStr, nullptr, &error)) {
		if (error.empty())
			error = "Не удалось получить список приложений";
		return false;
	}

	return parseAttachmentList(jsonStr, result, error);
}

bool downloadTelegramAttachment(
	MultiServerManager& manager,
	const std::string& serverId,
	int telegramId,
	int telegramType,
	const std::string& fileName,
	const std::string& localPath,
	std::uint64_t* receivedSize,
	std::string* error,
	bool flushStreamToOs,
	bool replaceAfterSuccess)
{
	if (error)
		error->clear();

	const std::string advertisedName = telegramAttachmentBaseName(fileName);
	if (advertisedName.empty()) {
		if (error)
			*error = "Имя приложения пустое";
		return false;
	}

	const std::string request =
		makeSingleRequest(telegramId, telegramType, advertisedName).dump();
	return manager.requestFileToPath(
		serverId,
		request,
		COMMAND::GET_TELEGA_SINGLE_ATACHMENT,
		localPath,
		nullptr,
		receivedSize,
		error,
		flushStreamToOs,
		replaceAfterSuccess);
}
