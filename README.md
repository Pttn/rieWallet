# rieWallet

![Riecoin Logo](https://riecoin.dev/files/w/thumb.php?f=Riecoin.svg&width=128)

rieWallet is a light and simple Riecoin wallet, which allows to receive and send Riecoins very easily without any setup or blockchain download. This is great for Riecoin beginners or people that don't have a powerful computer. Though, we encourage people to use Riecoin Core and leave it open in order to support the Riecoin network and access more features.

**rieWallet is experimental software and should for now only be used for small transactions**, though as long as you backup your keys.txt file, there will always be a way to recover funds if something goes wrong.

Find the latest binaries [here](https://riecoin.dev/resources/Pttn/rieWallet.php) for Linux and Windows.

This README is intended for advanced users and will mainly only give information for developers like how to compile or contribute. The use of rieWallet should be intuitive, but if you encounter issues, you may get support in the [Riecoin Forum](https://forum.riecoin.dev/).

## Minimum requirements

* Windows Vista or recent enough Linux;
* Virtually any usual 32 or 64 bits CPU (should work for any x86 since Pentium Pro and recent ARMs);
* We only provide binaries for Windows and Linux x64. In the other cases, you must have access to an appropriate build environment and compile yourself the code;
* Some dependencies may be required for Linux, you should be able to figure out which ones.

## Compile this program

Compilation should work fine for the systems below. If not, you are welcomed to report issues, however do not make reports when using an old OS.

### Debian/Ubuntu

You can compile this C++ program with g++, as, m4 and make, install them if needed. Then, get if needed the following dependencies:

* [libSSL](https://www.openssl.org/)
* [cURL](https://curl.haxx.se/)
* [NLohmann Json](https://json.nlohmann.me/)
* [FLTK 1.3](https://www.fltk.org/)

On a recent enough Debian or Ubuntu, you can easily install these by doing as root:

```bash
apt install g++ make git libssl-dev libcurl4-openssl-dev nlohmann-json3-dev libfltk1.3-dev
```

Then, download the source files, go/`cd` to the directory:

```bash
git clone https://github.com/Pttn/rieWallet.git
cd rieWallet
```

Finally, make the executable:

```bash
make
```

For other Linux, executing equivalent commands (using `pacman` instead of `apt`,...) should work.

#### Static building

rieWallet can be built statically, in order to have a binary that can be distributed. However, some libraries like X11 or GLibC still have to be linked dynamically so rieWallet may fail to run on very old Linux distributions or need some libraries to be manually installed. For static building, you may need to install

```bash
apt install libxft-dev
```

A script that retrieves the dependencies' source codes from Riecoin.dev and compile them is provided. If you use it, you should first ensure that you can decompress `tar.xz` files using `tar`. If you already built the dependencies once, you can usually reuse existing `incs` and `libs` folders and skip this step, though the dependencies may be updated once a while on Riecoin.dev.

Run the script with

```bash
sh GetDependencies.sh build
```

Then wait for the download and compilation to finish. The script must not have been interrupted by an error and the `incs` and `libs` folders must have appeared. Then, use

```bash
make static
```

### Windows x64 or x86

You can compile rieWallet on Windows, and here is one way to do this. First, install [MSYS2](http://www.msys2.org/) (follow the instructions on the website).

Then, for x64, enter in the MSYS **MinGW 64** console, and install the tools and dependencies:

```bash
pacman -S make m4 git
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-curl
pacman -S mingw-w64-x86_64-nlohmann-json
pacman -S mingw-w64-x86_64-fltk
```

Note that you must install the `mingw-w64-x86_64-...` packages and not just `gcc` or `curl`. Some dependencies are already included in others, for example GCC includes OpenSSL.

If building for x86, enter instead in the **MinGW 32** console and use the `mingw-w64-i686-...` prefix for the packages.

Clone rieWallet with `git`, go to its directory with `cd`, and compile with `make` (same commands as Linux, see above).

#### Static building for x64

The produced executable will only run in the MSYS console, or if all the needed DLLs are next to the executable. To obtain a standalone executable, you need to link statically the dependencies. For this, the static building instructions above also work in the MSYS console.

```bash
sh GetDependencies.sh build
make static
```

## Developers and license

* [Pttn](https://forum.riecoin.dev/memberlist.php?mode=viewprofile&u=2), author and maintainer.

Reach us in the [Riecoin Forum](https://forum.riecoin.dev/)!

This work is released under the MIT license.

### Versioning

The version naming scheme is 0.9, 0.99, 0.999 and so on for major versions, analogous to 1.0, 2.0, 3.0,.... The first non 9 decimal digit is minor, etc. For example, the version 0.9925a can be thought as 2.2.5a. A perfect bug-free software will be version 1. No precise criteria have been decided about incrementing major or minor versions for now.

## Contributing

Feel free to do a pull request or open an issue, and I will review it. I am open for adding new features, but I also wish to keep this project minimalist. Any useful contribution will be welcomed.

By contributing to rieWallet, you accept to place your code in the MIT license.

Donations to the Riecoin Project are welcome:

* Riecoin: ric1qr3yxckxtl7lacvtuzhrdrtrlzvlydane2h37ja
* Bitcoin: bc1qr3yxckxtl7lacvtuzhrdrtrlzvlydaneqela0u

Also star the repositories of Riecoin software, that would be appreciated!

## Resources

* [Riecoin website](https://Riecoin.dev/)
  * [rieWallet's page](https://riecoin.dev/en/rieWallet)
