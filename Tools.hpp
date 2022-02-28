// (c) 2022 Pttn (https://riecoin.dev/en/rieWallet)

#ifndef HEADER_Tools_hpp
#define HEADER_Tools_hpp

#include <array>
#include <chrono>
#include <curl/curl.h>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <random>
#include <string>
#include <vector>

#include "External/bech32.h"
#include "External/segwit_addr.h"


using namespace std::string_literals;
#define leading0s(x) std::setw(x) << std::setfill('0')
#define FIXED(x) std::fixed << std::setprecision(x)

inline std::random_device randomDevice;
inline uint8_t rand(uint8_t min, uint8_t max) {
	if (min > max) std::swap(min, max);
	std::uniform_int_distribution<uint8_t> urd(min, max);
	return urd(randomDevice);
}

inline bool validHex(const std::string &str) {
	for (uint16_t i(0) ; i < str.size() ; i++) {
		if (!((str[i] >= '0' && str[i] <= '9')
		   || (str[i] >= 'A' && str[i] <= 'F')
		   || (str[i] >= 'a' && str[i] <= 'f')))
			return false;
	}
	return true;
}

inline bool isHexStr(const std::string &str) {
	return std::all_of(str.begin(), str.end(), [](unsigned char c){return std::isxdigit(c);});
}
inline bool isHexStrOfSize(const std::string &str, const std::string::size_type size) {
	return str.size() == size && isHexStr(str);
}

inline std::string v8ToHexStr(const std::vector<uint8_t> &v) {
	std::ostringstream oss;
	for (const auto &u8 : v) oss << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(u8);
	return oss.str();
}
inline std::vector<uint8_t> hexStrToV8(std::string str) {
	if (str.size() % 2 != 0) str = "0" + str;
	std::vector<uint8_t> v;
	for (std::string::size_type i(0) ; i < str.size() ; i += 2) {
		uint8_t byte;
		try {byte = std::stoll(str.substr(i, 2), nullptr, 16);}
		catch (...) {byte = 0;}
		v.push_back(byte);
	}
	return v;
}

template<std::size_t N> std::array<uint8_t, N> v8ToA8(std::vector<uint8_t> v8) {
	std::array<uint8_t, N> a8{0};
	std::copy_n(v8.begin(), N, a8.begin());
	return a8;
}
template<std::size_t N> std::vector<uint8_t> a8ToV8(const std::array<uint8_t, N> &a8) {
	return std::vector<uint8_t>(a8.begin(), a8.end());
}

inline std::array<uint8_t, 4> u32ToA8(const uint32_t u32) {
	return std::array<uint8_t, 4>{static_cast<uint8_t>(u32 % 256U),
		static_cast<uint8_t>((u32 >> 8U) % 256U),
		static_cast<uint8_t>((u32 >> 16U) % 256U),
		static_cast<uint8_t>((u32 >> 24U))};
}

inline std::array<uint8_t, 8> u64ToA8(uint64_t u64) {
	std::array<uint8_t, 8> a8;
	for (uint32_t i(0) ; i < 8 ; i++) {
		a8[i] = u64 % 256U;
		u64 /= 256U;
	}
	return a8;
}

inline std::vector<uint8_t> sha256(const uint8_t *data, uint32_t len) {
	std::vector<uint8_t> hash;
	uint8_t hashTmp[32];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, data, len);
	SHA256_Final(hashTmp, &sha256);
	for (uint8_t i(0) ; i < 32 ; i++) hash.push_back(hashTmp[i]);
	return hash;
}

inline std::vector<uint8_t> sha256sha256(const uint8_t *data, uint32_t len) {
	std::vector<uint8_t> hash;
	hash = sha256(data, len);
	hash = sha256(hash.data(), 32);
	return hash;
}

inline std::vector<uint8_t> ripem160(const uint8_t *data, uint32_t len) {
	std::vector<uint8_t> hash;
	uint8_t hashTmp[20];
	RIPEMD160_CTX ripem160;
	RIPEMD160_Init(&ripem160);
	RIPEMD160_Update(&ripem160, data, len);
	RIPEMD160_Final(hashTmp, &ripem160);
	for (uint8_t i(0) ; i < 20 ; i++) hash.push_back(hashTmp[i]);
	return hash;
}

inline double timeSince(const std::chrono::time_point<std::chrono::steady_clock> &t0) {
	const std::chrono::time_point<std::chrono::steady_clock> t(std::chrono::steady_clock::now());
	const std::chrono::duration<double> dt(t - t0);
	return dt.count();
}

inline size_t curlWriteCallback(void *data, size_t size, size_t nmemb, std::string *s) {
	s->append((char*) data, size*nmemb);
	return size*nmemb;
}

inline nlohmann::json postRequest(const std::string &url, const std::string &postData) {
	CURL *curl(curl_easy_init());
	if (curl == nullptr)
		throw std::runtime_error("Could not init Curl");
	std::string s;
	nlohmann::json jsonObj;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
	const CURLcode cc(curl_easy_perform(curl));
	if (cc != CURLE_OK) {
		curl_easy_cleanup(curl);
		throw std::runtime_error("Curl_easy_perform failed: "s + std::string(curl_easy_strerror(cc)));
	}
	try {
		jsonObj = nlohmann::json::parse(s);
	}
	catch (nlohmann::json::parse_error &e) {
		curl_easy_cleanup(curl);
		throw std::runtime_error("Received invalid JSON from server, content was "s + s);
	}
	curl_easy_cleanup(curl);
	return jsonObj;
}

#endif
