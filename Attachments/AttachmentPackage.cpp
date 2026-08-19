#include "AttachmentPackage.h"
#include "ClientFilePublish.h"

#include <boost/archive/binary_iarchive.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace boost::serialization {

template<class Archive>
void serialize(Archive& ar, std::filesystem::path& p, const unsigned int)
{
    std::string str = p.string();
    ar & str;
    if (Archive::is_loading::value) {
        p = std::filesystem::path(str);
    }
}

} // namespace boost::serialization

void AttachmentPackage::addAttachment(const std::filesystem::path& file_dir)
{
    std::ifstream file(file_dir, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл: " + file_dir.string());
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), {});
    attachments[file_dir.filename().string()] = data;
}

AttachmentSaveReport AttachmentPackage::saveToDirectory(
    const std::filesystem::path& dirPath,
    const std::set<std::string>& alreadySaved) const
{
    AttachmentSaveReport report;
    std::filesystem::create_directory(dirPath);

    for (const auto& [relative_path, data] : attachments) {
        const std::string key = relative_path.string();
        if (alreadySaved.find(key) != alreadySaved.end()) {
            continue;
        }

        std::filesystem::path absolutePath;
        std::wstring pathError;
        if (!client_file_publish::ResolveUnderBase(
                dirPath, relative_path, absolutePath, &pathError)) {
            report.failed.emplace_back(
                key,
                "Небезопасный путь вложения (" +
                    client_file_publish::Narrow(pathError) + ")");
            continue;
        }
        if (!client_file_publish::EnsureUniqueFilePath(absolutePath)) {
            report.failed.emplace_back(
                key, "Не удалось подобрать свободное имя");
            continue;
        }
        if (!client_file_publish::WriteNewFile(
                absolutePath, data.data(), data.size(), &pathError)) {
            report.failed.emplace_back(
                key,
                "Ошибка записи (" + client_file_publish::Narrow(pathError) + ")");
            continue;
        }

        AttachmentSaveItem item;
        item.key = key;
        item.writtenAs = absolutePath.filename().string();
        report.saved.push_back(std::move(item));
    }

    return report;
}

AttachmentPackage AttachmentPackage::loadFromDirectory(const std::filesystem::path& dirPath)
{
    AttachmentPackage package;
    for (auto& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
        if (std::filesystem::is_regular_file(entry)) {
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) {
                throw std::runtime_error(
                    "Не удалось открыть файл вложения: " + entry.path().string());
            }
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), {});
            std::filesystem::path relative_path =
                std::filesystem::relative(entry.path(), dirPath);
            package.attachments[relative_path.string()] = data;
        }
    }

    return package;
}

std::vector<uint8_t> AttachmentPackage::serializeToBytes(
    const AttachmentPackage& fileData)
{
    std::ostringstream oss(std::ios::binary);
    portable_binary_oarchive oa(oss);
    oa << fileData;

    const std::string str = oss.str();
    std::vector<std::uint8_t> buffer;
    buffer.reserve(str.size());
    for (char item : str) {
        buffer.push_back(static_cast<std::uint8_t>(item));
    }

    return buffer;
}

AttachmentPackage AttachmentPackage::deserializeFromBytes(
    const std::vector<uint8_t>& bytes)
{
    std::string str(bytes.begin(), bytes.end());
    std::istringstream iss(str, std::ios::binary);
    portable_binary_iarchive ia(iss);

    AttachmentPackage package;
    ia >> package;

    return package;
}
