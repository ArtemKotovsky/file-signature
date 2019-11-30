#!/usr/bin/python3
# -*- coding: <encoding name> -*-

import sys, os
import zlib, hashlib

FILENAME = "Filename: "
FILESIZE = "Filesize: "
HASH = "Hash: "

def crc32(data):
    return "{0:08x}".format(zlib.crc32(data) & 0xffffffff).lower()
    
def sha2(data):
    return hashlib.sha256(data).hexdigest()

def getHeader(file):
    
    filename = None
    filesize = None
    hash = None
    
    line = file.readline().splitlines()[0]
    
    while line:
    
        if line.lower().startswith(FILENAME.lower()):
            filename = line[len(FILENAME):]
            
        elif line.lower().startswith(FILESIZE.lower()):
            filesize = int(line[len(FILESIZE):])
            
        elif line.lower().startswith(HASH.lower()):
            s = line[len(HASH):]
            hash = globals()[s]
        
        if filename and filesize and hash:
            return (filename, filesize, hash)
        
        line = file.readline().splitlines()[0]
            
    raise Exception("File header is invalid")

def checkFile(filename):
    print("\nChecking file", filename)
    
    with open(filename, "r") as file:
        filename, filesize, hash = getHeader(file)
        
        if os.path.getsize(filename) != filesize:
            raise Exception("invalid file size {0}".format(filesize))
            
        with open(filename, "rb") as bin:
        
            line = file.readline()
            
            while line:
                line = line.splitlines()[0].split(":")
                
                offset = int(line[0], 16)
                size = int(line[1], 16)
                hashVal = line[2]
                
                bin.seek(offset)
                hashVal2 = hash(bin.read(size))
                
                if hashVal2 != hashVal:
                    raise Exception("invalid hash offset={0} size={1} {2}!={3}".format(
                                    offset, size, hashVal, hashVal2))
                                    
                print("offset {0} ok".format(offset))
                line = file.readline()
    print("File is ok")

def main():
    
    if len(sys.argv) == 1:
        print("Using: {0} <file.signature>".format(os.path.basename(sys.argv[0])))
        return

    for filename in sys.argv[1:]:
        checkFile(filename)

if __name__ == '__main__':
    main()
