# 弈境围棋 Qt 6 客户端

中文桌面客户端使用 Qt 6 Quick/QML 与 C++20 编写，并由根 CMake 工程统一构建。客户端通过 REST 与 WebSocket 连接本机或局域网服务端。

使用 Qt 6.11.2 的 `msvc2022_64` 套件和 Visual Studio MSVC 工具链。Visual Studio 可直接打开根目录的 `CMakeLists.txt`。

构建命令：

```powershell
# 使用 ASCII 构建入口，避免 qmlimportscanner 误处理中文绝对路径
cmake -S E:\weiqi-cmake -B E:\weiqi-cmake\build-msvc -G "Visual Studio 18 2026" -A x64 -DQt6_DIR=D:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6
cmake --build E:\weiqi-cmake\build-msvc --config Release --target weiqi_client
```
