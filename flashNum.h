//---------------------------------------------------------------------------

#ifndef flashNumH
#define flashNumH
#include <iostream>
#include <Windows.h>
#include <string>
#include <fstream>

#include <iostream>
#include <string>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>


std::string GetVolumeSerialNumber(std::string drive) {

	if (drive.empty())  return "";
	if (drive.back() != '\\') drive += std::string{'\\'};

    DWORD serialNumber = 0;
    if (GetVolumeInformationA(
        drive.c_str(),
        nullptr,
		0,
        &serialNumber,
        nullptr,
        nullptr,
        nullptr,
        0))
    {
        return std::to_string(serialNumber);
    }
    return "";
}

bool CheckTokenFile(const std::string& drive, const std::string& expectedSerialNumber, const std::string& tokenFileName) {
    // Получение серийного номера текущей флешки
	std::string currentSerialNumber = GetVolumeSerialNumber(drive);
    if (currentSerialNumber != expectedSerialNumber) {
        return false; // Серийный номер не совпадает
    }

    // Проверка наличия файла-токена
    std::ifstream tokenFile(drive + "\\" + tokenFileName);
    if (!tokenFile) {
        return false; // Файл не найден
    }

    return true; // Серийный номер совпадает, файл найден
}



struct FlashPodpis
{
    std::string serial_number;
    std::string random_str;
    std::string fio;
    std::string hash_podp;
    std::string podpis;

    std::string getResultStr() const;
    std::string getHashStr() const;
    void setHashPodp(std::string&& str_hash);
    bool checkPodp(const std::string hash_str);
    void calcHash();

    // Метод для сериализации
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & serial_number;
        ar & random_str;
        ar & fio;
        ar & hash_podp;
        ar & podpis;
    }
};
FlashPodpis loadFromBinaryFile(const std::string& filePath = "token-HS.bin");
void saveToBinaryFile(const FlashPodpis& msg, const std::string& filePath);
#endif
