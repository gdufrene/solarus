![Solarus logo](../images/solarus_logo.png)

**MSYS2 BUILDING GUIDE**

[![License](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/copyleft/gpl.html)

---

# About Solarus

Solarus is an open-source adventure 2D game engine written in C++. It can run
games scripted in Lua. This engine is used by our Zelda fangames. Solarus is
licensed under GPL v3.

This document details how to build Solarus in Windows using the MSYS2 environment.

# About MSYS2

From: https://www.msys2.org/

> MSYS2 is a software distro and building platform for Windows.
> At its core is an independent rewrite of MSYS, based on modern Cygwin (POSIX
> compatibility layer) and MinGW-w64 with the aim of better interoperability
> with native Windows software. It provides a bash shell, Autotools, revision
> control systems and the like for building native Windows applications using
> MinGW-w64 toolchains.

From: https://github.com/msys2/msys2/wiki/MSYS2-introduction

> Every subsystem has an associated "shell", which is essentially a set of
> environment variables that allow the subsystems to co-operate properly.

The MSYS2 instaler will create a shortcut for each available shell in
your Start Menu.

* `MSYS`: provides an emulated mostly-POSIX-compliant environment for building
  software, package management, and shell scripting. These programs live in a
  virtual single-root filesystem (the root is the MSYS2 installation directory).
  Some effort is made to have the programs work well with native Windows
  programs, but it's not seamless. This part builds on the Cygwin project.
* `MinGW32` and `MinGW64`: provide native Windows programs and are the main
  focus of the project. These programs are built to co-operate well with other
  Windows programs, independently of the other subsystems. This part builds on
  the MinGW-w64 project.

For setting up MSYS2 and installing packages, you should use the `MSYS` shell.
For building Solarus, you should use the `MinGW32` or `MinGW64` shells,
depending on the desired output binary type (32 or 64 bits).

# Setup MSYS2

1. Install the 32 or 64 bits version from the [website](https://www.msys2.org/)
   according to your **host computer**, not the version you intend to build.
   Despite the name, the 64 bits version of MSYS2 is able to generate both,
   32 and 64 bits binaries.

2. Launch the `MSYS` shell and update the system (the first time you might get a
   freeze here, just close the window, restart the shell and repeat):

       pacman -Syu

3. Restart and re-update until there are no more updates (as in step 2).

4. Install all the necessary packages (32 and 64 bits can co-exist):

       pacman --noconfirm --needed -S \
         git make \
         mingw-w64-i686-gcc mingw-w64-x86_64-gcc \
         mingw-w64-i686-cmake mingw-w64-x86_64-cmake \
         mingw-w64-i686-pkg-config mingw-w64-x86_64-pkg-config \
         mingw-w64-i686-SDL2 mingw-w64-x86_64-SDL2 \
         mingw-w64-i686-SDL2_image mingw-w64-x86_64-SDL2_image \
         mingw-w64-i686-SDL2_ttf mingw-w64-x86_64-SDL2_ttf \
         mingw-w64-i686-openal mingw-w64-x86_64-openal \
         mingw-w64-i686-libvorbis mingw-w64-x86_64-libvorbis \
         mingw-w64-i686-libmodplug mingw-w64-x86_64-libmodplug \
         mingw-w64-i686-glm mingw-w64-x86_64-glm \
         mingw-w64-i686-physfs mingw-w64-x86_64-physfs \
         mingw-w64-i686-luajit mingw-w64-x86_64-luajit \
         mingw-w64-i686-qt5 mingw-w64-x86_64-qt5

   **Note:** Qt dependencies are only required if building the GUI.

8. Clean the package cache:

       pacman --noconfirm -Sc

# Building Solarus

To generate 32 bits binaries use the `MinGW32` shell, for 64 bits binaries use
the `MinGW64` shell in the steps below.

1. Launch the desired `MinGW` shell and clone the repository:

       git clone https://gitlab.com/solarus-games/solarus.git

2. Go into the cloned repository and configure the build:

       cd solarus
       mkdir build
       cd build
       cmake -G "MSYS Makefiles" ..

   **Note:** use `-DSOLARUS_GUI=OFF` to disable building the GUI.

3. Build Solarus (adjust `-j4` for the number of cores you have available):

       make -j4

4. (Optionally) test the built Solarus binaries:

       ctest

Now you should have working `solarus-run.exe` and `gui/solarus-launcher.exe`
binaries in the current build directory. You can invoke them directly from the
`MinGW` shell to start a quest or use the GUI launcher.
