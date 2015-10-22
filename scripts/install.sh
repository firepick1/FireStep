#! /bin/bash

echo -e "\nSTART\t: `pwd`/$0\t`date`"

if [ "$SUDO_USER" != "" ]; then
  echo "ERROR	: This script must be run by non-root user"
  echo "TRY	:   scripts/install.sh"
  exit -1
fi

read -p "WARN	: This script may use sudo to change your system. Type \"y\" to proceed: " SUDOOK
if [ "$SUDOOK" != "y" ]; then
    echo -e "END\t: `pwd`/$0 (CANCELLED)"
	exit -1;
fi

if [ ! -e DueTimer ]; then
	echo "GIT	: git clone https://github.com/firepick1/DueTimer"
	git clone https://github.com/firepick1/DueTimer
fi
if [ ! -e Adafruit_NeoPixel ]; then
	echo "GIT	: git clone https://github.com/adafruit/Adafruit_NeoPixel"
	git clone https://github.com/adafruit/Adafruit_NeoPixel	
fi
if [ ! -e ArduinoJson ]; then
	echo "GIT	: git clone https://github.com/firepick1/ArduinoJson.git"
	git clone https://github.com/firepick1/ArduinoJson.git
	RC=$?; if [ $RC -ne 0 ]; then
		echo "ERROR	: git clone failed (RC=$RC)"
		exit -1
	fi
fi
pushd ArduinoJson
if [ -e lib ]; then
	GITMASTER=`git rev-parse @`
fi
GITREMOTE=`git rev-parse origin/master`
if [ "$GITMASTER" != "$GITREMOTE" ]; then
	echo "GIT	: git checkout master # `pwd`"
	git checkout master
	echo "BUILD	: `pwd`"
	cmake .
	make clean all
fi
popd

if [ "${FIREDUINO}" == "" ]; then
	export FIREDUINO=MOCK_MEGA2560
fi

if [ "${MEMORY_MODEL}" == "" ]; then
	export MEMORY_MODEL=MEMORY_MODEL_TINY
fi
echo "DEFINE	: MEMORY_MODEL=${MEMORY_MODEL}"
if [ ! -e ph5 ]; then
	echo "GIT	: git clone https://github.com/firepick1/ph5"
	git clone https://github.com/firepick1/ph5
	RC=$?; if [ $RC -ne 0 ]; then
		echo "ERROR	: git clone failed (RC=$RC)"
		exit -1
	fi
fi
pushd ph5
if [ -e /usr/lib/lib_ph5.so ]; then
	GITMASTER=`git rev-parse @`
fi
GITREMOTE=`git rev-parse origin/master`
if [ "$GITMASTER" != "$GITREMOTE" ]; then
	echo "GIT	: git checkout master # `pwd`"
	git checkout master
	git pull
	./build clean all
fi
popd

if [ "$DESTDIR" == "" ]; then
	echo "INSTALL	: using default installation prefix"
	export DESTDIR=/usr/local
fi
echo "INSTALL	: ${DESTDIR}"

if [ "$WINDIR" != "" ]; then
	echo "SYSTEM	: FireStep Windows support is limited to Arduino IDE"
	echo "TRY	: Copy ArduinoJson       to <your-Arduino-sketch-folder>/libraries/ArduinoJson"
	echo "TRY	: Copy Adafruit_NeoPixel to <your-Arduino-sketch-folder>/libraries/Adafruit_NeoPixel"
	echo "TRY	: Launch Arduino 1.6.5 IDE and open FireStep/FireStep.ino"
	echo "STATUS	: build complete"
	exit 0;
fi

if [ "$(type -p gcc)" == "" ]; then
  echo "COMMAND	: pkgin -y install scmgit-base gcc47"
  sudo pkgin -y install scmgit-base gcc47
else
  echo "STATUS	: GCC installed"
fi

if [ "$(type -p cmake)" == "" ]; then
  if [ `uname -o` == "Solaris" ]; then
    echo "COMMAND	: pkgin -y install gmake; pkgin -y install cmake"
    sudo pkgin -y install gmake
    sudo pkgin -y install cmake
  else
    echo "COMMAND	: apt-get -y install cmake"
    sudo apt-get -y install cmake
  fi
else
  echo "STATUS	: cmake installed"
fi

if [ "$(type -p libtool)" == "" ]; then
  echo "COMMAND	: pkgin -y install libtool"
  sudo pkgin -y install libtool
else
  echo "STATUS	: libtool installed"
fi

if [ "$(type -p aclocal)" == "" ]; then
  echo "COMMAND	: pkgin -y install automake"
  sudo pkgin -y install automake
else
  echo "STATUS	: automake installed"
fi

echo -e "END\t: `pwd`/$0\t`date`\t(COMPLETE)"

./build
