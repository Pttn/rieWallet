// (c) 2022 Pttn (https://riecoin.dev/en/rieWallet)

#include <fstream>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>

#include "main.hpp"
#include "Wallet.hpp"

void Key::_ecdsaPubGen() {
	EC_GROUP *group(EC_GROUP_new_by_curve_name(NID_secp256k1));
	if (group == nullptr)
		throw std::runtime_error("Could not create Group");
	BN_CTX *ctx(BN_CTX_new());
	if (ctx == nullptr) {
		EC_GROUP_free(group);
		throw std::runtime_error("Could not create Context");
	}
	BN_CTX_start(ctx);
	BIGNUM *bnPrv(BN_bin2bn(_privateKey.data(), 32, nullptr));
	if (bnPrv == nullptr) {
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not convert private key bytes to BigNum");
	}
	EC_POINT *p(EC_POINT_new(group));
	if (p == nullptr) {
		BN_free(bnPrv);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not init public key point");
	}
	if (EC_POINT_mul(group, p, bnPrv, nullptr, nullptr, ctx) != 1) {
		EC_POINT_free(p);
		BN_free(bnPrv);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not calculate public key");
	}
	if (EC_POINT_point2oct(group, p, static_cast<point_conversion_form_t>(2), _publicKeyCompressed.data(), 33, ctx) != 33) {
		EC_POINT_free(p);
		BN_free(bnPrv);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not get compressed public key");
	}
	_ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
	if (_ecKey == nullptr) {
		EC_POINT_free(p);
		BN_free(bnPrv);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not init private key");
	}
	if (EC_KEY_set_private_key(_ecKey, bnPrv) != 1) {
		EC_KEY_free(_ecKey);
		EC_POINT_free(p);
		BN_free(bnPrv);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not set private key");
	}
	if (EC_KEY_set_public_key(_ecKey, p) != 1) {
		EC_KEY_free(_ecKey);
		EC_POINT_free(p);
		BN_free(bnPrv);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		EC_GROUP_free(group);
		throw std::runtime_error("Could not set public key");
	}
	EC_POINT_free(p);
	BN_free(bnPrv);
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	EC_GROUP_free(group);
}

std::vector<uint8_t> Key::sign(const std::vector<uint8_t> &data) {
	ECDSA_SIG *signature(ECDSA_do_sign(data.data(), data.size(), _ecKey));
	if (signature == nullptr)
		throw std::runtime_error("Could not sign data");
	BN_CTX *ctx(BN_CTX_new());
	if (ctx == nullptr) {
		ECDSA_SIG_free(signature);
		throw std::runtime_error("Could not create Context");
	}
	BN_CTX_start(ctx);
	const EC_GROUP *group(EC_KEY_get0_group(_ecKey));
	if (group == nullptr) {
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		ECDSA_SIG_free(signature);
		throw std::runtime_error("Could not get Group");
	}
	BIGNUM *order(BN_CTX_get(ctx)), *halforder(BN_CTX_get(ctx)), *r(BN_CTX_get(ctx)), *s(BN_CTX_get(ctx));
	if (order == nullptr || halforder == nullptr || r == nullptr || s == nullptr) {
		if (order != nullptr) BN_free(order);
		if (halforder != nullptr) BN_free(halforder);
		if (r != nullptr) BN_free(r);
		if (s != nullptr) BN_free(s);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		ECDSA_SIG_free(signature);
		throw std::runtime_error("Could not init Bignums");
	}
	s = BN_dup(ECDSA_SIG_get0_s(signature));
	if (EC_GROUP_get_order(group, order, ctx) == 0) {
		BN_free(order);
		BN_free(halforder);
		BN_free(r);
		BN_free(s);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		ECDSA_SIG_free(signature);
		throw std::runtime_error("Could not get order");
	}
	BN_rshift1(halforder, order);
	if (BN_cmp(s, halforder) > 0) {
		BN_sub(s, order, s);
		r = BN_dup(ECDSA_SIG_get0_r(signature));
		ECDSA_SIG_set0(signature, r, s);
	}
	std::vector<uint8_t> signatureV8(ECDSA_size(_ecKey));
	unsigned char *p(&signatureV8[0]);
	signatureV8.resize(i2d_ECDSA_SIG(signature, &p));
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	ECDSA_SIG_free(signature);
	return signatureV8;
}

