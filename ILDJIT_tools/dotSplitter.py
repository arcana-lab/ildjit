#!/usr/bin/python
# -*- coding: utf-8 -*-
"""Split all the dot documents that are contained in a single file over multiple files"""

import sys
import os.path
from glob import glob

if len(sys.argv)==1:
	#Missing parameter
	print """No file name specified.

SYNTAX
	svgSplitter <fileName>
"""
else:
	#Fetch the list of files to elaborate
	fileNames=glob(sys.argv[1])
	print fileNames
	#Split the files
	if len(fileNames)==0:
		print "The specified file does not exist."
	else:
		for inFileName in fileNames:
			print "Splitting \""+ inFileName + "\":"
			inFile=open(inFileName, "r")
			outFileBaseName=os.path.splitext(inFileName)[0].replace(" ", "_")
			outFileExt=".dot" #os.path.splitext(inFileName)[1]
			fileCount=0
			outFile=open("Errors.txt", "w")
			for line in inFile:
				if line.startswith("digraph") and line.rstrip().endswith("{"):
					outFile.close();
					fileCount += 1
					outFile=open(outFileBaseName+str(fileCount)+outFileExt, "w")

				outFile.write(line)

			outFile.close()

			print
			print str(fileCount)+ " svg files generated"
			print
			print "All done"


