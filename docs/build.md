# Build

## Requirements
 - [Qt 6.10.2](https://www.qt.io/blog/qt-6.10.2-released)
 - cmake, gcc
 - optionally on linux [appimagetool-x86_64.appimage](https://github.com/AppImage/appimagetool)

## Linux build

Everything is managed with the ./build.sh script:
```
# build application in ./build/src/Release/qmsc
./build.sh release
# create appimage in ./dist/qmsc.AppImage
./build.sh release install
```

## Windows build

On Windows, install Qt/Qt-creator and build with it.

Alternatively, on linux, you can use docker build image [stateoftheartio](https://hub.docker.com/r/stateoftheartio/qt6) :
```
docker run -it --rm -v "$PWD:/home/user/project" stateoftheartio/qt6:6.5-mingw-aqt
# build application in ./dist/qmsc_win.tgz
user@2867a6d0b85d:~$ cd project && ./build.sh release install win32
```
