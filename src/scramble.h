#ifndef __SCRAMBLE_H__
#define __SCRAMBLE_H__

#include <string>
#include <openssl/core.h>

#define AES128_KEY_SIZE     16
#define AES128_SALT_SIZE    8

class Scramble
{
    public:
        Scramble();
        ~Scramble();

        bool aesInit(const std::string& key, const std::string& salt, bool encrypt=false);
        bool aesDecodeFile(std::ifstream& is);
        bool aesDecodeFile(const std::string& fname);
        bool aesEncodeFile(std::ifstream& is);
        bool aesEncodeFile(const std::string& fname);
        unsigned char *getAesKey(size_t& len) { len = AES128_KEY_SIZE; return mAesKey; }
        unsigned char *getAesSalt(size_t& len) { len = AES128_SALT_SIZE; return mAesSalt; }
        unsigned char *getAesIV(size_t& len) { len = AES128_KEY_SIZE; return mAesIV; }
        std::string& getDecrypted() { return mDecrypted; }
        void aesReset() { mAesInitialized = false; }

    private:
        unsigned char mAesKey[AES128_KEY_SIZE];     // Key used for decryption/encryption
        unsigned char mAesSalt[AES128_SALT_SIZE];   // Salt for decryption/encryption
        unsigned char mAesIV[AES128_KEY_SIZE];      // Initialization Vector
        EVP_CIPHER_CTX *mCtx{nullptr};              // Context
        std::string mDecrypted;                     // Buffer for clear (decrypted) text
        bool mAesInitialized{false};                // TRUE if initialized
};

#endif
