#! /usr/bin/python
# scripted by ryuan.choi
import sys
import os
import re

specFileName = "packaging/webkit2-efl.spec"

fin = open(specFileName, "r")
buf = fin.read()
fin.close()
fout = open(specFileName, "w")
fout.write(re.sub(r"Version: [\d_.]*", "Version: 152340_0.999.0", buf))
fout.close()
