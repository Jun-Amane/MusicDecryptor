#include "ncmcrypt.hpp"
#include "aes.hpp"
#include "base64.hpp"
#include "cJSON.hpp"

#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>

#include <stdexcept>
#include <string>

const unsigned char NcmDecrypt::sCoreKey[17] = {
    0x68, 0x7A, 0x48, 0x52, 0x41, 0x6D, 0x73, 0x6F, 0x35,
    0x6B, 0x49, 0x6E, 0x62, 0x61, 0x78, 0x57, 0};
const unsigned char NcmDecrypt::sModifyKey[17] = {
    0x23, 0x31, 0x34, 0x6C, 0x6A, 0x6B, 0x5F, 0x21, 0x5C,
    0x5D, 0x26, 0x30, 0x55, 0x3C, 0x27, 0x28, 0};

const unsigned char NcmDecrypt::mPng[8] = {0x89, 0x50, 0x4E, 0x47,
                                           0x0D, 0x0A, 0x1A, 0x0A};

static void aesEcbDecrypt(const unsigned char *key, std::string &src,
                          std::string &dst) {
  int n, i;

  unsigned char out[16];

  n = src.length() >> 4;

  dst.clear();

  AES aes(key);

  for (i = 0; i < n - 1; i++) {
    aes.decrypt((unsigned char *)src.c_str() + (i << 4), out);
    dst += std::string((char *)out, 16);
  }

  aes.decrypt((unsigned char *)src.c_str() + (i << 4), out);
  char pad = out[15];
  if (pad > 16) {
    pad = 0;
  }
  dst += std::string((char *)out, 16 - pad);
}

