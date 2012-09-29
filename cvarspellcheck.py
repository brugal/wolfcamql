#!/usr/bin/env python
import sys, re


f = open(sys.argv[1])
lines = f.readlines()
reObj = re.compile('\s*?{\s*?&\s*(?P<cvarName>.*?)\s*?,\s*?"\s*?(?P<stringName>.*?)\s*?",')
for line in lines:
    match = reObj.search(line, 1)
    #print match
    if match:
        #print (match["cvarName"],)
        #print match.groups()[1]
        if match.groups()[0] != match.groups()[1]:
            print line[:-1]
