// (c) 2022 Pttn (https://riecoin.dev/en/rieWallet)

#include "Fltk.hpp"

WalletWindow::WalletWindow(int W, int H, const char *T, const std::shared_ptr<Wallet> &wallet) : Fl_Window(W, H, T), _wallet(wallet), _balance(0.), _priceBTC(0.), _priceUSD(0.) {
	Fl_Menu_Item menuitems[] = {
		{"File", 0, 0, 0, FL_SUBMENU, 0, 0, 0, 0},
			{"Exit", 0, [](Fl_Widget*, void*) {exit(0);}, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0},
		{"About", 0, [](Fl_Widget*, void*) {fl_message("%s", (std::string(walletVersionString) + " by Pttn, light and simple Riecoin Wallet\nhttps://Riecoin.dev/en/rieWallet\n\nThis is experimental software, consider using Riecoin Core\nfor important transactions or more features.\n\nLibraries: OpenSSL, NLohmann-Json, FLTK\nBuilt with G++\nReleased under the MIT License").c_str());}, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	_menu = std::unique_ptr<Fl_Menu_Bar>(new Fl_Menu_Bar(1, 0, W, 24));
	_menu->box(FL_NO_BOX);
	_menu->copy(menuitems, this);
	
	_tabs = std::unique_ptr<Fl_Tabs>(new Fl_Tabs(8, 32, W - 16, H - 176));
	_tabs->box(FL_ENGRAVED_BOX);
		_walletTab = std::unique_ptr<Fl_Group>(new Fl_Group(8, _menu->h() + 32, w() - 16, h() - _menu->h() - 16, "Send Money"));
			_availableText = std::unique_ptr<Fl_Box>(new Fl_Box(16, 72, 108, 24, "Available RIC - Only confirmed balance is shown, it may take minutes to update"));
			_availableText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			_balanceOutput = std::unique_ptr<Fl_Output>(new Fl_Output(32, 96, 576, 64));
			_balanceOutput->box(FL_ENGRAVED_BOX);
			_balanceOutput->color(fl_rgb_color(16U, 16U, 16U));
			_balanceOutput->textsize(48);
			_balanceOutput->value("0.00000000");
			_worthText = std::unique_ptr<Fl_Box>(new Fl_Box(24, _balanceOutput->y() + _balanceOutput->h() + 4, 480, 24, "~0.00000000 BTC / ~0.000 USD"));
			_worthText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			_sendGroup = std::unique_ptr<Fl_Group>(new Fl_Group(16, _worthText->y() + _worthText->h() + 24, W - 32, 108, "Send Riecoins to a Bech32 Address"));
			_sendGroup->box(FL_ENGRAVED_BOX);
				_sendText = std::unique_ptr<Fl_Box>(new Fl_Box(32, _worthText->y() + _worthText->h() + 48, 48, 24, "Send"));
				_sendText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				_amountToSend = std::unique_ptr<Fl_Input>(new Fl_Input(_sendText->x() + _sendText->w() + 4, _sendText->y(), 160, 24, ""));
				_amountToSend->color(fl_rgb_color(16U, 16U, 16U));
				_amountToSend->box(FL_ENGRAVED_BOX);
				_amountToSend->callback([](Fl_Widget*, void* window) {WalletWindow::showFee(nullptr, window);}, this);
				_amountToSend->when(FL_WHEN_CHANGED);
				_ricText = std::unique_ptr<Fl_Box>(new Fl_Box(_amountToSend->x() + _amountToSend->w() + 4, _amountToSend->y(), 32, 24, "RIC"));
				_ricText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				_feeText = std::unique_ptr<Fl_Box>(new Fl_Box(_ricText->x() + _ricText->w() + 32, _ricText->y(), 384, 24, "Note: balance must cover amount + fee"));
				_feeText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				_toAddressText = std::unique_ptr<Fl_Box>(new Fl_Box(32, _amountToSend->y() + _amountToSend->h() + 16, 24, 24, "to"));
				_toAddressText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				_destinationAddress = std::unique_ptr<Fl_Input>(new Fl_Input(_toAddressText->x() + _toAddressText->w() + 4, _toAddressText->y(), 384, 24, ""));
				_destinationAddress->color(fl_rgb_color(16U, 16U, 16U));
				_destinationAddress->box(FL_ENGRAVED_BOX);
				_destinationAddress->callback([](Fl_Widget*, void* window) {WalletWindow::showFee(nullptr, window);}, this);
				_destinationAddress->when(FL_WHEN_CHANGED);
				_sendButton = std::unique_ptr<Fl_Button>(new Fl_Button(_destinationAddress->x() + _destinationAddress->w() + 4, _destinationAddress->y(), 96, 24, "Send"));
				_sendButton->box(FL_ENGRAVED_BOX);
				_sendButton->callback([](Fl_Widget*, void* window) {WalletWindow::sendMoney(nullptr, window);}, this);
			_sendGroup->end();
		_walletTab->end();
		_addressesTab = std::unique_ptr<Fl_Group>(new Fl_Group(8, _menu->h() + 32, w() - 16, h() - _menu->h() - 16, "Receiving Addresses"));
			_addressesText = std::unique_ptr<Fl_Box>(new Fl_Box(16, 64, 480, 24, "These are your Riecoin addresses."));
			_addressesText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			_generateAddressButton = std::unique_ptr<Fl_Button>(new Fl_Button(W - 256, _addressesText->y(), 240, 24, "Generate new address"));
			_generateAddressButton->box(FL_ENGRAVED_BOX);
			_generateAddressButton->callback([](Fl_Widget*, void* window) {WalletWindow::addAddress(nullptr, window);}, this);
			_addressList = std::unique_ptr<Fl_Browser>(new Fl_Browser(16, _addressesText->y() + _addressesText->h() + 4, W - 32, 128, ""));
			_addressList->type(FL_HOLD_BROWSER);
			_addressList->box(FL_ENGRAVED_BOX);
			_addressList->color(fl_rgb_color(16U, 16U, 16U));
			_addressList->callback([](Fl_Widget* addressList, void* window) {WalletWindow::updateSelectedAddress(addressList, window);}, this);
			_addressCopyable = std::unique_ptr<Fl_Output>(new Fl_Output(16, _addressList->y() + _addressList->h() + 4, W - 32, 24));
			_addressCopyable->box(FL_ENGRAVED_BOX);
			_addressCopyable->color(fl_rgb_color(16U, 16U, 16U));
			_showPrivateKeyButton = std::unique_ptr<Fl_Button>(new Fl_Button(_addressCopyable->x(), _addressCopyable->y() + _addressCopyable->h() + 4, 128, 24, "Show Private Key"));
			_showPrivateKeyButton->box(FL_ENGRAVED_BOX);
			_privateKeyOutput = std::unique_ptr<Fl_Output>(new Fl_Output(_showPrivateKeyButton->x() + _showPrivateKeyButton->w() + 4, _showPrivateKeyButton->y(), W - _showPrivateKeyButton->x() - _showPrivateKeyButton->w() - 20, 24));
			_showPrivateKeyButton->callback([](Fl_Widget*, void* window) {WalletWindow::showPrivateKey(nullptr, window);}, this);
			_privateKeyOutput->box(FL_ENGRAVED_BOX);
			_privateKeyOutput->color(fl_rgb_color(16U, 16U, 16U));
			_privateKeyInput = std::unique_ptr<Fl_Input>(new Fl_Input(_showPrivateKeyButton->x(), _privateKeyOutput->y() + _privateKeyOutput->h() + 28, W - 180, 24));
			_privateKeyInput->box(FL_ENGRAVED_BOX);
			_privateKeyInput->color(fl_rgb_color(16U, 16U, 16U));
			_importPrivateKeyButton = std::unique_ptr<Fl_Button>(new Fl_Button(_privateKeyInput->x() + _privateKeyInput->w() + 4, _privateKeyInput->y(), 144, 24, "Import Private Key"));
			_importPrivateKeyButton->box(FL_ENGRAVED_BOX);
			_importPrivateKeyButton->callback([](Fl_Widget*, void* window) {WalletWindow::importPrivateKey(nullptr, window);}, this);
		_addressesTab->end();
	_tabs->end();
	
	_logBuffer = std::unique_ptr<Fl_Text_Buffer>(new Fl_Text_Buffer());
	_logTd = std::unique_ptr<Fl_Text_Display>(new Fl_Text_Display(8, _tabs->y() + _tabs->h() + 8, W - 16, 128, ""));
	_logTd->wrap_mode(Fl_Text_Editor::WRAP_AT_BOUNDS, 250);
	_logTd->buffer(_logBuffer.get());
	_logTd->box(FL_ENGRAVED_BOX);
	_logTd->color(fl_rgb_color(16U, 16U, 16U));
	
	log("Welcome to "s + std::string(walletVersionString) + "\n"s);
	log("Learn more in its project page: https://riecoin.dev/en/rieWallet\n");
	log("Get support in the Riecoin Forum: https://forum.riecoin.dev\n\n");
	try {
		_wallet->loadKeysFromFile(keysFile);
	}
	catch (const std::exception &e) {
		log("Could not access wallet file: "s + e.what() + "\n");
		log("rieWallet should be closed and the issue investigated\n");
	}
	updateAddresses();
	if (_addresses.size() == 0) {
		log("The wallet is empty. Generate a Riecoin address and use it to receive money.\n");
		log("The "s + keysFile + " file contains your private keys, never share it to anyone and make backups of this file after generating addresses.\n"s);
	}
}

void WalletWindow::updateBalance() {
	try {
		_wallet->fetchUtxos();
		_balance = static_cast<double>(_wallet->getBalance())/1e8;
		std::ostringstream oss;
		oss << FIXED(8) << _balance;
		_balanceOutput->value(oss.str().c_str());
	}
	catch (const std::exception &e) {
		log("Could not fetch UTXOs: "s + e.what() + "\n"s);
		log("This can happen once a while and can usually be ignored, check your connection if this happens often\n"s);
	}
}
void WalletWindow::updateWorth() {
	try {
		const nlohmann::json getPrices(postRequest("https://riecoin.dev/Api/rieWallet.php?method=getPrices", ""));
		if (!getPrices.contains("result"))
			throw std::runtime_error("Invalid response from server");
		if (!getPrices.contains("error"))
			throw std::runtime_error("Invalid response from server");
		if (getPrices["error"] != nullptr)
			throw std::runtime_error(std::string(getPrices["error"]));
		const nlohmann::json pricesJson(getPrices["result"]);
		_priceBTC = pricesJson["priceBTC"];
		_priceUSD = pricesJson["priceUSD"];
		std::ostringstream oss;
		oss << "~" << FIXED(8) << _balance*_priceBTC << " BTC / ~" << FIXED(3) << _balance*_priceUSD << " USD";
		_worthStr = oss.str();
		_worthText->label(_worthStr.c_str());
	}
	catch (const std::exception &e) {
		log("Could not fetch Riecoin price: "s + e.what() + "\n"s);
		log("This can happen once a while and can usually be ignored, check your connection if this happens often\n"s);
	}
}

void WalletWindow::showFee(Fl_Widget*, void* window) {
	try {
		const std::string address(reinterpret_cast<WalletWindow*>(window)->_destinationAddress->value());
		const uint64_t amount(std::stod(reinterpret_cast<WalletWindow*>(window)->_amountToSend->value())*1e8);
		const Transaction transaction(reinterpret_cast<WalletWindow*>(window)->_wallet->createTransaction({{address, amount}}));
		std::ostringstream oss;
		oss << "+ fee of " << static_cast<double>(transaction.getFee())/1e8 << " RIC";
		reinterpret_cast<WalletWindow*>(window)->_feeStr = oss.str();
		reinterpret_cast<WalletWindow*>(window)->_feeText->label(reinterpret_cast<WalletWindow*>(window)->_feeStr.c_str());
	}
	catch (const std::exception &e) {
		reinterpret_cast<WalletWindow*>(window)->_feeText->label("Note: balance must cover amount + fee");
	}
}

void WalletWindow::sendMoney(Fl_Widget*, void* window) {
	uint64_t amount(0ULL);
	try {
		amount = std::stod(reinterpret_cast<WalletWindow*>(window)->_amountToSend->value())*1e8;
		try {
			if (amount < 1000ULL)
				throw std::invalid_argument("Amount is too small");
			const std::string address(reinterpret_cast<WalletWindow*>(window)->_destinationAddress->value());
			const Transaction transaction(reinterpret_cast<WalletWindow*>(window)->_wallet->createTransaction({{address, amount}}));
			reinterpret_cast<WalletWindow*>(window)->_wallet->broadcastTransaction(transaction);
			reinterpret_cast<WalletWindow*>(window)->log("Sent "s + std::to_string(static_cast<double>(amount)/1e8) + " RIC to "s + address + "\n"s);
			reinterpret_cast<WalletWindow*>(window)->log("TxId: "s + v8ToHexStr(a8ToV8(transaction.getTxId())) + "\n"s);
			reinterpret_cast<WalletWindow*>(window)->updateBalance();
			reinterpret_cast<WalletWindow*>(window)->updateWorth();
		}
		catch (const std::exception &e) {
			reinterpret_cast<WalletWindow*>(window)->log("Could not send transaction: "s + e.what() + "\n"s);
		}
	}
	catch (const std::exception &e) {
		reinterpret_cast<WalletWindow*>(window)->log("Could not send transaction: invalid amount\n"s);
	}
}

void WalletWindow::updateSelectedAddress(Fl_Widget* browser, void* window) {
	if (dynamic_cast<Fl_Browser*>(browser)->value() > 0 && dynamic_cast<Fl_Browser*>(browser)->value() <= static_cast<int>(reinterpret_cast<WalletWindow*>(window)->_addresses.size())) // Index starts at 1
		reinterpret_cast<WalletWindow*>(window)->_addressCopyable->value(reinterpret_cast<WalletWindow*>(window)->_addresses[dynamic_cast<Fl_Browser*>(browser)->value() - 1].c_str());
	else
		reinterpret_cast<WalletWindow*>(window)->_addressCopyable->value("");
	reinterpret_cast<WalletWindow*>(window)->_privateKeyOutput->value("");
}
void WalletWindow::addAddress(Fl_Widget*, void* window) {
	try {
		const std::shared_ptr<Key> key(new Key());
		reinterpret_cast<WalletWindow*>(window)->_wallet->addKey(key);
		reinterpret_cast<WalletWindow*>(window)->updateAddresses();
		reinterpret_cast<WalletWindow*>(window)->log("Created "s + segwit_addr::encode("ric", 0, key->getWitnessProgram()) + "\n"s);
		try {
			reinterpret_cast<WalletWindow*>(window)->_wallet->saveKeysToFile(keysFile);
		}
		catch (const std::exception &e) {
			reinterpret_cast<WalletWindow*>(window)->log("Could not save the wallet: "s + e.what() + "\n"s);
		}
	}
	catch (const std::exception &e) {
		reinterpret_cast<WalletWindow*>(window)->log("Could not generate address: "s + e.what() + "\n"s);
	}
	reinterpret_cast<WalletWindow*>(window)->_addressCopyable->value("");
	reinterpret_cast<WalletWindow*>(window)->_privateKeyOutput->value("");
}
void WalletWindow::showPrivateKey(Fl_Widget*, void* window) {
	if (reinterpret_cast<WalletWindow*>(window)->_addressList->value() > 0 && reinterpret_cast<WalletWindow*>(window)->_addressList->value() <= static_cast<int>(reinterpret_cast<WalletWindow*>(window)->_addresses.size())) { // Index starts at 1
		try {
			reinterpret_cast<WalletWindow*>(window)->_privateKeyOutput->value(
				v8ToHexStr(a8ToV8(reinterpret_cast<WalletWindow*>(window)->_wallet->getKey(reinterpret_cast<WalletWindow*>(window)->_addresses[reinterpret_cast<WalletWindow*>(window)->_addressList->value() - 1])->getKey())).c_str());
		}
		catch (const std::exception &e) {
			reinterpret_cast<WalletWindow*>(window)->log("Could not get private key: "s + e.what() + "\n"s);
		}
	}
}
void WalletWindow::importPrivateKey(Fl_Widget* , void* window) {
	const std::string privateKeyHex(reinterpret_cast<WalletWindow*>(window)->_privateKeyInput->value());
	if (!isHexStrOfSize(privateKeyHex, 64)) {
		reinterpret_cast<WalletWindow*>(window)->log("Invalid private key\n");
	}
	else {
		try {
			std::shared_ptr<Key> key(new Key(v8ToA8<32>(hexStrToV8(privateKeyHex))));
			reinterpret_cast<WalletWindow*>(window)->_wallet->addKey(key);
			reinterpret_cast<WalletWindow*>(window)->updateAddresses();
			reinterpret_cast<WalletWindow*>(window)->log("Imported "s + segwit_addr::encode("ric", 0, key->getWitnessProgram()) + "\n"s);
			reinterpret_cast<WalletWindow*>(window)->_privateKeyInput->value("");
			try {
				reinterpret_cast<WalletWindow*>(window)->_wallet->saveKeysToFile(keysFile);
			}
			catch (const std::exception &e) {
				reinterpret_cast<WalletWindow*>(window)->log("Could not save the wallet: "s + e.what() + "\n"s);
			}
		}
		catch (const std::exception &e) {
			reinterpret_cast<WalletWindow*>(window)->log("Could not import address: "s + e.what() + "\n"s);
		}
	}
	reinterpret_cast<WalletWindow*>(window)->_privateKeyOutput->value("");
	reinterpret_cast<WalletWindow*>(window)->updateBalance();
	reinterpret_cast<WalletWindow*>(window)->updateWorth();
}
