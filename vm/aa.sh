cd ..
source ./activate
cd userprog
make clean
make
cd build
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/args-multiple:args-multiple --swap-disk=4 -- -q   -f run 'args-multiple some arguments for you!'
Kernel command line: -q -f put multi-oom run multi-oom
perl -I../.. ../../tests/userprog/no-vm/multi-oom.ck tests/userprog/no-vm/multi-oom tests/userprog/no-vm/multi-oom.result