#include "ncmcrypt.hpp"
#include "qmccrypt.hpp"
#include <cstdio>
#include <iostream>
#include <string>

using namespace std;

string qmcFilename(const string &filename);

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("无输入文件。\n");
    return 1;
  }
  try {
    int i;
    for (i = 1; i < argc; i++) {

      string filename = argv[i];
      auto pos = filename.rfind('.');
      auto base = filename.substr(0, pos);
      auto ext = filename.substr(pos + 1);

      if (pos == string::npos) {
        printf("无扩展名，跳过:%s。\n", filename.c_str());
      } else if (ext == "mp3" || ext == "flac") {

        printf("无需转换：%s。\n", filename.c_str());

      } else if (ext == "ncm") {
        NcmDecrypt ncmcrypt(argv[i]);
        ncmcrypt.Decrypt();
        ncmcrypt.FixMetadata();

        printf("NCM解密输出：%s。\n", ncmcrypt.DecryptFilepath().c_str());

      } else if (ext == "qmc0" || ext == "qmc3" || ext == "qmcflac") {

        bool result = QmcDecrypt(argv[i], qmcFilename(argv[i]));

        if (!result) {
          return 1;
        } else {
          printf("QMC解密输出：%s。\n", qmcFilename(argv[i]).c_str());
        }
      } else {
        printf("不支持的扩展名，跳过：%s。\n", filename.c_str());
      }
    }

  } catch (std::invalid_argument err) {
    printf("错误详情：%s。\n", err.what());
  } catch (...) {
    printf("错误。");
  }
}

string qmcFilename(const string &filename) {

  auto pos = filename.rfind('.');

  auto base = filename.substr(0, pos);
  auto ext = filename.substr(pos + 1);
  if (ext == "qmcflac")
    ext = "flac";
  else if (ext == "qmc0" || ext == "qmc3")
    ext = "mp3";
  return base + '.' + ext;
}
