#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>

unsigned char *stringToByte(const std::string &str);
std::string formatHex(unsigned char *key, int len);
std::string bytesToString(unsigned char *key, int len);
unsigned char *bytesToHexString(unsigned char *str, size_t len);

#endif
