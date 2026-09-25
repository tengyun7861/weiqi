# 弈境围棋（C++ / Qt 6）

弈境围棋是一套面向本机和局域网的围棋对战系统，采用 C++20、Qt 6、CMake 与 SQLite 实现。运行入口为 `server-cpp` 与 `client-qt`。

源代码保留在 `E:\围棋`；请通过 ASCII Junction `E:\weiqi-cmake` 构建：

```powershell
cmake -S E:\weiqi-cmake -B E:\weiqi-cmake\build-msvc -G "Visual Studio 18 2026" -A x64 -DQt6_DIR=D:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6
cmake --build E:\weiqi-cmake\build-msvc --config Release
ctest --test-dir E:\weiqi-cmake\build-msvc -C Release --output-on-failure
```

启动服务端：

```powershell
.\build-msvc\server-cpp\Release\weiqi_server.exe .\data\weiqi.sqlite3 8081
```

部署、局域网防火墙、SQLite 备份和 KataGo 配置见[部署说明](docs/deployment.md)。
