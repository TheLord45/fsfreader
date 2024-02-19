#include <iostream>
#include <fstream>
#include <cstring>

#include <openssl/err.h>
#include <openssl/evp.h>

#include "scramble.h"
#include "utils.h"
#include "fsfreader.h"

using std::string;
using std::ifstream;
using std::ios_base;
using std::ios;
using std::exception;
using std::min;
using std::cout;
using std::cerr;
using std::endl;

#define CHUNK_SIZE          1024
#define OSSL_SUCCESS        1
#define OSSL_ERROR          0

Scramble::Scramble()
{
    mCtx = EVP_CIPHER_CTX_new();

    if (!mCtx)
        cerr << "Error getting new context!" << endl;
}

Scramble::~Scramble()
{
    if (mCtx)
    {
        EVP_CIPHER_CTX_cleanup(mCtx);
        EVP_CIPHER_CTX_free(mCtx);
    }
}

bool Scramble::aesInit(const string& key, const string& salt, bool encrypt)
{
    if (mAesInitialized)
        return true;

    if (!mCtx)
    {
        cerr << "ERROR: No context available! Initialisation failed!" << endl;
        return false;
    }

    int keySize;
    int count = 5;      // Number iterations

    const EVP_MD *md = EVP_sha1();
    const EVP_CIPHER *pCipher = EVP_aes_128_cbc();

    if (!md)
    {
        cerr << "Error getting SHA1 hash function!" << endl;
        return false;
    }

    if (!pCipher)
    {
        cerr << "Error getting the AES128-CBC cipher algorithm!" << endl;
        return false;
    }

    // If the given salt is less then AES128_SALT_SIZE bytes, we first initialize
    // the buffer with 0 bytes. Then the salt is copied into the buffer. This
    // guaranties that we have a padding.
    memset(mAesSalt, 0, AES128_SALT_SIZE);
    memcpy(mAesSalt, salt.c_str(), min((size_t)AES128_SALT_SIZE, salt.length()));
    // Initialize the key and IV with 0
    memset(mAesKey, 0, AES128_KEY_SIZE);
    memset(mAesIV, 0, AES128_KEY_SIZE);

    keySize = EVP_BytesToKey(pCipher, md, mAesSalt, (unsigned char *)key.c_str(), key.length(), count, mAesKey, mAesIV);

    if (keySize == AES128_KEY_SIZE)
    {
        EVP_CIPHER_CTX_init(mCtx);

        if (encrypt)
        {
            if (EVP_EncryptInit_ex(mCtx, pCipher, nullptr, mAesKey, mAesIV) != OSSL_SUCCESS)
            {
                cerr << "Error initializing decrypting!" << endl;
                return false;
            }
        }
        else
        {
            if (EVP_DecryptInit_ex(mCtx, pCipher, nullptr, mAesKey, mAesIV) != OSSL_SUCCESS)
            {
                cerr << "Error initializing decrypting!" << endl;
                return false;
            }
        }

        EVP_CIPHER_CTX_set_key_length(mCtx, AES128_KEY_SIZE);
    }
    else
    {
        cerr << "Key size is " << (keySize * 8) << " bits - should be 128 bits" << endl;
        return false;
    }

    mAesInitialized = true;
    return mAesInitialized;
}

bool Scramble::aesDecodeFile(const string& fname)
{
    if (fname.empty())
    {
        cerr << "Got no file name to open a file!" << endl;
        return false;
    }

    ifstream ifile;

    try
    {
        ifile.open(fname, ios::binary);
        bool state = aesDecodeFile(ifile);
        ifile.close();
        return state;
    }
    catch(exception& e)
    {
        cerr << "Error opening file \"" << fname << "\": " << e.what() << endl;

        if (ifile.is_open())
            ifile.close();
    }

    return false;
}

bool Scramble::aesDecodeFile(ifstream& is)
{
    if (!is.is_open())
    {
        cerr << "No open file!" << endl;
        return false;
    }

    if (!mAesInitialized || !mCtx)
    {
        cerr << "Class was not initialized!" << endl;
        return false;
    }

    mDecrypted.clear();
    // Find file size
    is.seekg(0, ios_base::end);         // Seek to end of file
    size_t fileSize = is.tellg();       // Get current position (file size)
    is.seekg(0);                        // Seek to first byte of file
    size_t pos = 0;                     // Position (offset) in input and output buffer
    // Allocate space for input and output buffers
    unsigned char *buffer = new unsigned char[fileSize];
    unsigned char *outBuffer = new unsigned char[fileSize];
    unsigned char decBuffer[CHUNK_SIZE];    // Buffer for decoding a chunk.
    unsigned char encBuffer[CHUNK_SIZE];    // Buffer for decoding a chunk.
    int len = 0;
    // Not really necessary, but initialize output buffer with zeros
    memset(outBuffer, 0, fileSize);
    // Read whole file
    is.read(reinterpret_cast<char *>(buffer), fileSize);
    // decode
    if (fileSize > CHUNK_SIZE)     // Is the file size greater then a chunk?
    {                               // Yes, then start reading the file in chunks
        size_t numBlocks = fileSize / CHUNK_SIZE;

        for (size_t i = 0; i < numBlocks; ++i)
        {
            memcpy(encBuffer, buffer + i * CHUNK_SIZE, CHUNK_SIZE);

            if (EVP_DecryptUpdate(mCtx, decBuffer, &len, encBuffer, CHUNK_SIZE) != OSSL_SUCCESS)
            {
                cerr << "Loop " << i << ": Error updating" << endl;
                ERR_print_errors_fp(stderr);
                delete[] buffer;
                delete[] outBuffer;
                return false;
            }

            memcpy(outBuffer+pos, decBuffer, len);
            pos += len;
        }

        int size2 = fileSize - (numBlocks * CHUNK_SIZE);

        if (size2 > 0)      // Is there something left of the file less then CHUNK_SIZE?
        {                   // Yes, then decrypt it
            memcpy(encBuffer, buffer + numBlocks * CHUNK_SIZE, size2);

            if (EVP_DecryptUpdate(mCtx, decBuffer, &len, encBuffer, size2) != OSSL_SUCCESS)
            {
                cerr << "Error updating" << endl;
                ERR_print_errors_fp(stderr);
                delete[] buffer;
                delete[] outBuffer;
                return false;
            }

            memcpy(outBuffer+pos, decBuffer, len);
            pos += len;
        }
    }
    else
    {
        if (EVP_DecryptUpdate(mCtx, outBuffer, &len, buffer, fileSize) != OSSL_SUCCESS)
        {
            ERR_print_errors_fp(stderr);
            delete[] buffer;
            delete[] outBuffer;
            return false;
        }

        pos = len;
    }

    if (EVP_DecryptFinal_ex(mCtx, outBuffer + pos, &len) != OSSL_SUCCESS)
    {
        cerr << "Error finalizing" << endl;

        if (verbose)
        {
            cout << "DEBUG: pos: " << pos << ", len: " << len << endl;
            cout << "DEBUG: " << formatHex(outBuffer, 24) << endl;
        }

        ERR_print_errors_fp(stderr);
        delete[] buffer;
        delete[] outBuffer;
        return false;
    }

    pos += len;
    mDecrypted.assign(reinterpret_cast<char *>(outBuffer), pos);
    delete[] buffer;
    delete[] outBuffer;

    return true;
}

