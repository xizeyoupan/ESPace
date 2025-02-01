# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# 获取操作系统类型
if(WIN32)
    # Windows: 使用 PowerShell 执行 Git 命令
    execute_process(
        COMMAND powershell -Command "git log -n 1 --pretty=format:'%h'"
        OUTPUT_VARIABLE GIT_COMMIT
    )
else()
    # Linux/macOS: 使用 Bash 执行 Git 命令
    execute_process(
        COMMAND bash -c "git log -n 1 --pretty=format:'%h'"
        OUTPUT_VARIABLE GIT_COMMIT
    )
endif()

# 去掉末尾的换行符
string(STRIP "${GIT_COMMIT}" GIT_COMMIT)

# 定义 Git 提交哈希值为宏
add_definitions(-DGIT_COMMIT_SHA1="${GIT_COMMIT}")

set(EXTRA_COMPONENT_DIRS ./components/)

include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(magic-wand)
