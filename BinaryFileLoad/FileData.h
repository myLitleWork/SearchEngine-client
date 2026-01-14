#ifndef FILEDATA_H
#define FILEDATA_H

#include <iostream>
#include <string>
#include <vector>
#include "boost/archive/portable_binary_oarchive.hpp"
#include "boost/archive/portable_binary_iarchive.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <codecvt>
#include <locale>
#include <cstdint>

// Класс FileData для хранения имени файла и его данных

class FileData {

public:
    FileData() = default;
    FileData(const std::string& filename, const std::vector<uint8_t>& data);
    FileData(const std::string& filepath);  // Новый конструктор

    const std::string& getFilename() const;
    const std::vector<uint8_t>& getData() const;

    friend std::ostream& operator<<(std::ostream& os, const FileData& fd);

private:

    std::string filename;
    std::vector<uint8_t> data;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};


std::vector<uint8_t> serializeToBytes(const FileData& fileData);
FileData deserializeFromBytes(const std::vector<uint8_t>& bytes);


#endif // FILEDATA_H
