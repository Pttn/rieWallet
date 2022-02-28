// (c) 2022 Pttn (https://riecoin.dev/en/rieWallet)

#ifndef HEADER_Fltk_hpp
#define HEADER_Fltk_hpp

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Value_Output.H>
#include <FL/Fl_Window.H>

#include "../main.hpp"
#include "../Wallet.hpp"

class WalletWindow : public Fl_Window {
	std::shared_ptr<Wallet> _wallet;
	
	std::unique_ptr<Fl_Menu_Bar> _menu;
	std::unique_ptr<Fl_Tabs> _tabs;
	
	std::unique_ptr<Fl_Group> _walletTab;
	std::unique_ptr<Fl_Box> _availableText;
	double _balance;
	std::unique_ptr<Fl_Output> _balanceOutput;
	double _priceBTC, _priceUSD;
	std::string _worthStr;
	std::unique_ptr<Fl_Box> _worthText;
	std::unique_ptr<Fl_Group> _sendGroup;
	std::unique_ptr<Fl_Box> _sendText;
	std::unique_ptr<Fl_Input> _amountToSend;
	std::unique_ptr<Fl_Box> _ricText;
	std::string _feeStr;
	std::unique_ptr<Fl_Box> _feeText;
	std::unique_ptr<Fl_Box> _toAddressText;
	std::unique_ptr<Fl_Input> _destinationAddress;
	std::unique_ptr<Fl_Button> _sendButton;
	
	std::unique_ptr<Fl_Group> _addressesTab;
	std::vector<std::string> _addresses;
	std::unique_ptr<Fl_Box> _addressesText;
	std::unique_ptr<Fl_Button> _generateAddressButton;
	std::unique_ptr<Fl_Browser> _addressList;
	std::unique_ptr<Fl_Output> _addressCopyable;
	std::unique_ptr<Fl_Button> _showPrivateKeyButton;
	std::unique_ptr<Fl_Output> _privateKeyOutput;
	std::unique_ptr<Fl_Button> _importPrivateKeyButton;
	std::unique_ptr<Fl_Input> _privateKeyInput;
	
	std::string _logStr;
	std::unique_ptr<Fl_Text_Buffer> _logBuffer;
	std::unique_ptr<Fl_Text_Display> _logTd;
public:
	WalletWindow(int, int, const char*, const std::shared_ptr<Wallet>&);
	
	void updateAddresses() {
		_addressList->clear();
		const std::vector<std::string> addresses(_wallet->getAddresses());
		_addresses = addresses;
		for (const std::string &address : _addresses)
			_addressList->add(address.c_str());
	}
	void updateBalance();
	void updateWorth();
	void log(const std::string &message) {
		_logStr += message;
		_logBuffer->append(message.c_str());
		_logTd->insert_position(_logStr.size());
		_logTd->show_insert_position();
	}
	
	static void showFee(Fl_Widget*, void* window);
	static void sendMoney(Fl_Widget*, void* window);
	static void updateSelectedAddress(Fl_Widget* browser, void* window);
	static void addAddress(Fl_Widget*, void* window);
	static void showPrivateKey(Fl_Widget*, void* window);
	static void importPrivateKey(Fl_Widget* , void* window);
};

#endif
