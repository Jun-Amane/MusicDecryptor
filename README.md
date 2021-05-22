# MusicDecryptor
MusicDecryptor：用于将网易云音乐与QQ音乐下载的加密文件格式转换为普通格式。<br />
<br />
本仓库及文章仅以研究性学习记录为性质，以开拓思路与研究密码学、程式设计为目的。
<br />
使用方法：<br />
1. 编译：建立编译文件夹（如build）并进入；<br />
        cmake ..<br />
        make<br />
2. 运行：./MusicDecryptor [文件名（支持多个且多种格式混合）]。<br />
<br />
编译依赖于taglib。<br />
思路借鉴于github.com/anonymous5l/ncmdump、github.com/MegrezZhu/qmcdump。<br />
<br />
关于开源许可证：<br />
本程式按照MIT许可证开源。<br />