Transaction::Transaction(const std::vector<Utxo> &inputs, const std::vector<Output> &outputs, std::optional<Utxo> change) : _inputs(inputs), _outputs(outputs), _change(change), _inputValue(0ULL), _outputValue(0ULL), _fee(0ULL) { // https://riecoin.dev/en/Transaction_Creation
	std::vector<uint8_t> witnesses;
	// Transaction Version (02000000)
	_raw.insert(_raw.end(), {0x02, 0x00, 0x00, 0x00});
	_legacyRaw.insert(_legacyRaw.end(), {0x02, 0x00, 0x00, 0x00});
	// Marker (00) and Flag (01) for SegWit
	_raw.insert(_raw.end(), {0x00, 0x01});
	// Number of Inputs using the Variable Length Integer format (up to 65535)
	if (inputs.size() < 0xfd) {
		_raw.insert(_raw.end(), {static_cast<uint8_t>(inputs.size())});
		_legacyRaw.insert(_legacyRaw.end(), {static_cast<uint8_t>(inputs.size())});
	}
	else {
		_raw.insert(_raw.end(), {0xfd, static_cast<uint8_t>(inputs.size() % 256U), static_cast<uint8_t>(inputs.size()/256U)});
		_legacyRaw.insert(_legacyRaw.end(), {0xfd, static_cast<uint8_t>(inputs.size() % 256U), static_cast<uint8_t>(inputs.size()/256U)});
	}
	// Inputs
	for (const auto &input : _inputs) {
		// TxId of the Input, with reversed Endianness
		auto reversedTxId(input.txId);
		// std::vector<uint8_t> inputData;
		std::reverse(reversedTxId.begin(), reversedTxId.end());
		_raw.insert(_raw.end(), reversedTxId.begin(), reversedTxId.end());
		_legacyRaw.insert(_legacyRaw.end(), reversedTxId.begin(), reversedTxId.end());
		// VOut
		std::array<uint8_t, 4> vOut(u32ToA8(input.vOut));
		_raw.insert(_raw.end(), vOut.begin(), vOut.end());
		_legacyRaw.insert(_legacyRaw.end(), vOut.begin(), vOut.end());
		// ScriptSig (Empty)
		_raw.insert(_raw.end(), {0x00});
		_legacyRaw.insert(_legacyRaw.end(), {0x00});
		// Input Sequence (FFFFFFFF)
		_raw.insert(_raw.end(), {0xFF, 0xFF, 0xFF, 0xFF});
		_legacyRaw.insert(_legacyRaw.end(), {0xFF, 0xFF, 0xFF, 0xFF});
		
		// Sign Input
		std::vector<uint8_t> toSign;
		// 1. nVersion of the transaction
		toSign.insert(toSign.end(), {0x02, 0x00, 0x00, 0x00});
		// 2. hashPrevouts
		std::vector<uint8_t> prevouts, hashPrevouts;
		for (const auto &input : _inputs) {
			auto reversedTxId(input.txId);
			std::reverse(reversedTxId.begin(), reversedTxId.end());
			prevouts.insert(prevouts.end(), reversedTxId.begin(), reversedTxId.end());
			std::array<uint8_t, 4> vOut(u32ToA8(input.vOut));
			prevouts.insert(prevouts.end(), vOut.begin(), vOut.end());
		}
		hashPrevouts = sha256sha256(prevouts.data(), prevouts.size());
		toSign.insert(toSign.end(), hashPrevouts.begin(), hashPrevouts.end());
		// 3. hashSequence
		std::vector<uint8_t> sequences, hashSequence;
		for (uint64_t i(0ULL) ; i < _inputs.size() ; i++)
			sequences.insert(sequences.end(), {0xFF, 0xFF, 0xFF, 0xFF});
		hashSequence = sha256sha256(sequences.data(), sequences.size());
		toSign.insert(toSign.end(), hashSequence.begin(), hashSequence.end());
		// 4. outpoint
		std::vector<uint8_t> input2, outpoint;
		{
			auto reversedTxId(input.txId);
			std::reverse(reversedTxId.begin(), reversedTxId.end());
			outpoint.insert(outpoint.end(), reversedTxId.begin(), reversedTxId.end());
			std::array<uint8_t, 4> vOut(u32ToA8(input.vOut));
			outpoint.insert(outpoint.end(), vOut.begin(), vOut.end());
		}
		toSign.insert(toSign.end(), outpoint.begin(), outpoint.end());
		// 5. scriptCode of the input
		std::vector<uint8_t> scriptCode;
		toSign.insert(toSign.end(), {0x19, 0x76, 0xA9});
		toSign.insert(toSign.end(), input.witnessProgram.size());
		toSign.insert(toSign.end(), input.witnessProgram.begin(), input.witnessProgram.end());
		toSign.insert(toSign.end(), {0x88, 0xAC});
		// 6. value of the output spent by this input
		std::array<uint8_t, 8> value(u64ToA8(input.value));
		_inputValue += input.value;
		toSign.insert(toSign.end(), value.begin(), value.end());
		// 7. nSequence of the input
		toSign.insert(toSign.end(), {0xFF, 0xFF, 0xFF, 0xFF});
		// 8. hashOutputs
		std::vector<uint8_t> outputs, hashOutputs;
		for (const auto &output : _outputs) {
			std::array<uint8_t, 8> value(u64ToA8(output.value));
			outputs.insert(outputs.end(), value.begin(), value.end());
			outputs.insert(outputs.end(), {static_cast<uint8_t>(output.witnessProgram.size() + 2U)});
			outputs.insert(outputs.end(), {0x00});
			outputs.insert(outputs.end(), {static_cast<uint8_t>(output.witnessProgram.size())});
			outputs.insert(outputs.end(), output.witnessProgram.begin(), output.witnessProgram.end());
		}
		hashPrevouts = sha256sha256(outputs.data(), outputs.size());
		toSign.insert(toSign.end(), hashPrevouts.begin(), hashPrevouts.end());
		// 9. nLocktime of the transaction
		toSign.insert(toSign.end(), {0x00, 0x00, 0x00, 0x00});
		// 10. sighash type of the signature
		toSign.insert(toSign.end(), {0x01, 0x00, 0x00, 0x00});
		toSign = sha256sha256(toSign.data(), toSign.size());
		std::vector<uint8_t> signature(input.key->sign(toSign));
		std::vector<uint8_t> witness;
		witness.insert(witness.end(), {0x02});
		witness.insert(witness.end(), {static_cast<uint8_t>(signature.size() + 1U)});
		witness.insert(witness.end(), signature.begin(), signature.end());
		witness.insert(witness.end(), {0x01});
		witness.insert(witness.end(), {static_cast<uint8_t>(input.key->getPubKeyCompressed().size())});
		std::vector<uint8_t> pubKey(a8ToV8(input.key->getPubKeyCompressed()));
		witness.insert(witness.end(), pubKey.begin(), pubKey.end());
		witnesses.insert(witnesses.end(), witness.begin(), witness.end());
	}
	
	// Number of Outputs using the Variable Length Integer format (should never be more than 252 in rieWallet)
	_raw.insert(_raw.end(), {static_cast<uint8_t>(_outputs.size())});
	_legacyRaw.insert(_legacyRaw.end(), {static_cast<uint8_t>(_outputs.size())});
	// Outputs
	for (const auto &output : _outputs) {
		std::array<uint8_t, 8> value(u64ToA8(output.value));
		_outputValue += output.value;
		_raw.insert(_raw.end(), value.begin(), value.end());
		_raw.insert(_raw.end(), {static_cast<uint8_t>(output.witnessProgram.size() + 2)});
		_raw.insert(_raw.end(), {0x00});
		_raw.insert(_raw.end(), {static_cast<uint8_t>(output.witnessProgram.size())});
		_raw.insert(_raw.end(), output.witnessProgram.begin(), output.witnessProgram.end());
		_legacyRaw.insert(_legacyRaw.end(), value.begin(), value.end());
		_legacyRaw.insert(_legacyRaw.end(), {static_cast<uint8_t>(output.witnessProgram.size() + 2)});
		_legacyRaw.insert(_legacyRaw.end(), {0x00});
		_legacyRaw.insert(_legacyRaw.end(), {static_cast<uint8_t>(output.witnessProgram.size())});
		_legacyRaw.insert(_legacyRaw.end(), output.witnessProgram.begin(), output.witnessProgram.end());
	}
	_raw.insert(_raw.end(), witnesses.begin(), witnesses.end());
	// Lock Time
	_raw.insert(_raw.end(), {0x00, 0x00, 0x00, 0x00});
	_legacyRaw.insert(_legacyRaw.end(), {0x00, 0x00, 0x00, 0x00});
	_txId = v8ToA8<32>(sha256sha256(_legacyRaw.data(), _legacyRaw.size()));
	std::reverse(_txId.begin(), _txId.end());
	_fee = _inputValue - _outputValue;
	if (_change.has_value())
		_change.value().txId = _txId;
}

