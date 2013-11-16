#!/bin/sh

cd mac-binaries/
mkdir tmp
cd tmp
rm bin.tar.gz
#wget http://192.168.1.50:9000/deb/bin.tar.gz
#wget http://192.168.1.3:9000/deb/bin.tar.gz
wget http://10.0.0.5:9000/deb/bin.tar.gz

cd ..
tar -zxvf tmp/bin.tar.gz

