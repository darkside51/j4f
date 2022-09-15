#!/usr/bin/env python
# use example $python glsl_includes_ext.py -i ./glsl/ -o ./spirv/

import sys
import getopt
import os
import subprocess

def convert(inDir, fileName, outDir):
    fileData=""
    file = open(inDir + fileName, 'r')
    for line in file:
        if line.find('#import') == 0:
            includeFileName = line.replace('#import ', '')
            includeFileName = includeFileName.replace('\n', '')
            file2 = open(inDir + includeFileName, 'r')
            fileData2 = file2.read()
            fileData = fileData + fileData2
        else:
            fileData = fileData + line

    tmpFileName = 'tmp/' + fileName
    tmpFileName = tmpFileName.replace('.psh', '.frag')
    tmpFileName = tmpFileName.replace('.vsh', '.vert')
    tmpFileName = tmpFileName.replace('.gsh', '.geom')

    file3 = open(tmpFileName, 'w')
    file3.write(fileData)
    file3.close()

    VULKAN_SDK_PATH = os.getenv('VULKAN_SDK')
    converterUtilPath = VULKAN_SDK_PATH + '/Bin/glslc.exe'

    subprocess.run([converterUtilPath, tmpFileName, '-o', outDir + fileName + '.spv'])
    os.remove(tmpFileName)

def checkIsShaderFile(fileName):
    return fileName.endswith(".psh") or fileName.endswith(".vsh") or fileName.endswith(".gsh")

#entry point
if __name__ == '__main__':

    try:
        opts, args = getopt.getopt(sys.argv[1:], "i:o:", ["ifolder=", "ofolder="])
    except getopt.GetoptError:
        sys.exit(1)

    # check parameters length
    if len(opts) < 2:
        sys.exit(2)

    # parse parameters
    inputFolder = ''
    outputFolder = ''
    for opt, arg in opts:
        if opt in ("-i", "--ifolder"):
            inputFolder = arg
        elif opt in ("-o", "--ofolder"):
            outputFolder = arg

    if not os.path.exists(outputFolder):
        os.mkdir(outputFolder) 

    for path in os.listdir(inputFolder):
        # check if current path is a file
        if os.path.isfile(os.path.join(inputFolder, path)) and checkIsShaderFile(path):
            convert(inputFolder, path, outputFolder)

    print('*conversion complete*')