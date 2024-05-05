cmake_minimum_required(VERSION 2.5)
project(mudou)

# mudou最终编译成so动态库、设置动态库的路径、放在项目目录的根目录下的lib
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#调试信息
set(CMAEK_CXX_FLAGS "${CMAKE_CXX_FLAGS } -g -std=c++11 -fPIC")

# 定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)


# 编译生成动态库

add_library(mudou SHARED ${SRC_LIST})
