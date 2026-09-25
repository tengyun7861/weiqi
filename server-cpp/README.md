# 弈境围棋 C++20 服务端

这是围棋系统的正式 C++20 服务端：Crow REST/WebSocket、SQLite、服务端权威棋规和本地 KataGo GTP 均在此目录实现。

请从根 CMake 工程直接配置：

```powershell
cmake -S E:\weiqi -B E:\weiqi\build-msvc -G "Visual Studio 18 2026" -A x64 -DQt6_DIR=D:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6
cmake --build E:\weiqi\build-msvc --config Release --target weiqi_server
```

从根 CMake 工程构建 `weiqi_server`。默认监听 `0.0.0.0:8081`，启动形式为：

```powershell
weiqi_server.exe .\data\weiqi.sqlite3 8081
```

详情见[部署说明](../docs/deployment.md)。
