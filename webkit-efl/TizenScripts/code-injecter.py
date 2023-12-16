#! /usr/bin/python
# scripted by ryuan.choi
import sys
import os
import re

# gloaval variables.
fileMap = {}
macro = "TIZEN_PROFILE"
CONFIG_HEADER = "include \"config.h\""
HEADER = """
#include "WebCore/platform/efl/tizen/TizenProfiler.h"
"""

# functions
def resetFunctions(srcStr):
    srcStr = srcStr.replace(HEADER, "")
    srcStr = srcStr.replace(macro, "")
    return srcStr

def injectHeader(srcStr):
    return srcStr.replace(CONFIG_HEADER, CONFIG_HEADER + HEADER)

def injectFunctions(srcStr, functions):
    if "*" in functions:
        return re.sub(r"(\w*\([\s\w,<>&\*]*\)[\s\w]*\n{)", r"\1" + macro, srcStr)
    else:
        functions = "|".join(str(i).strip() for i in functions)
        return re.sub(r"((" + functions + ")\([\s\w,<>&\*]*\)[\s\w]*\n{)", r"\1" + macro, srcStr)

# main code
fin = open("TizenScripts/injecter/default.txt", "r")
lines = fin.readlines()
fin.close()
for line in lines:
    if line.startswith("#") or len(line.strip()) == 0:
        continue

    items = line.strip().split(":")
    if (len(items) == 1):
        functionNames = set('*')
    else:
        functionNames = set(items[1].split(','))

    fileName = items[0]
    if fileName in fileMap:
        fileMap[fileName] = fileMap[fileName] | functionNames
    else:
        fileMap[fileName] = functionNames

for fileName, functions in fileMap.iteritems():
    fileName = fileName.split("Source/")[-1] #if Source path is given, remove it.

    src = open("Source/" + fileName, "r")
    srcStr = src.read()
    src.close()

    buffer = resetFunctions(srcStr)
    if "reset" not in sys.argv:
        buffer = injectHeader(buffer)
        buffer = injectFunctions(buffer, functions)

    dest = open("Source/" + fileName, "w")
    dest.write(buffer)
    dest.close()