static void replace(std::string &str, const std::string &from,
                    const std::string &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

static std::string removeExt(const std::string &str) {
  size_t lastExt = str.find_last_of(".");
  return str.substr(0, lastExt);
}

NeteaseMusicMetadata::~NeteaseMusicMetadata() { cJSON_Delete(mRaw); }

NeteaseMusicMetadata::NeteaseMusicMetadata(cJSON *raw) {
  if (!raw) {
    return;
  }

  cJSON *swap;
  int artistLen, i;

  mRaw = raw;

  swap = cJSON_GetObjectItem(raw, "musicName");
  if (swap) {
    mName = std::string(cJSON_GetStringValue(swap));
  }

  swap = cJSON_GetObjectItem(raw, "album");
  if (swap) {
    mAlbum = std::string(cJSON_GetStringValue(swap));
  }

  swap = cJSON_GetObjectItem(raw, "artist");
  if (swap) {
    artistLen = cJSON_GetArraySize(swap);

    for (i = 0; i < artistLen - 1; i++) {
      mArtist += std::string(cJSON_GetStringValue(
          cJSON_GetArrayItem(cJSON_GetArrayItem(swap, i), 0)));
      mArtist += "/";
    }
    mArtist += std::string(cJSON_GetStringValue(
        cJSON_GetArrayItem(cJSON_GetArrayItem(swap, i), 0)));
  }

  swap = cJSON_GetObjectItem(raw, "bitrate");
  if (swap) {
    mBitrate = swap->valueint;
  }

  swap = cJSON_GetObjectItem(raw, "duration");
  if (swap) {
    mDuration = swap->valueint;
  }

  swap = cJSON_GetObjectItem(raw, "format");
  if (swap) {
    mFormat = std::string(cJSON_GetStringValue(swap));
  }
}

bool NcmDecrypt::openFile(std::string const &path) {
  try {
    mFile.open(path, std::ios::in | std::ios::binary);
  } catch (...) {
    return false;
  }
  return true;
}

bool NcmDecrypt::isNcmFile() {
  unsigned int header;

  mFile.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (header != (unsigned int)0x4e455443) {
    return false;
  }

  mFile.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (header != (unsigned int)0x4d414446) {
    return false;
  }

  return true;
}

int NcmDecrypt::read(char *s, std::streamsize n) {
  mFile.read(s, n);

  int gcount = mFile.gcount();

  if (gcount <= 0) {
    throw std::invalid_argument("无法读取文件。");
  }

  return gcount;
}

void NcmDecrypt::buildKeyBox(unsigned char *key, int keyLen) {
  int i;
  for (i = 0; i < 256; ++i) {
    mKeyBox[i] = (unsigned char)i;
  }

  unsigned char swap = 0;
  unsigned char c = 0;
  unsigned char last_byte = 0;
  unsigned char key_offset = 0;

  for (i = 0; i < 256; ++i) {
    swap = mKeyBox[i];
    c = ((swap + last_byte + key[key_offset++]) & 0xff);
    if (key_offset >= keyLen)
      key_offset = 0;
    mKeyBox[i] = mKeyBox[c];
    mKeyBox[c] = swap;
    last_byte = c;
  }
}

std::string NcmDecrypt::mimeType(std::string &data) {
  if (memcmp(data.c_str(), mPng, 8) == 0) {
    return std::string("image/png");
  }

  return std::string("image/jpeg");
}

void NcmDecrypt::FixMetadata() {
  if (mDecryptFilepath.length() <= 0) {
    throw std::invalid_argument("未进行解密的文件。");
  }

  TagLib::File *audioFile;
  TagLib::Tag *tag;
  TagLib::ByteVector vector(mImageData.c_str(), mImageData.length());

  if (mFormat == NcmDecrypt::MP3) {
    audioFile = new TagLib::MPEG::File(mDecryptFilepath.c_str());
    tag = dynamic_cast<TagLib::MPEG::File *>(audioFile)->ID3v2Tag(true);

    if (mImageData.length() > 0) {
      TagLib::ID3v2::AttachedPictureFrame *frame =
          new TagLib::ID3v2::AttachedPictureFrame;

      frame->setMimeType(mimeType(mImageData));
      frame->setPicture(vector);

      dynamic_cast<TagLib::ID3v2::Tag *>(tag)->addFrame(frame);
    }
  } else if (mFormat == NcmDecrypt::FLAC) {
    audioFile = new TagLib::FLAC::File(mDecryptFilepath.c_str());
    tag = audioFile->tag();

    if (mImageData.length() > 0) {
      TagLib::FLAC::Picture *cover = new TagLib::FLAC::Picture;
      cover->setMimeType(mimeType(mImageData));
      cover->setType(TagLib::FLAC::Picture::FrontCover);
      cover->setData(vector);

      dynamic_cast<TagLib::FLAC::File *>(audioFile)->addPicture(cover);
    }
  }

  if (mMetaData != NULL) {
    tag->setTitle(TagLib::String(mMetaData->name(), TagLib::String::UTF8));
    tag->setArtist(TagLib::String(mMetaData->artist(), TagLib::String::UTF8));
    tag->setAlbum(TagLib::String(mMetaData->album(), TagLib::String::UTF8));
  }

  tag->setComment(TagLib::String("Decrypted by music file Decrypt tool.",
                                 TagLib::String::UTF8));

  audioFile->save();
}

void NcmDecrypt::Decrypt() {
  int n, i;

  mDecryptFilepath = removeExt(mFilepath);

  n = 0x8000;

  unsigned char buffer[n];

  std::ofstream output;

  while (!mFile.eof()) {
    n = read((char *)buffer, n);

    for (i = 0; i < n; i++) {
      int j = (i + 1) & 0xff;
      buffer[i] ^=
          mKeyBox[(mKeyBox[j] + mKeyBox[(mKeyBox[j] + j) & 0xff]) & 0xff];
    }

    if (!output.is_open()) {

      if (buffer[0] == 0x49 && buffer[1] == 0x44 && buffer[2] == 0x33) {
        mDecryptFilepath += ".mp3";
        mFormat = NcmDecrypt::MP3;
      } else {
        mDecryptFilepath += ".flac";
        mFormat = NcmDecrypt::FLAC;
      }

      output.open(mDecryptFilepath, output.out | output.binary);
    }

    output.write((char *)buffer, n);
  }

  output.flush();
  output.close();
}

NcmDecrypt::~NcmDecrypt() {
  if (mMetaData != NULL) {
    delete mMetaData;
  }

  mFile.close();
}

NcmDecrypt::NcmDecrypt(std::string const &path) {
  if (!openFile(path)) {
    throw std::invalid_argument("无法打开文件。");
  }

  if (!isNcmFile()) {
    throw std::invalid_argument("非网易云音乐加密格式。");
  }

  if (!mFile.seekg(2, mFile.cur)) {
    throw std::invalid_argument("无法索取文件。");
  }

  mFilepath = path;

  int i;

  unsigned int n;
  read(reinterpret_cast<char *>(&n), sizeof(n));

  if (n <= 0) {
    throw std::invalid_argument("文件损坏。");
  }

  char keydata[n];
  read(keydata, n);

  for (i = 0; i < n; i++) {
    keydata[i] ^= 0x64;
  }

  std::string rawKeyData(keydata, n);
  std::string mKeyData;

  aesEcbDecrypt(sCoreKey, rawKeyData, mKeyData);

  buildKeyBox((unsigned char *)mKeyData.c_str() + 17, mKeyData.length() - 17);

  read(reinterpret_cast<char *>(&n), sizeof(n));

  if (n <= 0) {
    printf("[警告] `%s` metadata缺失！\n", path.c_str());

    mMetaData = NULL;
  } else {
    char modifyData[n];
    read(modifyData, n);

    for (i = 0; i < n; i++) {
      modifyData[i] ^= 0x63;
    }

    std::string swapModifyData;
    std::string modifyOutData;
    std::string modifyDecryptData;

    swapModifyData = std::string(modifyData + 22, n - 22);

    Base64::Decode(swapModifyData, modifyOutData);

    aesEcbDecrypt(sModifyKey, modifyOutData, modifyDecryptData);

    modifyDecryptData =
        std::string(modifyDecryptData.begin() + 6, modifyDecryptData.end());

    mMetaData =
        new NeteaseMusicMetadata(cJSON_Parse(modifyDecryptData.c_str()));
  }

  if (!mFile.seekg(9, mFile.cur)) {
    throw std::invalid_argument("无法索要文件。");
  }

  read(reinterpret_cast<char *>(&n), sizeof(n));

  if (n > 0) {
    char *imageData = (char *)malloc(n);
    read(imageData, n);

    mImageData = std::string(imageData, n);
  } else {
    printf("[警告] `%s` 专辑封面缺失！\n", path.c_str());
  }
}
