#!/bin/sh -e

CMD_CLEAN=false
CMD_INSTALL=false
CMD_GEN_TS=false
CMD_WIN=""
CMD_VERBOSE=""
CMD_BUILD="Debug"
CMD_NPROC=8
DIR_BUILD="build"
DIR_INSTALL="dist"

for ARG in "$@"; do
  case $ARG in
    "clean") CMD_CLEAN=true;;
    "release") CMD_BUILD="Release";;
    "verbose") CMD_VERBOSE="--verbose"; CMD_NPROC=1;;
    "install") CMD_INSTALL=true;;
    "gents") CMD_GEN_TS=true;;
    "win32") CMD_WIN="-DWIN32=ON";;
  esac
done

if $CMD_CLEAN; then
  echo "Cleaning build..."
  rm -rf ${DIR_BUILD} ${DIR_INSTALL}
  exit 0
fi

echo "Generating $CMD_BUILD build..."
cmake -Wno-dev -G "Ninja Multi-Config" -S . -B ${DIR_BUILD} -DQT_DEPLOY_USE_PATCHELF=ON ${CMD_WIN}

if ! $CMD_GEN_TS; then
  echo "Compiling $CMD_BUILD build..."
  cmake --build ${DIR_BUILD} --config ${CMD_BUILD} ${CMD_VERBOSE} -- -j${CMD_NPROC}
else
  echo "Updating translations..."
  cmake --build ${DIR_BUILD} --config ${CMD_BUILD} --target update_translations
fi

if $CMD_INSTALL; then
  echo "Installing in ${DIR_INSTALL}"
  rm -rf ${DIR_INSTALL}
  cmake --install ${DIR_BUILD} --config ${CMD_BUILD} --prefix $PWD/${DIR_INSTALL}/AppDir
  if [ -z "$CMD_WIN" ]; then
    ARCH=x86_64 appimagetool-x86_64.appimage ${DIR_INSTALL}/AppDir ${DIR_INSTALL}/qmsc.linux.AppImage
  else
    cd ${DIR_INSTALL} && tar cvzf qmsc.windows.tgz AppDir
  fi
fi

# boost
# git submodule update --init --depth 1 --recursive -- \
#          tools/build tools/boost_install libs/assert libs/bind libs/config \
#          libs/container_hash libs/core libs/describe libs/detail \
#          libs/function libs/function_types libs/functional libs/fusion \
#          libs/integer libs/io libs/mp11 libs/mpl libs/predef libs/preprocessor \
#          libs/static_assert libs/throw_exception libs/tuple libs/type_index \
#          libs/type_traits libs/typeof libs/utility libs/variant libs/parser
# ./bootstrap.sh
# ./b2 headers
# docker run --rm dockcross/windows-static-x64 > dockcross
# docker run -it --rm -v "$PWD:/home/user/project" stateoftheartio/qt6:6.5-mingw-aqt
