#!/bin/bash

# for current session
export PATH=$PATH:/database/config/db2inst1/sqllib/bin
export DB2INSTANCE=db2inst1

# for future sessions
echo 'export PATH=$PATH:/database/config/db2inst1/sqllib/bin' >> ~/.bashrc
echo 'export DB2INSTANCE=db2inst1' >> ~/.bashrc

usermod -a -G db2iadm1 root

yum install -y git gdb vim-enhanced jansson-devel systemd-devel gnutls-devel uncrustify libtool make gettext help2man

mkdir ~/source && cd $_
git clone https://git.savannah.gnu.org/git/texinfo.git
git clone https://github.com/Karlson2k/libmicrohttpd.git
git clone https://github.com/babelouest/orcania.git
git clone https://github.com/babelouest/yder.git
git clone https://github.com/babelouest/ulfius.git

# dependency of libmicrohttpd, makeinfo is not in the info (texinfo) RHEL 8.6 package...
# checkout older version due to only automake 1.16.1 available in the RHEL 8.6 repo...
cd texinfo
git checkout tags/texinfo-6.8 -b texinfo-6.8
./autogen.sh
./configure
make && make install
ln -s /usr/local/bin/makeinfo /usr/bin/makeinfo
cd ..

# dependency of ulfius, dev headers not in RHEL 8.6 repo...
cd libmicrohttpd
./bootstrap
./configure
make && make install
cd ..

cd orcania
make && make install
cd ..

cd yder
make && make install
cd ..

cd ulfius
make && make install
