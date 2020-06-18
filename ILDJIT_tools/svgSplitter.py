#!/usr/bin/python
# -*- coding: utf-8 -*-
"""Split all the svg (xml) documents that are contained in a single file over multiple files"""

import sys
import os.path
if len(sys.argv)==1:
	#Missing parameter
	print """No file name specified.

SYNTAX
	svgSplitter <fileName>
"""
else:
	inFileName=sys.argv[1]
	if os.path.isfile(inFileName):
		inFile=open(sys.argv[1], "r")
		outFileBaseName=os.path.splitext(inFileName)[0]
		outFileExt=os.path.splitext(inFileName)[1]
		fileCount=0
		outFile=open("Errors.txt", "w")
		for line in inFile:
			if line.startswith("<?xml"):
				outFile.close();
				fileCount += 1
				outFile=open(outFileBaseName+str(fileCount)+outFileExt, "w")

			outFile.write(line)

		outFile.close()

		print
		print str(fileCount)+ " svg files generated"
		print
		print "All done"

	else:
		print "The specified file does not exist."


