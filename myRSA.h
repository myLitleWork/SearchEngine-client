//---------------------------------------------------------------------------
#include <filesystem>
#include <tuple>
#include <functional>
#include <iostream>
#include <cstring>
#include <memory>
#include <vector>
#include <fstream>
#ifndef myRSAH
#define myRSAH
#define DLL_EXPORT(S) extern "C" __declspec(dllexport) S __stdcall

namespace dll
{

    DLL_EXPORT(char*) RsaPriDecrypt(const unsigned char* cipher_text, size_t cipher_text_len, const unsigned char* pri_key, int pri_key_len, int* decrypted_len);
    DLL_EXPORT(char*) RsaPubEncrypt(const unsigned char* clear_text, size_t clear_text_len, const unsigned char* pub_key, int pub_key_len, int* encrypted_len);
	DLL_EXPORT(void) GenerateRSAKey();
    DLL_EXPORT(void) myDelete(char* ptr);

    DLL_EXPORT(unsigned char*) __stdcall GenerateAESKey();
    DLL_EXPORT(char*) __stdcall AESEncrypt(const unsigned char* clear_text, size_t clear_text_len, const unsigned char* key, int key_len, int* encrypted_len);
    DLL_EXPORT(char*) __stdcall AESDecrypt(const unsigned char* cipher_text, size_t cipher_text_len, const unsigned char* key, int key_len, int* decrypted_len);

    DLL_EXPORT(char*) RsaPriEncrypt(const unsigned char* clear_text, size_t clear_text_len, const unsigned char* pri_key, int pri_key_len, int* encrypted_len);
    DLL_EXPORT(char*) RsaPubDecrypt(const unsigned char* cipher_text, size_t cipher_text_len, const unsigned char* pub_key, int pub_key_len, int* decrypted_len);

}
std::string unsignedCharToString(const unsigned char* arr, size_t len) {
	return std::string(reinterpret_cast<const char*>(arr), len);
}

unsigned char* stringToUnsignedChar(const std::string& str) {
    unsigned char* result = new unsigned char[str.length()];
    std::memcpy(result, str.c_str(), str.length());
    return result;
}

namespace fs = std::filesystem;

class RSA
{
	static inline const size_t aes_key_length = 32;
	static inline const size_t key_length = 4096; // Key length
	const std::string pub_key_file = "pubkey.pem"; // public key path
	const std::string pri_key_file = "prikey.pem"; // private key path
	using CryptoFunction = std::function<char*(const unsigned char*, size_t, const unsigned char*, int, int*)>;

	char* loadKeyFromFile(const std::string& filename, int& key_len)
	{
		std::ifstream file(filename, std::ios::binary);

		if (!file.is_open()) {
		   //	std::cerr << "Failed to open file: " << filename << std::endl;
			return nullptr;
		}

		file.seekg(0, std::ios::end);
		key_len = file.tellg();
		file.seekg(0, std::ios::beg);
		char* key = new char[key_len];
		file.read(key, key_len);
		file.close();

		return key;
	}


	public:

	void init()
	{

		bool pri_key_file_ex = fs::exists(pri_key_file);
		bool pub_key_file_ex = fs::exists(pub_key_file);

		if(pri_key_file_ex || pub_key_file_ex)
			return;

		dll::GenerateRSAKey();

	}


	std::string CryptoUniversal(const std::string& input_text, const unsigned char* char_key, int key_len, CryptoFunction cryptoFunc)
	{
		const unsigned char* input_char = (unsigned char*)input_text.data();
		size_t input_len = input_text.length(); // Используем length() для получения длины строки

		int output_len = 0;

		char* output_char = cryptoFunc(input_char, input_len, char_key, key_len, &output_len);
		delete[] char_key;
		if (!output_char)
		{
			return {}; // Возвращаем пустую строку в случае неудачи
		}

	    // Используем конструктор char*char_key,intkey_len,std::string, принимающий указатель и длину
		std::string output_text(output_char, output_len);
		dll::myDelete(output_char);

		return std::move(output_text); // Возвращаем объект char*char_key,intkey_len,std::string (move не нужен)
	}

