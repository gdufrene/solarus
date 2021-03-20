#!/bin/bash
LIB=""

function targetLibs() {
	install_name_tool -change $1 @rpath/libs/$(basename $1) $LIB
}

function copylib() {
	cp $1 libs
	LIB="libs/$(basename $1)"
	chmod u+w $LIB
	targetLibs $1 libs/$(basename $1)
}


LIB=libsolarus.1.dylib
targetLibs @rpath/libsolarus.1.dylib
targetLibs /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib
targetLibs /usr/local/opt/sdl2_image/lib/libSDL2_image-2.0.0.dylib
targetLibs /usr/local/opt/sdl2_ttf/lib/libSDL2_ttf-2.0.0.dylib
targetLibs /usr/local/opt/sdl2_net/lib/libSDL2_net-2.0.0.dylib
targetLibs /usr/lib/libsqlite3.dylib
targetLibs /usr/local/opt/luajit/lib/libluajit-5.1.2.dylib
targetLibs /usr/local/opt/physfs/lib/libphysfs.1.dylib
targetLibs /usr/local/opt/libvorbis/lib/libvorbis.0.dylib
targetLibs /usr/local/opt/libvorbis/lib/libvorbisfile.3.dylib
targetLibs /usr/local/opt/libogg/lib/libogg.0.dylib
targetLibs /usr/local/opt/libmodplug/lib/libmodplug.1.dylib

LIB=solarus-run
targetLibs @rpath/libsolarus.1.dylib
targetLibs /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib
targetLibs /usr/local/opt/sdl2_image/lib/libSDL2_image-2.0.0.dylib
targetLibs /usr/local/opt/sdl2_ttf/lib/libSDL2_ttf-2.0.0.dylib
targetLibs /usr/local/opt/sdl2_net/lib/libSDL2_net-2.0.0.dylib
targetLibs /usr/lib/libsqlite3.dylib
targetLibs /usr/local/opt/luajit/lib/libluajit-5.1.2.dylib
targetLibs /usr/local/opt/physfs/lib/libphysfs.1.dylib
targetLibs /usr/local/opt/libvorbis/lib/libvorbis.0.dylib
targetLibs /usr/local/opt/libvorbis/lib/libvorbisfile.3.dylib
targetLibs /usr/local/opt/libogg/lib/libogg.0.dylib
targetLibs /usr/local/opt/libmodplug/lib/libmodplug.1.dylib

