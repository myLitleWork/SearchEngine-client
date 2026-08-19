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
				if (!isValidAttachmentBaseName(info.name)) {
					error = "Список приложений содержит недопустимое имя файла";
					result.clear();
					return false;
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

bool isValidAttachmentBaseName(const std::string& name)
{
	if (name.empty() || name == "." || name == "..")
		return false;
	if (name.find('/') != std::string::npos ||
		name.find('\\') != std::string::npos ||
		name.find(':') != std::string::npos ||
		name.find('\0') != std::string::npos)
		return false;

	try {
		const std::filesystem::path path(name);
		if (path.empty() ||
			path.is_absolute() ||
			path.has_root_path() ||
			path.has_root_name() ||
			path.has_root_directory() ||
			path.filename() != path)
			return false;
	}
	catch (...) {
		return false;
	}

	return true;
}

const TelegramAttachmentInfo* findAdvertisedTelegramAttachment(
	const std::vector<TelegramAttachmentInfo>& items,
	const std::string& fileName)
{
	if (!isValidAttachmentBaseName(fileName))
		return nullptr;
	for (const auto& item : items) {
		if (item.name == fileName)
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

	if (!isValidAttachmentBaseName(fileName)) {
		if (error)
			*error = "Имя приложения недопустимо";
		return false;
	}

	const std::string request =
		makeSingleRequest(telegramId, telegramType, fileName).dump();
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

bool requestTelegramText(
	MultiServerManager& manager,
	const std::string& serverId,
	int telegramId,
	int telegramType,
	std::string& result,
	std::string& error)
{
	error.clear();
	result.clear();

	const std::string request = makeListRequest(telegramId, telegramType).dump();
	if (!manager.requestText(
			serverId, request, COMMAND::GET_TELEGA_TEXT, result, nullptr, &error)) {
		if (error.empty())
			error = "Не удалось получить текст телеграммы";
		return false;
	}

	return true;
}