	std::string RsaPubEncrypt(const std::string& input_text)
	{
			int keyLen;
			unsigned char* char_key = (unsigned char*)loadKeyFromFile(pub_key_file, keyLen);
			return CryptoUniversal(input_text, char_key, keyLen, dll::RsaPubEncrypt);
	}
	std::string RsaPriDecrypt(const std::string& input_text)
	{
			int keyLen;
			unsigned char* char_key = (unsigned char*)loadKeyFromFile(pri_key_file, keyLen);
			return CryptoUniversal(input_text, char_key, keyLen, dll::RsaPriDecrypt);
	}

    std::string RsaPriEncrypt(const std::string& input_text)
	{
		int keyLen;
		unsigned char* char_key = (unsigned char*)loadKeyFromFile(pri_key_file, keyLen);  // Загружаем закрытый ключ из файла
		return CryptoUniversal(input_text, char_key, keyLen, dll::RsaPriEncrypt);  // Шифруем с использованием закрытого ключа
	}

    std::string RsaPubDecrypt(const std::string& input_text)
	{
		int keyLen;
		unsigned char* char_key = (unsigned char*)loadKeyFromFile(pub_key_file, keyLen);  // Загружаем открытый ключ из файла
		return CryptoUniversal(input_text, char_key, keyLen, dll::RsaPubDecrypt);  // Расшифровываем с использованием открытого ключа
	}

	std::string AESEncrypt(const std::string& input_text, const std::string key)
	{
			const unsigned char* char_key = (unsigned char*) key.data();
			return CryptoUniversal(input_text, char_key, aes_key_length, dll::AESEncrypt);
	}

	std::string AESDecrypt(const std::string& input_text, const std::string key)
	{
			const  unsigned char* char_key = (unsigned char*) key.data();
			return CryptoUniversal(input_text, char_key, aes_key_length, dll::AESDecrypt);
	}

	static std::string GenerateAESKey()
	{
		unsigned char* char_key = dll::GenerateAESKey();
		std::string key = unsignedCharToString(char_key, aes_key_length);
		dll::myDelete((char*)char_key);
		return key;
	}

	std::vector<uint8_t> CryptoUniversal(const std::vector<uint8_t>& input_data, const std::string& key, CryptoFunction cryptoFunc)
	{
		size_t input_len = input_data.size();

		int output_len = 0;

		char* output_char = cryptoFunc(input_data.data(), input_len, reinterpret_cast<const unsigned char*>(key.data()), key.length(), &output_len);
		if (!output_char)
		{
			return {}; // Возвращаем пустой вектор в случае неудачи
		}

		// Создаем вектор из зашифрованных данных
		std::vector<uint8_t> output_data(output_char, output_char + output_len);
		dll::myDelete(output_char);

		return output_data; // Возвращаем вектор с зашифрованными данными
	}

	std::vector<uint8_t> RsaPubEncrypt(const std::vector<uint8_t>& input_data, const std::string& pub_key_file)
	{
		int keyLen;
		char* char_key = loadKeyFromFile(pub_key_file, keyLen);
		std::string key(char_key, char_key + keyLen);
		dll::myDelete(char_key);
		return CryptoUniversal(input_data, key, dll::RsaPubEncrypt);
	}

	std::vector<uint8_t> RsaPriDecrypt(const std::vector<uint8_t>& input_data, const std::string& pri_key_file)
	{
		int keyLen;
		char* char_key = loadKeyFromFile(pri_key_file, keyLen);
		std::string key(char_key, char_key + keyLen);
		dll::myDelete(char_key);
		return CryptoUniversal(input_data, key, dll::RsaPriDecrypt);
	}

	std::vector<uint8_t> AESEncrypt(const std::vector<uint8_t>& input_data, const std::string& key)
	{
		return CryptoUniversal(input_data, key, dll::AESEncrypt);
	}

	std::vector<uint8_t> AESDecrypt(const std::vector<uint8_t>& input_data, const std::string& key)
	{
		return CryptoUniversal(input_data, key, dll::AESDecrypt);
	}



};

//---------------------------------------------------------------------------
#endif
