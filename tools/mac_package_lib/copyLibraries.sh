#!/bin/bash
LIB=""

function targetLibs() {
	install_name_tool -change $1 @rpath/libs/$(basename $1) "$LIB"
}

function copylib() {
	cp $1 libs
	LIB="libs/$(basename $1)"
	chmod u+w $LIB
	targetLibs $1 libs/$(basename $1)
}

test ! -d libs && mkdir libs

copylib /usr/lib/libz.1.dylib

copylib /usr/local/opt/libpng/lib/libpng16.16.dylib
targetLibs /usr/lib/libz.1.dylib

copylib /usr/local/opt/jpeg/lib/libjpeg.9.dylib

copylib /usr/local/opt/libtiff/lib/libtiff.5.dylib
targetLibs /usr/local/opt/jpeg/lib/libjpeg.9.dylib 
targetLibs /usr/lib/libz.1.dylib 

copylib /usr/local/opt/webp/lib/libwebp.7.dylib

copylib /usr/local/opt/sdl2_net/lib/libSDL2_net-2.0.0.dylib
targetLibs /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib 

copylib /usr/lib/libbz2.1.0.dylib

copylib /usr/local/opt/freetype/lib/libfreetype.6.dylib
targetLibs /usr/lib/libz.1.dylib 
targetLibs /usr/lib/libbz2.1.0.dylib 
targetLibs /usr/local/opt/libpng/lib/libpng16.16.dylib 

copylib /usr/local/opt/sdl2_ttf/lib/libSDL2_ttf-2.0.0.dylib
targetLibs /usr/local/opt/freetype/lib/libfreetype.6.dylib 
targetLibs /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib 

copylib /usr/local/opt/luajit/lib/libluajit-5.1.2.dylib

copylib /usr/local/opt/libmodplug/lib/libmodplug.1.dylib

copylib /usr/local/opt/libogg/lib/libogg.0.dylib

copylib /usr/local/opt/physfs/lib/libphysfs.1.dylib

copylib /usr/local/opt/libvorbis/lib/libvorbis.0.dylib
targetLibs /usr/local/opt/libogg/lib/libogg.0.dylib 

copylib /usr/local/opt/libvorbis/lib/libvorbisfile.3.dylib
targetLibs /usr/local/Cellar/libvorbis/1.3.7/lib/libvorbis.0.dylib
targetLibs /usr/local/opt/libogg/lib/libogg.0.dylib

copylib /usr/local/opt/sdl2_image/lib/libSDL2_image-2.0.0.dylib
targetLibs /usr/local/opt/libpng/lib/libpng16.16.dylib
targetLibs /usr/local/opt/jpeg/lib/libjpeg.9.dylib
targetLibs /usr/local/opt/libtiff/lib/libtiff.5.dylib
targetLibs /usr/lib/libz.1.dylib
targetLibs /usr/local/opt/webp/lib/libwebp.7.dylib
targetLibs /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib

copylib /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib


echo "---"
echo "Prepare libs Done."
echo "---"

gamepath="$HOME/Downloads/Solarus Launcher.app"

test ! -d "$gamepath/Contents/Frameworks/libs" && mkdir "$gamepath/Contents/Frameworks/libs"
cp libs/* "$gamepath/Contents/Frameworks/libs/"

LIB="$gamepath/Contents/Resources/solarus-launcher"
test ! -f $(basename "$LIB.old") && cp "$LIB" $(basename "$LIB.old")
targetLibs @rpath/libsolarus.1.dylib
install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @rpath/libs/libSDL2-2.0.0.dylib "$LIB"
install_name_tool -change  @rpath/SDL2_image.framework/Versions/A/SDL2_image @rpath/libs/libSDL2_image-2.0.0.dylib "$LIB"
install_name_tool -change  @rpath/SDL2_ttf.framework/Versions/A/SDL2_ttf @rpath/libs/libSDL2_ttf-2.0.0.dylib "$LIB"
install_name_tool -change  @rpath/libluajit-5.1.2.0.4.dylib @rpath/libs/libluajit-5.1.2.dylib "$LIB"
targetLibs @rpath/libphysfs.1.dylib
install_name_tool -change  @rpath/Vorbis.framework/Versions/A/Vorbis @rpath/libs/libvorbis.0.dylib "$LIB"
install_name_tool -change  @rpath/Ogg.framework/Versions/A/Ogg @rpath/libs/libogg.0.dylib "$LIB"
targetLibs @rpath/libmodplug.1.dylib 

LIB="$gamepath/Contents/Frameworks/libsolarus-gui.1.dylib"
test ! -f $(basename "$LIB.old") && cp "$LIB" $(basename "$LIB.old")
targetLibs @rpath/libsolarus.1.dylib
install_name_tool -change  @rpath/SDL2.framework/Versions/A/SDL2 @rpath/libs/libSDL2-2.0.0.dylib "$LIB"
install_name_tool -change  @rpath/SDL2_image.framework/Versions/A/SDL2_image @rpath/libs/libSDL2_image-2.0.0.dylib "$LIB"
install_name_tool -change  @rpath/SDL2_ttf.framework/Versions/A/SDL2_ttf @rpath/libs/libSDL2_ttf-2.0.0.dylib "$LIB"
install_name_tool -change  @rpath/libluajit-5.1.2.0.4.dylib @rpath/libs/libluajit-5.1.2.dylib "$LIB"
targetLibs @rpath/libphysfs.1.dylib
install_name_tool -change  @rpath/Vorbis.framework/Versions/A/Vorbis @rpath/libs/libvorbis.0.dylib "$LIB"
install_name_tool -change  @rpath/Ogg.framework/Versions/A/Ogg @rpath/libs/libogg.0.dylib "$LIB"
targetLibs @rpath/libmodplug.1.dylib 

cp ../../build/solarus-run ../../build/libsolarus.1.dylib .

./fixBuild.sh

cp solarus-run "$gamepath/Contents/Resources/"
find "$gamepath/Contents/Frameworks" -name "libsolarus.1.dylib" -delete
cp libsolarus.1.dylib "$gamepath/Contents/Frameworks/libs/"

echo "---"
echo "Prepare App Done."
echo "---"

current=$(pwd)
cd "$gamepath/.."
zip -yr "$current/solarus-macosx64.zip" "$(basename "$gamepath")"
cd "$current"

echo "---"
echo "Package done "
echo "---"

ls -alh solarus-macosx64.zip

