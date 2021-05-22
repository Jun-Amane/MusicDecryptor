cmake_minimum_required(VERSION 3.16) #cmake最低版本
project("MusicDecryptor") #Project name
AUX_SOURCE_DIRECTORY(src DIR_SRCS)
SET(CMAKE_CXX_FLAGS "-ltag")#编译选项，非紧急情况不要动。
set(SRC_LISTS ${DIR_SRCS})#将源文件整理进一个变量，方便管理。文件之间用半角分号分割。
add_executable(MusicDecryptor ${SRC_LISTS})#同上。