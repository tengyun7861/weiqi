# 弈境围棋 C++20 服务端

这是围棋系统的正式 C++20 服务端：Crow REST/WebSocket、SQLite、服务端权威棋规和本地 KataGo GTP 均在此目录实现。

在 Windows 的中文目录下，使用 MinGW Makefiles 时采用快速目标，避免生成器对 Unicode 绝对路径重复依赖扫描：

```powershell
cmake -S server-cpp -B server-cpp\build -G "MinGW Makefiles"
cmake --build server-cpp\build --target weiqi_rules_test/fast
ctest --test-dir server-cpp\build --output-on-failure
```

从根 CMake 工程构建 `weiqi_server`。默认监听 `0.0.0.0:8081`，启动形式为：

```powershell
weiqi_server.exe .\data\weiqi.sqlite3 8081
```

详情见 [`../docs/cpp-qt-migration.md`](../docs/cpp-qt-migration.md)。
