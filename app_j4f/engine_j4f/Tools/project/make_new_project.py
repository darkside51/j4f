#!/usr/bin/env python3

import sys
import getopt
import os
import subprocess
import shutil

def copyDirectory(fromDir, toDir):
    for path in os.listdir(fromDir):
        if os.path.isfile(os.path.join(fromDir, path)):
            shutil.copy(fromDir + '/' + path, toDir + '/' + path)
        else:
            if not os.path.exists(toDir + '/' + path):
                os.mkdir(toDir + '/' + path)
            copyDirectory(fromDir + '/' + path, toDir + '/' + path)

#entry point
if __name__ == '__main__':
    print('*create new project*')

    try:
        opts, args = getopt.getopt(sys.argv[1:], "n:", ["name="])
    except getopt.GetoptError:
        sys.exit(1)

    # parse parameters
    project_name = ''
    for opt, arg in opts:
        if opt in ("-n", "--name"):
            project_name =  arg

    project_path = '../../../' + project_name

    if not os.path.exists(project_path):
        os.mkdir(project_path)

    src_path = 'new_project_src'
    # copy src files
    copyDirectory(src_path, project_path)

    #rename project
    fileData=""
    file = open(project_path + '/CMakeLists.txt', 'r')
    for line in file:
        if line.find('new_project') != -1:
            fixedLine = line.replace('new_project', project_name)
            fileData = fileData + fixedLine
        else:
            fileData = fileData + line

    file.close()
    file = open(project_path + '/CMakeLists.txt', 'w')
    file.write(fileData)
    file.close()
