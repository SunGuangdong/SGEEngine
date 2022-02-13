import sys
import argparse

def bytesToCHeader(fileBytes: bytes, inputFileName: str, variableName: str) -> str:
    fileText = "#pragma once\n";
    fileText += "\n";
    fileText += "//----------------------------------------------\n";
    fileText += "// This is an auto-generated file.\n";
    fileText += "// It contains the binary representation of " + inputFileName + " as a C-Array.\n";
    fileText += "//----------------------------------------------\n";
    fileText += "\n";
    fileText += "// clang-format off\n";
    fileText += "const unsigned char " + variableName + "[] = {\n\t"
    
    numElements = 0;
    for byte in fileBytes:
        if numElements >= 20:
            fileText += "\n\t"
            numElements = 0;
            
        fileText += hex(byte) 
        fileText += ", "
        numElements += 1
        
    fileText += "\n};\n"
    fileText += "// clang-format on\n"
    
    return fileText
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert binary file to C-style array.')
    parser.add_argument("filename", help="the file to be converted")
    parser.add_argument("-o", "--output", help="write output to a file")
    parser.add_argument("-v", "--variable", help="the name of the variable written in the C-Header that contains the data")
    args = parser.parse_args()

    with open(args.filename, 'rb') as f:
        fileBytes = f.read()
    fileText = bytesToCHeader(fileBytes, args.filename, args.variable);
    
    headerFile = open(args.output, "w")
    headerFile.write(fileText)