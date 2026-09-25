# 部署说明

## 构建与启动

源代码与构建产物统一位于 `E:\weiqi`，可直接配置：

```powershell
cmake -S E:\weiqi -B E:\weiqi\build-msvc -G "Visual Studio 18 2026" -A x64 -DQt6_DIR=D:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6
cmake --build E:\weiqi\build-msvc --config Release
ctest --test-dir E:\weiqi\build-msvc -C Release --output-on-failure
```

服务端默认端口为 `8081`：

```powershell
.\build-msvc\server-cpp\Release\weiqi_server.exe .\data\weiqi.sqlite3 8081
```

客户端默认连接 `http://127.0.0.1:8081`；局域网客户端应填写服务端主机的 IPv4 地址，例如 `http://192.168.1.10:8081`。

## Windows 局域网部署

管理员 PowerShell 放行服务端端口：

```powershell
New-NetFirewallRule -DisplayName "弈境围棋 8081" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 8081 -Profile Private
```

SQLite 使用 WAL 模式。停服后备份主数据库文件；运行时备份必须同时处理 `-wal` 和 `-shm`，推荐使用 SQLite 在线备份 API。

## KataGo

设置 `WEIQI_KATAGO_COMMAND` 为完整的本地 GTP 启动命令，例如：

```powershell
$env:WEIQI_KATAGO_COMMAND = 'D:\KataGo\katago.exe gtp -config D:\KataGo\gtp.cfg -model D:\KataGo\model.bin.gz'
```

服务端管理 KataGo 子进程，并通过 `/api/v1/katago` 返回状态。未配置或启动失败时，客户端会明确提示原因，不会生成伪造着法或分析结果。
