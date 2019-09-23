# fsfreader
A small command line program to read files containing files embedded into a FSF structure.

## What is FSF?
FSF is a proprietary file format from [AMX](https://www.amx.com). For details search for patent number *WO 2007/030421 A2*. A FSF file may contain several files and was invented to have a lightweight file system on an embedded controller. **AMX** is using it to save the data for configuring a panel. The program TPDesign (either version 4 and 5) is using it.

## What for is *fsfreader*?
This program is able to read a FSF file and extract all the files in it. If the files were extracted from a TP4 file, all resulting files are decoded and *unziped*. If you extract from a TP5 file, you'll get all files, but all XML files are somehow encoded and probably zipped. All binary files like graphic files (PNG, JPG) and sound files (WAV, MP3) are clear and readable. All configuration files are encrypted.
**If someone know how to decrypt them, I would apreciate this information!**

## Compile the program
*fsfreader* was developed on *Linux*. It needs at least a C++ compiler which supports C++17 standard. Because the program makes use of the new **filesystem** methods. **G++9** or newer is perfect. It should compile with older compilers too, but there is no garanty.

You need the following prerequisites:

* GNU C++ compiler version 9 or newer (or any other C++ compiler supporting the *filesystem* methods).
* ZLib library

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
`-f --file`|The path and name of the input file. This must be a FSF file.
`-d --directory`|Optional. The output directory where the content of the FSF file will be saved. If ths parameter is omitted, the files are written into the current directory.
`-t --transfer`|This make **FSFReader** to create a directory structure. All the graphic files are moved to `images`, the sound files are moved to `sounds` and the fonts are moved to `fonts`. The rest remains in the main directory.
`-v --verbose`|This makes the program very noisy. It prints out how it internaly follows the block structure of the input file.
`-h --help`|A small help that explains the available parameters.

