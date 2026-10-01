#include "AttachmentPackage.h"
#include "ClientFilePublish.h"

#include <boost/archive/binary_iarchive.hpp>
#include <algorithm>
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

namespace
{
    AttachmentPackage readAttachmentArchive(const std::string& bytes)
    {
        std::istringstream input(bytes, std::ios::binary);
        portable_binary_iarchive archive(input);
        AttachmentPackage package;
        archive >> package;
        return package;
    }

    bool findServerArchive20Version(
        const std::vector<std::uint8_t>& bytes,
        std::size_t& versionOffset)
    {
        const std::string signature(boost::archive::BOOST_ARCHIVE_SIGNATURE());
        const std::size_t signatureOffset = 2;
        const std::size_t versionSizeOffset = signatureOffset + signature.size();
        const std::size_t flagsOffset = versionSizeOffset + 2;

        // Only the known portable attachment header is compatible: one-byte
        // signature length, archive version 20, and default endian flags.
        if (bytes.size() <= flagsOffset || bytes[0] != 1 ||
            bytes[1] != signature.size() ||
            !std::equal(signature.begin(), signature.end(),
                bytes.begin() + signatureOffset) ||
            bytes[versionSizeOffset] != 1 ||
            bytes[versionSizeOffset + 1] != 20 || bytes[flagsOffset] != 0) {
            return false;
        }

        versionOffset = versionSizeOffset + 1;
        return true;
    }
} // namespace

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
    try {
        return readAttachmentArchive(str);
    } catch (const boost::archive::archive_exception& error) {
        std::size_t versionOffset = 0;
        if (error.code != boost::archive::archive_exception::unsupported_version ||
            static_cast<unsigned>(boost::archive::BOOST_ARCHIVE_VERSION()) != 17 ||
            !findServerArchive20Version(bytes, versionOffset)) {
            throw;
        }

        // Boost 1.85 and C++Builder's Boost 1.70 use the same layout for this
        // attachment map. Preserve the existing 20 -> 17 compatibility path
        // here, without changing the caller's bytes or outgoing archives.
        str[versionOffset] = static_cast<char>(17);
        return readAttachmentArchive(str);
    }
}