bool Scramble::aesEncodeFile(const string& fname)
{
    if (fname.empty())
    {
        cerr << "Got no file name to open a file!" << endl;
        return false;
    }

    ifstream ifile;

    try
    {
        ifile.open(fname, ios::binary);
        bool state = aesEncodeFile(ifile);
        ifile.close();
        return state;
    }
    catch(exception& e)
    {
        cerr << "Error opening file \"" << fname << "\": " << e.what() << endl;

        if (ifile.is_open())
            ifile.close();
    }

    return false;
}

bool Scramble::aesEncodeFile(ifstream& is)
{
    if (!is.is_open())
    {
        cerr << "No open file!" << endl;
        return false;
    }

    if (!mAesInitialized || !mCtx)
    {
        cerr << "Class was not initialized!" << endl;
        return false;
    }

    mDecrypted.clear();
    // Find file size
    is.seekg(0, ios_base::end);         // Seek to end of file
    size_t fileSize = is.tellg();       // Get current position (file size)
    is.seekg(0);                        // Seek to first byte of file
    size_t pos = 0;                     // Position (offset) in input and output buffer
    // Allocate space for input and output buffers
    unsigned char *buffer = new unsigned char[fileSize];
    unsigned char *outBuffer = new unsigned char[fileSize*2];
    unsigned char decBuffer[CHUNK_SIZE];    // Buffer for decoding a chunk.
    unsigned char encBuffer[CHUNK_SIZE*2];  // Buffer for decoding a chunk.
    int len = 0;
    // Not really necessary, but initialize output buffer with zeros
    memset(outBuffer, 0, fileSize);
    // Read whole file
    is.read(reinterpret_cast<char *>(buffer), fileSize);
    // encrypt
    if (fileSize > CHUNK_SIZE)     // Is the file size greater then a chunk?
    {                               // Yes, then start reading the file in chunks
        size_t numBlocks = fileSize / CHUNK_SIZE;

        for (size_t i = 0; i < numBlocks; ++i)
        {
            memcpy(decBuffer, buffer + i * CHUNK_SIZE, CHUNK_SIZE);

            if (EVP_EncryptUpdate(mCtx, encBuffer, &len, decBuffer, CHUNK_SIZE) != OSSL_SUCCESS)
            {
                cerr << "Loop " << i << ": Error updating" << endl;
                ERR_print_errors_fp(stderr);
                delete[] buffer;
                delete[] outBuffer;
                return false;
            }

            memcpy(outBuffer+pos, encBuffer, len);
            pos += len;
        }

        int size2 = fileSize - (numBlocks * CHUNK_SIZE);

        if (size2 > 0)      // Is there a is rest of the file less then CHUNK_SIZE?
        {                   // Yes, then decrypt it
            memcpy(decBuffer, buffer + numBlocks * CHUNK_SIZE, CHUNK_SIZE);

            if (EVP_EncryptUpdate(mCtx, encBuffer, &len, decBuffer, size2) != OSSL_SUCCESS)
            {
                cerr << "Error updating" << endl;
                ERR_print_errors_fp(stderr);
                delete[] buffer;
                delete[] outBuffer;
                return false;
            }

            memcpy(outBuffer+pos, encBuffer, len);
            pos += len;
        }
    }
    else
    {
        if (EVP_EncryptUpdate(mCtx, outBuffer, &len, buffer, fileSize) != OSSL_SUCCESS)
        {
            ERR_print_errors_fp(stderr);
            delete[] buffer;
            delete[] outBuffer;
            return false;
        }

        pos = len;
    }

    if (EVP_EncryptFinal_ex(mCtx, outBuffer + pos, &len) != OSSL_SUCCESS)
    {
        cerr << "Error finalizing" << endl;
        cout << "DEBUG: " << formatHex(outBuffer, 24) << endl;
        ERR_print_errors_fp(stderr);
        delete[] buffer;
        delete[] outBuffer;
        return false;
    }

    pos += len;
    mDecrypted.assign(reinterpret_cast<char *>(outBuffer), pos);
    delete[] buffer;
    delete[] outBuffer;

    return true;
}
