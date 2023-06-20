make clean
source ./activate
cd vm
make clean
make
cd build
sleep 2
make check