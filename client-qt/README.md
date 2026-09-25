# 弈境围棋 Qt 6 客户端

中文桌面界面使用 Qt 6 Quick 编写，工程仅使用 CMake。当前包含大厅导航、19 路棋盘、落子交互和对局信息面板；下一步会接入 C++ 服务端的房间与实时对局接口。

本机已安装 Qt 6.11.2 的 MinGW 套件。若需要由 Visual Studio 编译，请在 Qt Maintenance Tool 中额外安装与本机 VS 匹配的 `MSVC 2022/2026 64-bit` 套件；Qt 的 MinGW 库不能与 MSVC 链接器混用。Visual Studio 可以直接打开本工程的 `CMakeLists.txt`，但当前这台机器的 Qt 库实际由 MinGW 编译。

当前可构建命令：

```powershell
# 已建立 ASCII 路径别名，避免 Qt 的 qmlimportscanner 误处理中文绝对路径
cmake -S E:\weiqi-cmake\client-qt -B E:\weiqi-cmake\client-qt\build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:\Qt\6.11.2\mingw_64"
cmake --build E:\weiqi-cmake\client-qt\build
```
