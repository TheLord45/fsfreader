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

#ifndef __READTP4__
#define __READTP4__

#include <string>
#include <cstdint>

namespace reader
{
	struct HEADER		// This is the first entry in the file.
	{
		unsigned char abyFileID[8];		// 0 - 7
		uint32_t listStartBlock;		// 8 - 11
	};

	struct BLOCK		// This is a block. It points to other blocks and contains the block data.
	{
		uint32_t thisBlock;				// 0 - 3
		uint32_t prevBlock;				// 4 - 7
		uint32_t nextBlock;				// 8 - 11
		uint16_t bytesUsed;				// 12 - 13
		unsigned char abyData[512];		// 14 - 525
	};

	struct USAGE_BLOCK	// This is an index entry
	{
		uint32_t thisBlock;				// 0 - 3
		uint32_t prevBlock;				// 4 - 7
		uint32_t nextBlock;				// 8 - 11
		uint16_t bytesUsed;				// 12 - 13
		unsigned char filePath[260];	// 14 - 273
		time_t tmCreate;				// 274 - 277
		time_t tmModify;				// 278 - 281
		uint32_t flags;					// 282 - 285
		uint32_t startBlock;			// 286 - 289
		uint32_t sizeBlocks;			// 290 - 293
		uint32_t sizeBytes;				// 294 - 297
	};

	struct FILE_HEAD	// This is the pointer part of a block.
	{
		uint32_t thisBlock;				// 228 - 231
		uint32_t prevBlock;				// 232 - 235
		uint32_t nextBlock;				// 236 - 239
		uint16_t blockLen;				// 240 - 141
	};

	struct INDEX
	{
		struct USAGE_BLOCK *ublock{nullptr};
		struct INDEX *prev{nullptr};
		struct INDEX *next{nullptr};
	};

	#define SIZE_HEADER			12
	#define SIZE_BLOCK			526
	#define SIZE_USAGE_BLOCK	298
	#define SIZE_FILE_HEAD		14

	class ReadTP4
	{
		std::string fname{""};
		std::string target{"."};
		struct INDEX *idx{nullptr};

		public:
			explicit ReadTP4(const std::string& fn)
				: fname{fn}
			{}

			ReadTP4(const std::string& fn, const std::string& tg)
				: fname{fn},
				  target{tg}
			{}

			~ReadTP4();

			bool doRead();
			std::string toHex(int num, int width);

		private:
			void fillBlock(struct BLOCK& bl, const unsigned char *buf);
			void fillUsageBlock(struct USAGE_BLOCK& ub, const unsigned char *buf);
			void fillFileHead(struct FILE_HEAD& fh, const unsigned char *buf);
			size_t calcBlockPos(uint32_t block);

			struct INDEX *appendUBlock(const struct USAGE_BLOCK *ub);
			void deleteIndex();

			uint32_t makeDWord(const unsigned char *buf);
			uint16_t makeWord(const unsigned char *buf);
			void dump(const unsigned char *buf, size_t size);
	};
}

#endif
