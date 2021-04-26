export REL_TOP=/data/bmnnsdk2-bm1684_v2.3.2

rm -fr build
mkdir build
cd build
cmake -DTARGET_ARCH=soc -DUSE_BMNNSDK2=OFF ..
make -j4
cd ..

