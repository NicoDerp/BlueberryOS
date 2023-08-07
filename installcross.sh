#!/bin/sh


# Exit immediately if a command returns a non-zero value
set -e


# Get BLUEBERRYOS_SOURCE
BLUEBERRYOS_SOURCE=$(dirname $(realpath "$0"))
RED='\033[0;31m'
NC='\033[0m'

echo "Welcome to the BlueberryOS installer!"
echo "You will now be promped with questions with the default answer in parentheses (like this)."
echo "If you are unsure of what to answer, press enter to use the default answer.\n"


# Get binutils install directory
read -p "Enter the path to where you want binutils and GCC source to be installed ($HOME/crosscompiler): " CROSSCOMPILER_DIR

# Default
if [ -z "$CROSSCOMPILER_DIR" ]; then
    CROSSCOMPILER_DIR="${HOME}/crosscompiler"
fi

# Check if path is valid
case $CROSSCOMPILER_DIR in
  /*) ;;
  *)
    echo "\n${RED}Fatal error occured!${NC} Path ${CROSSCOMPILER_DIR} is not an absolute path!"
    exit ;;
esac

# Check if directory already exists
if [ -d "$CROSSCOMPILER_DIR" ]; then
    echo "\n${RED}Fatal error occured!${NC} Directory ${CROSSCOMPILER_DIR} already exists!\nPlease choose another path or delete it."
    exit
fi


# Get binutils install directory
read -p "Enter the path to where you want BlueberryOS toolchain to be installed ($HOME/opt/cross): " PREFIX


# Default
if [ -z "$PREFIX" ]; then
    PREFIX="${HOME}/opt/cross"
fi

# Check if path is valid
case $PREFIX in
  /*) ;;
  *)
    echo "\n${RED}Fatal error occured!${NC} Path ${PREFIX} is not an absolute path!"
    exit ;;
esac

# Check if directory already exists
if [ -d "$PREFIX" ]; then
    echo "\n${RED}Fatal error occured!${NC} Directory ${PREFIX} already exists!\nPlease choose another path or delete it."
    exit
fi


# Get number of cores
RECOMMENDED_CORES=$(nproc --ignore=2)
read -p "Enter the number of cores that you want to use while compiling, where 1 is no multiprocessing ($RECOMMENDED_CORES): " CORES

# Default
if [ -z "$CORES" ]; then
    CORES=$RECOMMENDED_CORES
fi
CORES=-j$CORES



BINUTILS=$CROSSCOMPILER_DIR/binutils
BINUTILS_BUILD=$CROSSCOMPILER_DIR/build-bintuils
GCC=$CROSSCOMPILER_DIR/gcc
GCC_BUILD=$CROSSCOMPILER_DIR/build-gcc
PATH="$PREFIX/bin:$PATH"

echo

echo "Using BINUTILS at '${BINUTILS}'"
echo "Using BINUTILS_BUILD at '${BINUTILS_BUILD}'"
echo "Using GCC at '${GCC}'"
echo "Using GCC_BUILD at '${GCC_BUILD}'"
echo "Using BLUEBERRYOS_SOURCE as '$BLUEBERRYOS_SOURCE'"
echo "Using CORES $CORES"
echo "Using PATH '$PATH'"

echo
read -p "Press enter to continue" tmp


echo
echo "Downloading binutils source..."
echo git clone https://sourceware.org/git/binutils-gdb.git $BINUTILS
git clone https://sourceware.org/git/binutils-gdb.git $BINUTILS

echo "Downloading GCC source..."
echo git clone https://sourceware.org/git/binutils-gdb.git $GCC
git clone https://sourceware.org/git/binutils-gdb.git $GCC



echo
echo "Applying binutils patch..."
echo "cd $BINUTILS"
cd $BINUTILS
echo "git apply $BLUEBERRYOS_SOURCE/binutils.patch"
git apply $BLUEBERRYOS_SOURCE/binutils.patch


echo
echo "Applying GCC patch..."
echo "cd $GCC"
cd $GCC
echo "git apply $BLUEBERRYOS_SOURCE/gcc.patch"
git apply $BLUEBERRYOS_SOURCE/gcc.patch


echo
echo "Configuring bintuils..."
echo "mkdir $BINUTILS_BUILD"
mkdir $BINUTILS_BUILD
echo "cd $BINUTILS_BUILD"
cd $BINUTILS_BUILD
echo "../binutils/configure --target=i686-blueberryos --prefix="$PREFIX" --with-sysroot=$BLUEBERRYOS_SOURCE/sysroot --enable-languages=c"
../binutils/configure --target=i686-blueberryos --prefix="$PREFIX" --with-sysroot=$BLUEBERRYOS_SOURCE/sysroot --enable-languages=c


echo
echo "Building binutils..."
echo "make $CORES && make install $CORES"
make $CORES && make install $CORES


echo
echo "Configuring GCC..."
echo "mkdir $GCC_BUILD"
mkdir $GCC_BUILD
echo "cd $GCC_BUILD"
cd $GCC_BUILD
echo "../gcc/configure --target=i686-blueberryos --prefix="$PREFIX" --with-sysroot=$BLUEBERRYOS_SOURCE/sysroot --enable-languages=c"
../gcc/configure --target=i686-blueberryos --prefix="$PREFIX" --with-sysroot=$BLUEBERRYOS_SOURCE/sysroot --enable-languages=c

echo
echo "Building GCC..."
echo "make all-gcc $CORES && make install-gcc $CORES"
make all-gcc $CORES && make install-gcc $CORES


echo
echo "Updating PATH in .bashrc..."
echo "echo $PREFIX/bin:$PATH >> $HOME/.bashrc"
echo $PREFIX/bin:$PATH >> $HOME/.bashrc


echo
echo "All done!"
echo "BlueberryOS GCC version:"
echo $(i686-blueberryos-gcc --version)

