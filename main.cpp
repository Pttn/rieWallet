// (c) 2022 Pttn (https://riecoin.dev/en/rieWallet)

#include <thread>

#include "Wallet.hpp"
#include "Gui/Fltk.hpp"

void walletThread(const std::shared_ptr<WalletWindow> &window) {
	auto nextUpdateTp(std::chrono::steady_clock::now());
	while (true) {
		if (std::chrono::steady_clock::now() >= nextUpdateTp) {
			Fl::lock();
			window->updateBalance();
			window->updateWorth();
			Fl::unlock();
			Fl::awake();
			nextUpdateTp = std::chrono::steady_clock::now() + std::chrono::seconds(30);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

int main() {
	Wallet wallet;
	Fl::background(32U, 32U, 32U);
	Fl::foreground(224U, 224U, 224U);
	std::shared_ptr<WalletWindow> window(std::make_shared<WalletWindow>(640, 480, "rieWallet", std::make_shared<Wallet>(wallet)));
	Fl::lock();
	window->show();
	std::thread thread(walletThread, window);
    thread.detach();
	return(Fl::run());
}
