#coding:utf-8

from os import listdir
from os.path import isfile, isdir, join
import os
import sys

FSL_LWIP_HTTP = 0
#NATIVE_LWIP_HTTP = 1
RTCS_TFS_HTTP = 1
FSL_LWIP_HTTP_NCC = 2
FSL_LWIP_HTTP_FSL_CHIP = 3
FSL_LWIP_HTTP_NCC_EMI_TEST = 4
FSL_LWIP_HTTP_SAFETY_TEST = 5
FSL_LWIP_HTTP_A4_NEO = 6
TRANSLATE_TYPE_MAX = 7

FSL_LWIP_HTTP_STRUCT = 'HTTPSRV_FS_DIR_ENTRY'
RTCS_TFS_HTTP_STRUCT = 'TFS_DIR_ENTRY'

fileDict = {}
writeFileName = 'tfs_data_Q1.c'
folder = 'web_pages_minify'
translateType = FSL_LWIP_HTTP
setInputFolder = 0
setOutputFile = 0
setTranslateType = 0
structString = FSL_LWIP_HTTP_STRUCT

def translateStart(myDirectory, translateType):
    global structString
    global RTCS_TFS_HTTP
    global FSL_LWIP_HTTP_NCC
    global FSL_LWIP_HTTP
    global FSL_LWIP_HTTP_FSL_CHIP  
    global FSL_LWIP_HTTP_NCC_EMI_TEST    
    
    if translateType == RTCS_TFS_HTTP:
        structString = RTCS_TFS_HTTP_STRUCT
        print("Is old FSL RTCS type TFS data!!")
    else:
        print("Is NEW FSL LWIP type TFS data!!")
    saveDirectory = os.getcwd() 
    if len(fileDict) > 0:
        try:
            os.unlink(saveDirectory + '\\' + writeFileName)
        except:
            pass
        print('write File Name:' + writeFileName)
        writeFile = open(saveDirectory + '\\' + writeFileName, 'a')
        if translateType == RTCS_TFS_HTTP:
            writeFile.write('#ifndef SENSMINI_Q1\n\n')
            writeFile.write('#include <tfs.h>\n\n')
        else:
            if translateType == FSL_LWIP_HTTP_FSL_CHIP:
                writeFile.write('#ifndef SENSMINI_Q1\n\n')
            elif translateType == FSL_LWIP_HTTP_NCC:
                writeFile.write('#if NCC_VERSION && !EMI_TEST_VERSION\n\n')
            elif translateType == FSL_LWIP_HTTP_NCC_EMI_TEST:
                writeFile.write('#if NCC_VERSION && EMI_TEST_VERSION\n\n')
            elif translateType == FSL_LWIP_HTTP_SAFETY_TEST:
                writeFile.write('#if SAFETY_TEST_VERSION\n\n')
            elif translateType == FSL_LWIP_HTTP_A4_NEO:
                writeFile.write('#ifdef SENSMINIA4_NEO\n\n')
            else:
                writeFile.write('#if !NCC_VERSION && !SAFETY_TEST_VERSION\n\n')
                
            if translateType == FSL_LWIP_HTTP_A4_NEO:
                writeFile.write('#include "network/httpsrv/httpsrv_fs.h"\n\n');
            else:
                writeFile.write('#include "NET_APP/HTTPd/httpsrv/httpsrv_fs.h"\n\n');
        writeFile.write('extern const ' + structString + ' tfs_data[];\n\n');
    
    for x in range(len(fileDict)):
        readFilePath = join(myDirectory, fileDict[x])
        filenameStr1 = str(fileDict[x])
        filenameStr = filenameStr1.replace('.', '_')
        file = open(readFilePath, 'rb')
        code = bytearray(file.read())
        variableStr = 'tfs_' + folder + '_' + filenameStr
        file.close()
        writeFile.write('static const unsigned char ' + variableStr + '[] = {\n');
        writeFile.write('    /* ' + folder + '/' + fileDict[x] + ' */\n');
        sepStr = ''
        writeFile.write('    ')
        for y in range(len(code)):
            writeFile.write(sepStr)
            if y > 0 and (y % 16) == 0:
                writeFile.write('\n    ')
            writeFile.write('0x%02X' %code[y])
            sepStr = ', '
        writeFile.write('\n};\n\n')
    writeFile.write('const ' + structString + ' tfs_data[] = {\n')
    for x in range(len(fileDict)):
        filenameStr1 = str(fileDict[x])
        filenameStr = filenameStr1.replace('.', '_')
        variableStr = 'tfs_' + folder + '_' + filenameStr
        writeFile.write('    {"/' + fileDict[x] + '", 0, (unsigned char *)' + variableStr + ', sizeof(' + variableStr + ') },\n')
    writeFile.write('    { 0, 0, 0, 0}\n};\n\n')    
    
    #if translateType == RTCS_TFS_HTTP:
    writeFile.write('#endif\n\n')
    writeFile.close()
    
def getAllFile(directory):
    files = listdir(directory)
    x=0
    for fHandle in files:
        fullPath = join(directory, fHandle)
        if isfile(fullPath):
            if fullPath.endswith(".bak"):
                pass
            fileDict.setdefault(x, str(fHandle))
            x += 1
       
def showHelp():
    showInputReferent()
    showOutputReferent()
    showTypeReferent()
    print('example: ' + __file__ + ' -f web_pages_minify -o tfs_data_Q1.c -t 0')
        
def showInputReferent():
    print('-f / -F : input folder, default is web_pages_minify')
    
def showOutputReferent():
    print('-o / -O : output filename, default is tfs_data_Q1.c')
    
def showTypeReferent():    
    #print('-t / -T : translate type, 0: FSL LWIP HTTP, 1: NATIVE LWIP HTTP, 2: RTCS TFS HTTP, default is FSL LWIP HTTP')
    #print('-t / -T : translate type, 0: FSL LWIP HTTP, 1: RTCS TFS HTTP, default is FSL LWIP HTTP')
    print('-t / -T : translate type, 0: FSL LWIP HTTP, 1: RTCS TFS HTTP, default is FSL LWIP HTTP')
    
def showError():
    print('error argument!!')
       
def main():
    # print command line arguments
    global setInputFolder
    global setOutputFile
    global setTranslateType
    global translateType
    global writeFileName
    
    #for arg in sys.argv[1:]:
    #    print('arg:' + str(arg))
    for arg in sys.argv:
        if arg == 'h' or arg == 'H' or arg == '-h' or arg == '-H':
            showHelp()
            exit()
        elif arg == '-f' or arg == '-F':
            setInputFolder = 1
        elif arg == '-o' or arg == '-O':
            setOutputFile = 1
        elif arg == '-t' or arg == '-T':
            setTranslateType = 1
        elif setInputFolder == 1:
            setInputFolder = 0
            folder = arg
        elif setOutputFile == 1:
            setOutputFile = 0
            writeFileName = arg
        elif setTranslateType == 1:
            setTranslateType = 0
            translateType = int(arg)
                       
    if translateType >= TRANSLATE_TYPE_MAX:
        showError()
        showTypeReferent()
        exit()
    
    #print('argc:' + str(len(sys.argv)))
    #if len(sys.argv) == 2:
    #    folder = str(sys.argv[1:])[2:-2]
    #else :
    #    folder = 'web_pages_minify'

    saveDirectory = os.getcwd() 
    myDirectory = saveDirectory + '\\' + folder
    #writeDirectory = saveDirectory + '\\' + folder + '_minify'
    
    getAllFile(myDirectory)
    translateStart(myDirectory, translateType)

if __name__ == "__main__":
    main()
	
