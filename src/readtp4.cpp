/*
 * Copyright (C) 2019 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <bits/stdc++.h>
#if __GNUC__ < 9
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include "fsfreader.h"
#include "readtp4.h"
#include "expand.h"

namespace fs = std::filesystem;
using namespace reader;
using namespace std;

reader::ReadTP4::~ReadTP4()
{
	deleteIndex();
}

bool ReadTP4::doRead()
{
	if (fname.length() == 0)
	{
		cerr << "Missing input file!" << endl;
		return false;
	}

	ifstream fn (fname, ios::in | ios::binary);

	if (!fn.is_open())
	{
		cerr << "Could not open file " << fname << "!" << endl;
		return false;
	}

	struct HEADER head;
	struct BLOCK block;
	struct USAGE_BLOCK usageBlock;
	struct FILE_HEAD fhead;
	unsigned char memblock[1024];

	try
	{
		fn.read(reinterpret_cast<char*>(&head), SIZE_HEADER);

		if (memcmp(head.abyFileID, "\0FSFILE\0", 8) != 0)
		{
			cerr << "File " << fname << " is not an FSF file!" << endl;
			fn.close();
			return false;
		}

		if (verbose)
			cout << "First block starts at: " << to_string(head.listStartBlock) << endl << endl;

		fn.read(reinterpret_cast<char*>(&memblock[0]), SIZE_BLOCK);
		fillBlock(block, memblock);
		fillFileHead(fhead, memblock);

		if (verbose)
		{
			cout << "thisBlock: " << to_string(block.thisBlock) << endl;
			cout << "prevBlock: " << to_string(block.prevBlock) << endl;
			cout << "nextBlock: " << to_string(block.nextBlock) << endl;
			cout << "bytesUsed: " << to_string(block.bytesUsed) << endl << endl;
		}

		uint32_t nextBlock = head.listStartBlock;
		// first we read the index
		while (nextBlock > 0)
		{
			fn.seekg(calcBlockPos(nextBlock), ios::beg);
			fn.read(reinterpret_cast<char*>(memblock), SIZE_BLOCK);
			fillUsageBlock(usageBlock, memblock);
			fillFileHead(fhead, memblock);

			if (fhead.thisBlock != nextBlock)
			{
				cerr << "No valid block position (" << to_string(fn.tellg()) << " [" << toHex(fn.tellg(), 8) << "])" << endl;
				break;
			}

			if (verbose)
			{
				cout << "thisBlock: " << to_string(usageBlock.thisBlock) << " (" << toHex(usageBlock.thisBlock, 8) << ")" << endl;
				cout << "prevBlock: " << to_string(usageBlock.prevBlock) << " (" << toHex(usageBlock.prevBlock, 8) << ")" << endl;
				cout << "nextBlock: " << to_string(usageBlock.nextBlock) << " (" << toHex(usageBlock.nextBlock, 8) << ")" << endl;
				cout << "bytesUsed: " << to_string(usageBlock.bytesUsed) << " (" << toHex(usageBlock.bytesUsed, 8) << ")" << endl;
				cout << "filePath:  " << usageBlock.filePath << endl << endl;
				cout << "creat time:" << asctime(gmtime(&usageBlock.tmCreate));
				cout << "modif time:" << asctime(gmtime(&usageBlock.tmModify));
				cout << "Flags:     " << to_string(usageBlock.flags) << endl;
				cout << "startBlock:" << to_string(usageBlock.startBlock) << endl;
				cout << "sizeBlocks:" << to_string(usageBlock.sizeBlocks) << endl;
				cout << "sizeBytes: " << to_string(usageBlock.sizeBytes) << " (" << toHex(usageBlock.sizeBytes, 8) << ")" << endl << endl;
			}

			appendUBlock(&usageBlock);
			nextBlock = fhead.nextBlock;
		}

		if (verbose)
			cout << endl << endl << "-- Starting to extract files ..." << endl << endl;

		// Now we read the files
		struct INDEX *act = idx;

		while (act != nullptr)
		{
			string ofile;

			if (transfer)
			{
				string f(cp1250ToUTF8((char *)act->ublock->filePath));
				struct MANIFEST mf;
				mf.size = act->ublock->sizeBytes;
				mf.tmCreate = act->ublock->tmCreate;
				mf.tmModify = act->ublock->tmModify;
				mf.fname.assign(f);
				manifest.push_back(mf);
				string dir;

				if (f.find(".png") != string::npos || f.find(".jpg") != string::npos ||
						f.find(".gif") != string::npos || f.find(".tiff") != string::npos)
					dir = "/images";
				else if (f.find(".wav") != string::npos || f.find(".mp3") != string::npos)
					dir = "/sounds";
				else if (f.find(".ttf") != string::npos)
					dir = "/fonts";

				if (verbose)
					cout << "Testing for directory: " << target << dir << endl;

				if (!fs::exists(target+dir))
					fs::create_directories(target+dir);

				ofile = target + dir + "/" + f;
			}
			else
				ofile = target + "/" + string((char*)act->ublock->filePath);

			cout << "Writing file " << ofile << endl;
			ofstream of(ofile, ios::out | ios::binary);

			if (!of)
			{
				cerr << "Error opening target file" << ofile << "!" << endl;
				fn.close();
				return false;
			}

			nextBlock = act->ublock->startBlock;
			bool compressed = false;

			for (uint32_t i = 0; i < act->ublock->sizeBlocks; i++)
			{
				fn.seekg(calcBlockPos(nextBlock), ios::beg);
				fn.read(reinterpret_cast<char*>(memblock), SIZE_BLOCK);
				fillBlock(block, memblock);

				if (i == 0 && block.abyData[0] == 0x1f && block.abyData[1] == 0x8b)
					compressed = true;

				nextBlock = block.nextBlock;
				of.write(reinterpret_cast<char*>(block.abyData), block.bytesUsed);

				if (verbose)
				{
					cout << "thisBlock: " << to_string(block.thisBlock) << " (" << toHex(block.thisBlock, 8) << ")" << endl;
					cout << "prevBlock: " << to_string(block.prevBlock) << " (" << toHex(block.prevBlock, 8) << ")" << endl;
					cout << "nextBlock: " << to_string(block.nextBlock) << " (" << toHex(block.nextBlock, 8) << ")" << endl;
					cout << "blockSize: " << to_string(block.bytesUsed) << " (" << toHex(block.bytesUsed, 4) << ")" << endl << endl;
				}
			}

			of.close();

			if (compressed)
			{
				cout << "Decompressing file " << ofile << " ...";
				Expand exp(ofile);
				int ret = exp.unzip();

				if (ret != 0)
					cerr << "WARNING: File " << ofile << " was not decompressed!" << endl;
				else
					cout << " done" << endl;
			}

			compressed = false;
			act = act->next;
		}

		if (transfer)
		{
			ofstream mfStream;
			string manFileName = target + "/manifest.xma";

			mfStream.open(manFileName, ios::out | ios::binary | ios::trunc);
			sort(manifest.begin(), manifest.end(), compareManifest);
			size_t num = manifest.size();
			size_t cnt = 0;

			for (auto itr = manifest.begin(); itr != manifest.end(); ++itr)
			{
				mfStream << itr->size << "|" << itr->tmCreate << "|" << itr->tmModify << "|" << itr->fname;
				cnt++;

				if (cnt == num)
					mfStream << "\r";
				else
					mfStream << "\r\n";
			}

			mfStream.close();
		}
	}
	catch (exception& e)
	{
		cerr << "ReadTP4::doRead: " << e.what() << endl;
		fn.close();
		return false;
	}

	fn.close();
	return true;
}

void ReadTP4::fillBlock (struct BLOCK &bl, const unsigned char *buf)
{
	bl.thisBlock = makeDWord(buf);
	bl.prevBlock = makeDWord(buf+4);
	bl.nextBlock = makeDWord(buf+8);
	bl.bytesUsed = makeWord(buf+12);
	memcpy(&bl.abyData, buf+14, 512);
}

void ReadTP4::fillUsageBlock (struct USAGE_BLOCK &ub, const unsigned char *buf)
{
	ub.thisBlock = makeDWord(buf);
	ub.prevBlock = makeDWord(buf+4);
	ub.nextBlock = makeDWord(buf+8);
	ub.bytesUsed = makeWord(buf+12);
	memcpy(&ub.filePath, buf+14, 260);
	ub.tmCreate = makeDWord(buf+274);
	ub.tmModify = makeDWord(buf+278);
	ub.flags = makeDWord(buf+282);
	ub.startBlock = makeDWord(buf+286);
	ub.sizeBlocks = makeDWord(buf+290);
	ub.sizeBytes = makeDWord(buf+294);
}

void ReadTP4::fillFileHead (struct FILE_HEAD &fh, const unsigned char *buf)
{
	fh.thisBlock = makeDWord(buf);
	fh.prevBlock = makeDWord(buf+4);
	fh.nextBlock = makeDWord(buf+8);
	fh.blockLen = makeWord(buf+12);
}

struct INDEX *ReadTP4::appendUBlock (const struct USAGE_BLOCK *ub)
{
	struct USAGE_BLOCK *nb;
	struct INDEX *act, *nidx;

	try
	{
		nb = new struct USAGE_BLOCK;
		memcpy(nb, ub, sizeof(struct USAGE_BLOCK));

		if (idx == nullptr)
		{
			act = new struct INDEX;
			act->prev = nullptr;
			act->next = nullptr;
			act->ublock = nb;
			idx = act;
		}
		else
		{
			act = idx;

			while (act->next != nullptr)
				act = act->next;

			nidx = new struct INDEX;
			nidx->prev = act;
			nidx->next = nullptr;
			nidx->ublock = nb;
			act->next = nidx;
			act = nidx;
		}
	}
	catch (exception& e)
	{
		cerr << "ReadTP4::appendUBlock: " << e.what() << endl;
		exit(3);
	}

	return act;
}

void ReadTP4::deleteIndex()
{
	struct INDEX *tmp, *act = idx;

	while (act != nullptr)
	{
		tmp = act->next;

		if (act->ublock != nullptr)
			delete act->ublock;

		delete act;
		act = tmp;
	}

	idx = nullptr;
}

size_t ReadTP4::calcBlockPos (uint32_t block)
{
    if (block == 0)
        return SIZE_HEADER;

    size_t first = SIZE_HEADER;
    return (first + (SIZE_BLOCK * block));
}

uint32_t ReadTP4::makeDWord (const unsigned char *buf)
{
	unsigned short b1, b2, b3, b4;

	b1 = *buf;
	b2 = *(buf+1);
	b3 = *(buf+2);
	b4 = *(buf+3);
	uint32_t dword = ((b4 << 24) & 0xff000000) | ((b3 << 16) & 0x00ff0000) | ((b2 << 8) & 0x0000ff00) | b1;
	return dword;
}

uint16_t ReadTP4::makeWord (const unsigned char *buf)
{
	uint16_t word = ((*(buf+1) << 8) & 0xff00) | *buf;
	return word;
}

string ReadTP4::toHex(int num, int width)
{
	string ret;
	std::stringstream stream;
	stream << std::setfill ('0') << std::setw(width) << std::hex << num;
	ret = stream.str();
	return ret;
}

void ReadTP4::dump (const unsigned char *buf, size_t size)
{
	short pos = 0;
	char byte = 0;
	bool nl = true;

	for (size_t i = 0; i < size; i++)
	{
		if (nl)
			cout << toHex(pos, 4) << ": ";

		nl = false;
		byte = *(buf+i);
		cout << toHex(byte&0x000000ff, 2) << " ";

		if (i > 0 && ((i+1) % 8) == 0)
		{
			pos = i + 1;
			nl = true;
			cout << endl;
		}
	}

	cout << endl << endl;
}

bool ReadTP4::compareManifest(struct MANIFEST& m1, struct MANIFEST& m2)
{
	size_t pos1 = m1.fname.find_last_of(".");
	size_t pos2 = m2.fname.find_last_of(".");
	string ext1, ext2;
	int weight1, weight2;

	if (pos1 != string::npos)
		ext1 = m1.fname.substr(pos1+1);
	else
		ext1 = m1.fname;

	if (pos2 != string::npos)
		ext2 = m2.fname.substr(pos2+1);
	else
		ext2 = m2.fname;

	if (ext1.compare("xma") == 0)
		weight1 = 1;
	else if (ext1.compare("xml") == 0)
		weight1 = 2;
	else if (ext1.compare("ttf") == 0)
		weight1 = 3;
	else if (ext1.compare("png") == 0 || ext1.compare("jpg") == 0 || ext1.compare("gif") == 0)
		weight1 = 4;
	else
		weight1 = 5;

	if (ext2.compare("xma") == 0)
		weight2 = 1;
	else if (ext2.compare("xml") == 0)
		weight2 = 2;
	else if (ext2.compare("ttf") == 0)
		weight2 = 3;
	else if (ext2.compare("png") == 0 || ext2.compare("jpg") == 0 || ext2.compare("gif") == 0)
		weight2 = 4;
	else
		weight2 = 5;

	if (weight1 == weight2)
		return m1.fname.compare(m2.fname) < 0;

	return weight1 < weight2;
}

string ReadTP4::cp1250ToUTF8(const string& str)
{
	string out;

	for (size_t j = 0; j < str.length(); j++)
	{
		int i = -1;
		unsigned char ch = str.at(j);
		short utf = -1;

		if (ch < 0x80)
		{
			do
			{
				i++;

				if (__cht[i].ch == ch)
				{
					utf = __cht[i].byte;
					break;
				}
			}
			while (__cht[i].ch != 0xff);

			if (utf < 0)
				utf = ch;
		}
		else
			utf = ch;

		if (utf > 0x00ff)
		{
			out.push_back((utf >> 8) & 0x00ff);
			out.push_back(utf & 0x00ff);
		}
		else if (ch > 0x7f)
		{
			out.push_back(0xc0 | ch >> 6);
			out.push_back(0x80 | (ch & 0x3f));
		}
		else
			out.push_back(ch);
	}

	return out;
}
