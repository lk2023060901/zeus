#!/bin/bash

# Zeus网络类批量重命名脚本
# 用法: ./rename_network_classes.sh

echo "开始批量重命名Zeus网络类..."

# 项目根目录
PROJECT_ROOT="/Volumes/work/coding/c++/zeus"
cd "$PROJECT_ROOT"

# 定义需要替换的文件类型
FILE_PATTERN="\( -name '*.h' -o -name '*.cpp' -o -name '*.hpp' -o -name '*.cxx' \)"

echo "正在替换类名..."

# 使用find和sed进行批量替换
# 替换类名
find . $FILE_PATTERN -exec sed -i '' 's/TcpServer/TcpAcceptor/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/TcpConnection/TcpConnector/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/KcpServer/KcpAcceptor/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/KcpConnection/KcpConnector/g' {} +

echo "正在替换头文件包含..."

# 替换头文件包含
find . $FILE_PATTERN -exec sed -i '' 's/#include "tcp_server\.h"/#include "tcp_acceptor.h"/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/#include "tcp_connection\.h"/#include "tcp_connector.h"/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/#include "kcp_server\.h"/#include "kcp_acceptor.h"/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/#include "kcp_connection\.h"/#include "kcp_connector.h"/g' {} +

# 替换系统级包含（如果有）
find . $FILE_PATTERN -exec sed -i '' 's/#include <tcp_server\.h>/#include <tcp_acceptor.h>/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/#include <tcp_connection\.h>/#include <tcp_connector.h>/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/#include <kcp_server\.h>/#include <kcp_acceptor.h>/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/#include <kcp_connection\.h>/#include <kcp_connector.h>/g' {} +

echo "正在替换工厂方法名称..."

# 替换工厂方法
find . $FILE_PATTERN -exec sed -i '' 's/CreateTcpServer/CreateTcpAcceptor/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/CreateTcpClient/CreateTcpConnector/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/CreateKcpServer/CreateKcpAcceptor/g' {} +
find . $FILE_PATTERN -exec sed -i '' 's/CreateKcpClient/CreateKcpConnector/g' {} +

echo "正在更新CMakeLists.txt文件..."

# 更新CMakeLists.txt中的源文件列表
find . -name "CMakeLists.txt" -exec sed -i '' 's/tcp_server\.cpp/tcp_acceptor.cpp/g' {} +
find . -name "CMakeLists.txt" -exec sed -i '' 's/tcp_connection\.cpp/tcp_connector.cpp/g' {} +
find . -name "CMakeLists.txt" -exec sed -i '' 's/kcp_server\.cpp/kcp_acceptor.cpp/g' {} +
find . -name "CMakeLists.txt" -exec sed -i '' 's/kcp_connection\.cpp/kcp_connector.cpp/g' {} +

find . -name "CMakeLists.txt" -exec sed -i '' 's/tcp_server\.h/tcp_acceptor.h/g' {} +
find . -name "CMakeLists.txt" -exec sed -i '' 's/tcp_connection\.h/tcp_connector.h/g' {} +
find . -name "CMakeLists.txt" -exec sed -i '' 's/kcp_server\.h/kcp_acceptor.h/g' {} +
find . -name "CMakeLists.txt" -exec sed -i '' 's/kcp_connection\.h/kcp_connector.h/g' {} +

echo "正在重命名测试文件..."

# 重命名测试文件
if [ -f "tests/network/tcp/test_tcp_server.cpp" ]; then
    mv "tests/network/tcp/test_tcp_server.cpp" "tests/network/tcp/test_tcp_acceptor.cpp"
fi

if [ -f "tests/network/tcp/test_tcp_connection.cpp" ]; then
    mv "tests/network/tcp/test_tcp_connection.cpp" "tests/network/tcp/test_tcp_connector.cpp"
fi

if [ -f "tests/network/kcp/test_kcp_server.cpp" ]; then
    mv "tests/network/kcp/test_kcp_server.cpp" "tests/network/kcp/test_kcp_acceptor.cpp"
fi

if [ -f "tests/network/kcp/test_kcp_connection.cpp" ]; then
    mv "tests/network/kcp/test_kcp_connection.cpp" "tests/network/kcp/test_kcp_connector.cpp"
fi

echo "批量重命名完成！"
echo "请检查编译是否成功，并手动调整任何遗漏的引用。"