/*
 * Copyright (C) 2019, 2024 by Andreas Theofilu <andreas@theosys.at>
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

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include "fsfreader.h"
#include "readtp4.h"

namespace fs = std::filesystem;
using namespace std;

string prgName;
bool verbose;
bool transfer;
string password = "8P0puxB5OVUFI6uX";
string salt = "MarkRobs";

void usage();
void version();

int main (int argc, char **argv)
{
	string fname, outdir;

	prgName.assign(*argv);
	size_t pos = prgName.find_last_of("/");

	if (pos != string::npos)
		prgName = prgName.substr(pos+1);

	verbose = transfer = false;
	version();

	outdir.assign(".");
	argc--;
	argv++;

	while (argc > 0)
	{
		string par(*argv);

		if (par.compare("-f") == 0 || par.compare("--file") == 0)
		{
			if (argc > 1)
			{
				argc--;
				argv++;
				fname.assign(*argv);
			}
			else
			{
				usage();
				exit (1);
			}
		}

		if (par.compare("-d") == 0 || par.compare("--directory") == 0)
		{
			if (argc > 1)
			{
				argc--;
				argv++;
				outdir.assign(*argv);
			}
			else
			{
				usage();
				exit(1);
			}
		}

		if (par.compare("-t") == 0 || par.compare("--transfer") == 0)
			transfer = true;

		if (par.compare("-v") == 0 || par.compare("--verbose") == 0)
			verbose = true;

		if (par.compare("-h") == 0 || par.compare("--help") == 0)
		{
			usage();
			exit(0);
		}

		if (par.compare("-a") == 0 || par.compare("--alternate") == 0)
		{
			password = "6Brzh63P3wlAkTtl";
			salt = "M0rPh3u5";
			cout << "Using alternate password and salt." << endl << endl;
		}

		if (par.compare("-p") == 0 || par.compare("--password") == 0)
		{
			if (argc > 1)
			{
				argc--;
				argv++;
				password.assign(*argv);
				cout << "Using individual password." << endl << endl;
			}
			else
			{
				usage();
				return 1;
			}
		}

		if (par.compare("-s") == 0 || par.compare("--salt") == 0)
		{
			if (argc > 1)
			{
				argc--;
				argv++;
				salt.assign(*argv);
				cout << "Using individual salt." << endl << endl;
			}
			else
			{
				usage();
				return 1;
			}
		}

		argc--;
		argv++;
	}

	if (fname.length() == 0)
	{
		cerr << "Missing file name to read from." << endl << endl;
		usage();
		exit (1);
	}

	if (password.length() != 16)
	{
		cerr << "Password has wrong length of " << password.length() << " characters! Should be 16 characters long." << endl << endl;;
		usage();
		return 1;
	}

	if (salt.length() != 8)
	{
		cerr << "Salt has wrong length of " << salt.length() << " characters! Should be 8 characters long." << endl << endl;
		usage();
		return 1;
	}

	if (outdir.length() > 0 && !fs::exists(outdir))
		fs::create_directories(outdir);

	reader::ReadTP4 readtp4(fname, outdir);

	if (!readtp4.doRead())
		return 2;

	return 0;
}

void usage()
{
	cout << "Syntax: " << prgName << " -f <file> [-o <directory>]" << endl << endl;
	cout << "\t-f <file>          The file to read. This should be a file created by" << endl;
	cout << "\t--file <file>      TPDesign4. These file have the file extension \".tp4\"." << endl << endl;
	cout << "\t-d <directory>     Optional: The directory into where the output files" << endl;
	cout << "\t--directory <dir>  should be written. A TP4 file contains several files." << endl;
	cout << "\t                   If the directory does not exist, it will be created." << endl << endl;
	cout << "\t-t                 " << prgName << " creates the needed directory structure and moves all" << endl;
	cout << "\t--transfer         the extracted file into their right place. This means," << endl;
	cout << "\t                   that graphic files are moved into \"images\", sound files" << endl;
	cout << "\t                   into \"sounds\" and fonts into \"fonts\". All other files" << endl;
	cout << "\t                   remain in the main directory." << endl << endl;
	cout << "\t-a                 Use alternate password and salt for TP5 file. This may be" << endl;
	cout << "\t--alternate        useful if the file was repacked by TPControl." << endl << endl;
	cout << "\t-p <password>      Use individual password to decrypt TP5 file." << endl;
	cout << "\t--password <password>" << endl << endl;
	cout << "\t-s <salt>          Use individual salt to decrypt TP5 file." << endl;
	cout << "\t--salt <salt>" << endl << endl;
	cout << "\t-v                 Verbose; Thismakes the program very noisy. The program shows" << endl;
	cout << "\t--verbose          how it reads the internal block structure." << endl << endl;
	cout << "\t-h --help          This help." << endl << endl;
	cout << "HINT: In TP5 files all XML configuration files are encrypted. If this file was" << endl;
	cout << "      created by TPDesign5 the default password is necessary to decrypt the" << endl;
	cout << "      TP5 files. If the file was prepared by TPControl so that the license" << endl;
	cout << "      was added and maybe the file was renamed, then the alternate password" << endl;
	cout << "      should be used to decrypt the file." << endl;
	cout << "      If you created your own files with other password and salt, then you can" << endl;
	cout << "      set an individual password and salt to decrypt the files. However: You" << endl;
	cout << "      must use the following credentials to encrypt the files: " << endl;
	cout << "          Cipher  : AES-128-CBC" << endl;
	cout << "          Digest  : SHA1" << endl;
	cout << "          Iterate : 5" << endl;
	cout << "          Password: 16 characters (letters upper case/lower case and digits)" << endl;
	cout << "          Salt    : 8 characters (letters upper case/lower case and digits)" << endl << endl;
}

void version()
{
	cout << prgName << " v" << VERSION << endl;
	cout << "Copyright 2019, 2024 by Andreas Theofilu." << endl;
	cout << "GNU/GPLv3 http://www.gnu.org/licenses/gpl-3.0.html" << endl << endl;
}
