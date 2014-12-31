WT_VERSION=3.2.1

wget http://prdownloads.sourceforge.net/witty/wt-3.2.1.tar.gz?download -O wt-$WT_VERSION.tar.gz
tar xvfz wt-$WT_VERSION.tar.gz
cd wt-$WT_VERSION
mkdir build
cd build
cmake ../
make
make install
cd ../../
rm wt-$WT_VERSION.tar.gz
mkdir -p ../bin
cd ../bin/
ln -s ../lib/wt-$WT_VERSION/resources/
sudo ldconfig
