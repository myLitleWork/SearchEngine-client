//---------------------------------------------------------------------------

#ifndef MessageH
#define MessageH
#include <map>
#include <vector>
#include <string>
#include <set>
#include <filesystem>
#include <boost/serialization/serialization.hpp>
#include "boost/archive/portable_binary_oarchive.hpp"
#include "boost/archive/portable_binary_iarchive.hpp"
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include "MyStrings/myWstring.h"

#define BYTE unsigned char


struct AttachmentSaveItem
{
	std::string key;
	std::string writtenAs;
};

struct AttachmentSaveReport
{
	std::vector<AttachmentSaveItem> saved;
	std::vector<std::pair<std::string, std::string>> failed; // key, error

	[[nodiscard]] bool allSucceeded() const { return failed.empty(); }
};

struct Message
{

	std::map<std::filesystem::path, std::vector<BYTE>> attachments;

	void addAttachment(const std::filesystem::path& _file_dir);


	// Writes only attachments whose key is not in alreadySaved.
	// Continues after per-item failures; caller retries only remaining keys.
	AttachmentSaveReport saveToDirectory(
		const std::filesystem::path& dirPath,
		const std::set<std::string>& alreadySaved = {}) const;
	Message() = default;


	[[nodiscard]] size_t getAttachCount() const {return attachments.size();}
    static Message loadFromDirectory(const std::filesystem::path& dirPath);


    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & attachments;
    }

    static std::vector<uint8_t> serializeToBytes(const Message& fileData);
    static Message deserializeFromBytes(const std::vector<uint8_t>& bytes);

};




//---------------------------------------------------------------------------
#endif
