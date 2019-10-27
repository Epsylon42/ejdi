# ejdi programming language

It's a C++ rewrite of my old project. I don't remember where the name comes from.

The language itself is more-or-less usable, but there's currenly no way to interact with the system except for the `print` function.

`test.ejdi` has an example program with generates and prints an array from 2 to 11.

## building

ejdi requires cmake and a c++ compiler with c++17 support (tested with g++ 9.2.1).

Building with cmake requires internet connection and will download some cmake code into your `$HOME/.cppm` and `$HOME/.hunter`. If you don't want this, build ejdi manually with `g++ -std=c++17 -Iinclude src/* -o ejdi`

## running

The first command line argument is a file which should be run
