//---------------------------------------------------------------------------

#pragma hdrstop

#include "flashNum.h"
#include <functional>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>
//---------------------------------------------------------------------------
#pragma package(smart_init)
std::string FlashPodpis::getResultStr() const
{
	return serial_number + random_str + fio;
}
void FlashPodpis::calcHash()
{
	std::hash<std::string> hasher;
	size_t hash_podp_int = hasher(getResultStr());
	hash_podp = std::to_string(hash_podp_int);

}
std::string FlashPodpis::getHashStr() const
{
	return hash_podp;
}

void FlashPodpis::setHashPodp(std::string&& str_hash)
{
	podpis = std::move(str_hash);
}
bool FlashPodpis::checkPodp(const std::string _hash_str)
{
	return hash_podp == _hash_str;
}



// Сохранение объекта FlashPodpis в бинарный файл
void saveToBinaryFile(const FlashPodpis& msg, const std::string& filePath) {
    std::ofstream ofs(filePath, std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);
    oa << msg;
}

// Загрузка объекта FlashPodpis из бинарного файла
FlashPodpis loadFromBinaryFile(const std::string& filePath) {
    FlashPodpis msg;
    std::ifstream ifs(filePath, std::ios::binary);
    if (ifs.is_open()) {
        boost::archive::binary_iarchive ia(ifs);
        ia >> msg;
    } else {
        std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
    }
    return msg;
}
