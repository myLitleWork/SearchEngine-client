#pragma once

#include "MultiServerManager.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

using TelegramAttachmentListKey = std::tuple<std::string, int, int>;

struct TelegramAttachmentInfo
{
	std::string name;
	bool exists{false};
	std::uint64_t size{0};
};

bool isValidAttachmentBaseName(const std::string& name);

const TelegramAttachmentInfo* findAdvertisedTelegramAttachment(
	const std::vector<TelegramAttachmentInfo>& items,
	const std::string& fileName);

bool requestTelegramAttachmentList(
	MultiServerManager& manager,
	const std::string& serverId,
	int telegramId,
	int telegramType,
	std::vector<TelegramAttachmentInfo>& result,
	std::string& error);

bool downloadTelegramAttachment(
	MultiServerManager& manager,
	const std::string& serverId,
	int telegramId,
	int telegramType,
	const std::string& fileName,
	const std::string& localPath,
	std::uint64_t* receivedSize = nullptr,
	std::string* error = nullptr,
	bool flushStreamToOs = false,
	bool replaceAfterSuccess = false);