void Wallet::loadKeysFromFile(const std::string &path) {
	std::ifstream file(path, std::ios::in | std::fstream::app);
	if (!file)
		throw std::runtime_error("Could not open private keys file");
	std::string line;
	while (std::getline(file, line)) {
		if (!isHexStrOfSize(line, 64))
			throw std::runtime_error("Invalid private key found in "s + path);
		const std::shared_ptr<Key> key(new Key(v8ToA8<32>(hexStrToV8(line))));
		addKey(key);
	}
}

void Wallet::saveKeysToFile(const std::string &path) {
	std::ofstream file(path, std::ios::out);
	if (!file)
		throw std::runtime_error("Could not open private keys file");
	for (const auto &key : _keys)
		file << v8ToHexStr(a8ToV8(key.second->getKey())) << std::endl;
}
	
void Wallet::fetchUtxos() {
	if (_keys.size() > 0ULL) {
		std::vector<Utxo> utxos;
		uint64_t balance(0ULL);
		std::string addresses;
		for (const auto &key : _keys)
			addresses += key.first + ","s;
		addresses.pop_back();
		const nlohmann::json getUtxos(postRequest("https://riecoin.dev/Api/rieWallet.php?method=getUtxos", "addresses="s + addresses));
		if (!getUtxos.contains("result"))
			throw std::runtime_error("Invalid response from server");
		if (!getUtxos.contains("error"))
			throw std::runtime_error("Invalid response from server");
		if (getUtxos["error"] != nullptr)
			throw std::runtime_error(std::string(getUtxos["error"]));
		const nlohmann::json utxosJson(getUtxos["result"]);
		for (const nlohmann::json &utxoJson : utxosJson) {
			const std::string address(utxoJson["address"]);
			const auto keyIt(_keys.find(address));
			if (keyIt == _keys.end())
				throw std::runtime_error("Received UTXO from address not belonging to wallet");
			const std::string txId(utxoJson["txid"]);
			const uint32_t n(utxoJson["n"]);
			const uint64_t value(utxoJson["value"]);
			utxos.push_back(Utxo{v8ToA8<32>(hexStrToV8(txId)), n, {_keys[address]->getWitnessProgram()}, value, _keys[address]});
		}
		std::erase_if( // Check if a transaction went through, taking in account dependencies (probably need some more work)
			_pendingTransactions,
			[&utxos](Transaction pendingTransaction) {
				std::vector<Utxo> inputs(pendingTransaction.getInputs());
				if (pendingTransaction.getChange().has_value())
					utxos.push_back(pendingTransaction.getChange().value());
				return (std::erase_if(
					utxos,
					[&inputs](Utxo utxo) {
						return std::find(inputs.begin(), inputs.end(), utxo) != inputs.end();}) == 0ULL);});
		for (const auto &utxo : utxos)
			balance += utxo.value;
		_balance = balance;
		_utxos = utxos;
	}
}

