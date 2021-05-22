#pragma once

#include "aes.hpp"
#include "cJSON.hpp"

#include <fstream>
#include <iostream>

class NeteaseMusicMetadata {

private:
  std::string mAlbum;
  std::string mArtist;
  std::string mFormat;
  std::string mName;
  int mDuration;
  int mBitrate;

private:
  cJSON *mRaw;

public:
  NeteaseMusicMetadata(cJSON *);
  ~NeteaseMusicMetadata();
  const std::string &name() const { return mName; }
  const std::string &album() const { return mAlbum; }
  const std::string &artist() const { return mArtist; }
  const std::string &format() const { return mFormat; }
  const int duration() const { return mDuration; }
  const int bitrate() const { return mBitrate; }
};

class NcmDecrypt {

private:
  static const unsigned char sCoreKey[17];
  static const unsigned char sModifyKey[17];
  static const unsigned char mPng[8];
  enum NcmFormat { MP3, FLAC };

private:
  std::string mFilepath;
  std::string mDecryptFilepath;
  NcmFormat mFormat;
  std::string mImageData;
  std::ifstream mFile;
  unsigned char mKeyBox[256];
  NeteaseMusicMetadata *mMetaData;

private:
  bool isNcmFile();
  bool openFile(std::string const &);
  int read(char *s, std::streamsize n);
  void buildKeyBox(unsigned char *key, int keyLen);
  std::string mimeType(std::string &data);

public:
  const std::string &filepath() const { return mFilepath; }
  const std::string &DecryptFilepath() const { return mDecryptFilepath; }

public:
  NcmDecrypt(std::string const &);
  ~NcmDecrypt();

public:
  void Decrypt();
  void FixMetadata();
};