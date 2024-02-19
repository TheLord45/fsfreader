#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

#include "utils.h"

using std::string;
using std::cerr;
using std::endl;
using std::stringstream;
using std::setw;
using std::setfill;
using std::hex;
using std::dec;

unsigned char *stringToByte(const string &str)
{
    if (str.empty())
        return nullptr;

    size_t len = str.length() * 2;
    unsigned char *key = new unsigned char[len + 1];
    memset(key, 0, len);
    int pos = 0;

    for (size_t i = 0; i < str.length(); i += 2)
    {
        char *endptr;
        string part = str.substr(i, 2);
        long ch = strtol(part.c_str(), &endptr, 16);

        if (errno != 0)
        {
            cerr << "Error interpreting number " << part << " into hex: " << strerror(errno) << endl;
            delete[] key;
            return nullptr;
        }

        if (endptr == part.c_str())
        {
            cerr << "No digits were found!" << endl;
            delete[] key;
            return nullptr;
        }

        *(key + pos) = static_cast<int>(ch);
        pos++;
    }

    return key;
}

string formatHex(unsigned char *key, int len)
{
    if (!key || len < 1)
        return string();

    std::stringstream s;

    for (int i = 0; i < len; ++i)
    {
        int ch = *(key+i);
        s << " " << std::setw(2) << std::setfill('0') << std::hex << ch;
    }

    string out = s.str().substr(1);     // Cut off the first blank
    return out;
}

string bytesToString(unsigned char *key, int len)
{
    char ch[2];
    ch[1] = 0;
    string skey;

    for (int i = 0; i < len; ++i)
    {
        ch[0] = *(key+i);
        skey.append(ch);
    }

    return skey;
}

unsigned char *bytesToHexString(unsigned char *str, size_t len)
{
    if (!str || !len)
        return nullptr;

    unsigned char *buffer = new unsigned char[len * 2 + 1];
    memset(buffer, 0, len * 2 + 1);

    for (size_t i = 0; i < len; ++i)
    {
        stringstream s;
        s << setw(2) << setfill('0') << hex << static_cast<int>(*(str+i));
        strncat(reinterpret_cast<char *>(str), s.str().c_str(), 2);
    }

    return buffer;
}