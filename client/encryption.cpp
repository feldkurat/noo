#include "encryption.h"
#include <assert.h>

#ifdef TARGET_WIN
# include <inttypes.h>
#endif

BlowfishCipher::BlowfishCipher()
{

}

BlowfishCipher::~BlowfishCipher()
{

}

void BlowfishCipher::setKey(const QByteArray &ba)
{
  mKey = ba;
  BF_set_key(&mContext, ba.count(), (unsigned char*)ba.data());
}

const QByteArray& BlowfishCipher::key() const
{
  return mKey;
}

void BlowfishCipher::encrypt(const QByteArray &plain, QByteArray &encrypted)
{
  QByteArray plainProxy(plain);
  if (plainProxy.length() % 8)
    plainProxy.resize((8 - plainProxy.length() % 8) + plainProxy.length());
  encrypted.resize(plainProxy.length());

  BF_cbc_encrypt((unsigned char*)plainProxy.data(), (unsigned char*)encrypted.data(), plainProxy.length(), &mContext, (unsigned char*)mIV.data(), BF_ENCRYPT);
}

void BlowfishCipher::decrypt(const QByteArray &encrypted, QByteArray &plain)
{
  assert(encrypted.length() % 8 == 0);
  plain.resize(encrypted.length());
  BF_cbc_encrypt((unsigned char*)encrypted.data(), (unsigned char*)plain.data(), encrypted.length(), &mContext, (unsigned char*)mIV.data(), BF_DECRYPT);
}

// ----------- Twofish encryption ---------
TwofishCipher::TwofishCipher()
  :mContext(nullptr)
{
  mContext = new Twofish();
}

TwofishCipher::~TwofishCipher()
{
  delete mContext; mContext = nullptr;
}

void TwofishCipher::setKey(const QByteArray &ba)
{
  mKey = ba;
  mContext->PrepareKey((Twofish_Byte*)ba.data(), ba.count(), &mKeyStruct);
}

const QByteArray& TwofishCipher::key() const
{
  return mKey;
}

void TwofishCipher::setIV(const QByteArray &ba)
{
  assert(ba.size() == 16);
  mIV = ba;
}

const QByteArray& TwofishCipher::IV() const
{
  return mIV;
}

void TwofishCipher::encrypt(const QByteArray &plain, int plainOffset, QByteArray &encrypted, int encryptedOffset)
{
  assert((encrypted.length() - encryptedOffset) % 16 == 0);

  // Prepare XOR template
  uint8_t xorTemplate[16];
  memcpy(xorTemplate, mIV.data(), 16);

  // Iterate blocks
  int blockCount = (plain.size() - plainOffset) / 16;
  for (int i=0; i<blockCount; i++)
  {
    uint8_t* blockInput = (uint8_t*)plain.data() + i * 16 + plainOffset;
    uint8_t* blockOutput = (uint8_t*)encrypted.data() + i * 16 + encryptedOffset;

    // Xor block at first
    uint8_t xored[16];
    for (int xorIndex = 0; xorIndex < 16; xorIndex++)
      xored[i] = blockInput[xorIndex] ^ xorTemplate[xorIndex];

    // Encrypt block
    mContext->Encrypt(&mKeyStruct, xored, blockOutput);

    // Replace XOR template
    memcpy(xorTemplate, blockOutput, 16);
  }

  // Encrypt tail if it exists
  int tail = (plain.size() - plainOffset) % blocksize();
  if (tail)
  {
    uint8_t* blockInput = (uint8_t*)plain.data() + blockCount * 16 + plainOffset;
    uint8_t* blockOutput = (uint8_t*)encrypted.data() + blockCount * 16 + encryptedOffset;

    // Xor block at first
    uint8_t xored[16];
    for (int xorIndex = 0; xorIndex < 16; xorIndex++)
    {
      if (xorIndex < plain.size())
        xored[xorIndex] = blockInput[xorIndex] ^ xorTemplate[xorIndex];
      else
        xored[xorIndex] = xorTemplate[xorIndex];
    }
    // Encrypt block
    mContext->Encrypt(&mKeyStruct, xored, blockOutput);

    // Replace XOR template
    memcpy(xorTemplate, blockOutput, 16);
  }
}

void TwofishCipher::decrypt(const QByteArray &encrypted, int encryptedOffset, QByteArray &plain, int plainOffset)
{
  assert(encrypted.size() - encryptedOffset == plain.size() - plainOffset);
  assert((encrypted.size() - encryptedOffset) % 16 == 0);

  // Prepare XOR template
  uint8_t xorTemplate[16];
  memcpy(xorTemplate, mIV.data(), 16);

  // Iterate blocks
  int blockCount = encrypted.size() / 16;
  for (int i=0; i<blockCount; i++)
  {
    uint8_t* blockInput = (uint8_t*)encrypted.data() + i * 16 + encryptedOffset;
    uint8_t* blockOutput = (uint8_t*)plain.data() + i * 16 + plainOffset;

    // Decrypt at first
    mContext->Decrypt(&mKeyStruct, blockInput, blockOutput);

    // Xor block
    for (int xorIndex = 0; xorIndex < 16; xorIndex++)
      blockOutput[i] = blockOutput[xorIndex] ^ xorTemplate[xorIndex];

    // Replace XOR template
    memcpy(xorTemplate, blockInput, 16);
  }
}



// ----------- SHA256 wrapper -------------

SHA256::SHA256()
{
  SHA256_Init(&mContext);
  mDigest.resize(SHA256_DIGEST_LENGTH);
}

SHA256::~SHA256()
{

}

void SHA256::update(const void *data, int length)
{
  SHA256_Update(&mContext, data, length);
}

void SHA256::final()
{
  SHA256_Final((unsigned char*)mDigest.data(), &mContext);
}

// ----------- IV ----------------

void IV::Generate(QByteArray &buffer)
{
  uint8_t* data = (uint8_t*)buffer.data();
  for (int i=0; i<buffer.length(); i++)
    data[i] = rand() % 256;
}
