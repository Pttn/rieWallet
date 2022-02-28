// (c) 2022 Pttn (https://riecoin.dev/en/rieWallet)

#ifndef HEADER_Wallet_hpp
#define HEADER_Wallet_hpp

#include <openssl/ec.h>
#include <optional>

#include "Tools.hpp"

class Key {
	EC_KEY *_ecKey;
	std::array<uint8_t, 32> _privateKey;
	std::array<uint8_t, 33> _publicKeyCompressed;
	std::vector<uint8_t> _witnessProgram;
	
	void _ecdsaPubGen();
public:
	Key() {
		for (uint8_t &byte : _privateKey)
			byte = rand(0x00U, 0xffU);
		_ecdsaPubGen();
		_witnessProgram = ripem160(sha256(_publicKeyCompressed.data(), 33).data(), 32);
	}
	Key(const std::array<uint8_t, 32> &privateKey) : _privateKey(privateKey) {
		_ecdsaPubGen();
		_witnessProgram = ripem160(sha256(_publicKeyCompressed.data(), 33).data(), 32);
	}
	Key(const Key &key) = delete;
	~Key() {EC_KEY_free(_ecKey);}
	
	std::array<uint8_t, 32> getKey() const {return _privateKey;}
	std::array<uint8_t, 33> getPubKeyCompressed() const {return _publicKeyCompressed;}
	std::vector<uint8_t> getWitnessProgram() const {return _witnessProgram;}
	
	std::vector<unsigned char> sign(const std::vector<uint8_t> &message);
};

struct Utxo {
	std::array<uint8_t, 32> txId;
	uint32_t vOut;
	std::vector<uint8_t> witnessProgram;
	uint64_t value;
	std::shared_ptr<Key> key;
};

inline bool operator==(const Utxo &utxo1, const Utxo &utxo2) {return utxo1.txId == utxo2.txId && utxo1.vOut == utxo2.vOut;}

struct Output {
	std::vector<uint8_t> witnessProgram;
	uint64_t value;
};

class Transaction {
	std::vector<Utxo> _inputs;
	std::vector<Output> _outputs;
	std::optional<Utxo> _change;
	
	std::vector<uint8_t> _raw, _legacyRaw;
	std::array<uint8_t, 32> _txId;
	
	uint64_t _inputValue, _outputValue, _fee;
public:
	Transaction(const std::vector<Utxo> &inputs, const std::vector<Output> &outputs, std::optional<Utxo>);
	
	std::vector<uint8_t> getRawTransaction() const {return _raw;}
	std::array<uint8_t, 32> getTxId() const {return _txId;}
	
	std::vector<Utxo> getInputs() const {return _inputs;}
	std::vector<Output> getOutputs() const {return _outputs;}
	std::optional<Utxo> getChange() const {return _change;}
	uint64_t getFee() const {return _fee;}
};

class Wallet {
	std::map<std::string, std::shared_ptr<Key>> _keys;
	std::vector<Utxo> _utxos;
	std::vector<Output> _outputs;
	uint64_t _balance;
	
	std::vector<Transaction> _pendingTransactions;
public:
	Wallet() : _balance(0ULL) {}
	
	void loadKeysFromFile(const std::string&);
	void saveKeysToFile(const std::string &path);
	
	std::vector<std::string> getAddresses() const {
		std::vector<std::string> addresses;
		for (const auto &key : _keys)
			addresses.push_back(key.first);
		return addresses;
	}
	void addKey(const std::shared_ptr<Key> &key) {
		if (_keys.size() >= 100)
			throw std::runtime_error("Too many keys, please delete some manually in the private key file (and be sure to double check that you are not deleting an address that has funds) or move your funds to a new wallet.");
		_keys[segwit_addr::encode("ric", 0, key->getWitnessProgram())] = key;
	}
	std::shared_ptr<Key> getKey(const std::string &address) {return _keys.at(address);}
	
	void fetchUtxos();
	uint64_t getBalance() const {return _balance;}
	std::vector<Utxo> getUtxos() const {return _utxos;}
	Transaction createTransaction(const std::vector<std::pair<std::string, uint64_t>>&);
	void broadcastTransaction(const Transaction&);
};

#endif