Transaction Wallet::createTransaction(const std::vector<std::pair<std::string, uint64_t>> &destinations) {
	std::vector<Output> outputs;
	uint64_t amountToSend(0ULL), estimatedSize(12ULL);
	for (const auto &destination : destinations) {
		std::pair<int, std::vector<uint8_t>> witnessProgram(segwit_addr::decode("ric", destination.first));
		if (witnessProgram.first < 0)
			throw std::runtime_error("Invalid address "s + destination.first);
		outputs.push_back(Output{witnessProgram.second, destination.second});
		amountToSend += destination.second;
		estimatedSize += witnessProgram.second.size() + 9ULL;
		if (amountToSend > _balance)
			throw std::runtime_error("Insufficient balance");
	}
	std::vector<Utxo> inputs;
	std::optional<Utxo> change(std::nullopt);
	uint64_t inputAmount(0ULL);
	for (const auto &utxo : _utxos) {
		estimatedSize += 149ULL;
		inputAmount += utxo.value;
		inputs.push_back(utxo);
		if (inputAmount >= amountToSend + 100ULL*estimatedSize + 5000ULL) { // Change Money
			const uint64_t changeAmount(inputAmount - amountToSend - 100ULL*estimatedSize);
			estimatedSize += _keys.begin()->second->getWitnessProgram().size() + 9ULL;
			outputs.push_back(Output{_keys.begin()->second->getWitnessProgram(), changeAmount});
			change = Utxo{{}, static_cast<uint32_t>(outputs.size()) - 1U, {_keys.begin()->second->getWitnessProgram()}, changeAmount, _keys.begin()->second};
			break;
		}
		if (inputAmount >= amountToSend + 100ULL*estimatedSize) // If too small, Change Money goes to the Fee
			break;
	}
	if (inputAmount < amountToSend + 100ULL*estimatedSize)
		throw std::invalid_argument("Insufficient balance to cover the transaction fee");
	return Transaction(inputs, outputs, change);
}

void Wallet::broadcastTransaction(const Transaction &transaction) {
	const nlohmann::json getUtxos(postRequest("https://riecoin.dev/Api/rieWallet.php?method=sendTx", "tx="s + v8ToHexStr(transaction.getRawTransaction())));
	if (!getUtxos.contains("result"))
		throw std::runtime_error("Invalid response from server");
	if (!getUtxos.contains("error"))
		throw std::runtime_error("Invalid response from server");
	if (getUtxos["error"] != nullptr)
		throw std::runtime_error(std::string(getUtxos["error"]));
	_pendingTransactions.push_back(transaction);
	fetchUtxos();
}
