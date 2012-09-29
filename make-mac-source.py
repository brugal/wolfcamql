#!/usr/bin/env python
import os

os.system("tar -zcvf build/macsrc.tar.gz Makefile* *.sh *.py code/ ui/ misc/")
