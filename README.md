# fsfreader
A small command line program to read files containing files embedded into a FSF structure.

## What is FSF?
FSF is a proprietary file format from [AMX](https://www.amx.com). For details search for patent number *WO 2007/030421 A2*. A FSF file may contain several files and was invented to have a lightweight file system on an embedded controller. **AMX** is using it to save the data for configuring a panel. The program TPDesign (either version 4 and 5) is using it.

## What for is *fsfreader*?
This program is able to read a FSF file and extract all the files in it. If the files were extracted from a TP4 or TP5 file, all resulting files are decoded and *unziped* (TP4) or decrypted (TP5). **fsfreader** detects the type of file. While binary files like fonts, graphic files and the like are never encrypted or compressed, they are taken as they are. All XML files (files ending in .XMA and .XML) ere either compressed or encrypted. In TP4 files this files are compressed, while in TP5 files they are encrypted. **fsfreader** detects the kind of file and uncompressed or decrypt them accordingly. Even if this is mixed in the TPx file (never seen a mixed file).

## Compile the program
*fsfreader* was developed on *Linux*. It needs at least a C++ compiler which supports C++17 standard. Because the program makes use of the new **filesystem** methods. **G++9** or newer is perfect, as well as **Clang5** or newer. It should compile with older compilers too, but there is no guaranty.

You need the following prerequisites:

* GNU C++ compiler version 9 or newer, or Clang compiler version 5 or newer (or any other C++ compiler supporting the *filesystem* methods).
* ZLib library
* openssl

1. Download the source from this page.
1. Enter the directory where you've downloaded the source and create a directory named **build**
1. Enter the just created directory **build** and type:
  * cmake ..
  * make
  * sudo make install

## How to use the program?

> `Syntax: fsfreader -f <infile> [-d <directory>] [-t] [-v] [-h]`
  
### Parameters
Parameter|Comment
---------|-------
`-f --file`|The path and name of the input file. This must be a FSF file (TP4 or TP5).
`-d --directory`|Optional. The output directory where the content of the FSF file will be saved. If ths parameter is omitted, the files are written into the current directory.
`-t --transfer`|This make **FSFReader** to create a directory structure. All the graphic files are moved to `images`, the sound files are moved to `sounds` and the fonts are moved to `fonts`. The rest remains in the main directory.
`-v --verbose`|This makes the program very noisy. It prints out how it internaly follows the block structure of the input file.
`-h --help`|A small help that explains the available parameters.

