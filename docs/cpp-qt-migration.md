# C++ / Qt 6 迁移说明

## 运行入口

源代码保留在 `E:\围棋`。由于 Qt/MSVC 的部分生成工具对中文绝对路径并不可靠，必须从 ASCII Junction `E:\weiqi-cmake` 配置和构建：

```powershell
cmake -S E:\weiqi-cmake -B E:\weiqi-cmake\build-msvc -G "Visual Studio 18 2026" -A x64 -DQt6_DIR=D:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6
cmake --build E:\weiqi-cmake\build-msvc --config Debug
ctest --test-dir E:\weiqi-cmake\build-msvc -C Debug --output-on-failure
```

请将生成器名替换成安装的 Visual Studio 版本。根工程只构建 `server-cpp`、`client-qt` 和测试；所有 MSVC 目标都使用 `/utf-8` 与 `/FS`。

服务端默认端口为 `8081`：

```powershell
.\build-msvc\server-cpp\Debug\weiqi_server.exe .\data\weiqi.sqlite3 8081
```

客户端默认连接 `http://127.0.0.1:8081`（实时连接会自动改用 `ws://127.0.0.1:8081/ws`）。局域网客户端应填主机的 IPv4 地址，例如 `http://192.168.1.10:8081`。

## Windows 局域网部署

管理员 PowerShell 放行服务端端口：

```powershell
New-NetFirewallRule -DisplayName "弈境围棋 8081" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 8081 -Profile Private
```

SQLite 使用 WAL 模式。停服后备份主数据库文件；运行时备份必须同时处理 `-wal` 和 `-shm`，推荐先调用 SQLite 的在线备份 API，不能只复制主文件。

## KataGo

设置 `WEIQI_KATAGO_COMMAND` 为完整的本地 GTP 启动命令，而不只是可执行文件路径，例如：

```powershell
$env:WEIQI_KATAGO_COMMAND = 'D:\KataGo\katago.exe gtp -config D:\KataGo\gtp.cfg -model D:\KataGo\model.bin.gz'
```

服务端独占管理 KataGo 子进程，并通过 `/api/v1/katago` 返回中文状态。未配置或启动失败时，客户端人机入口会禁用并展示原因；不会生成伪造着法或分析结果。

## 已实现的运行契约

- 服务端权威执行中国数子规则、提子、禁入点、简单劫、连续停一手、认输、基本时间加每手读秒，以及 SGF 持久化。
- SQLite 自动创建 `users`、`sessions`、`rooms`、`games`、`moves`、`chat_messages` 和匹配队列表；密码为 PBKDF2-SHA256 强哈希，令牌只保存 SHA-256 摘要。
- REST 前缀为 `/api/v1`，所有响应均为 `{ ok, data, error }`；WebSocket 为 `ws://主机:8081/ws`，认证、房间、匹配、对局、聊天和观战事件均为 JSON 文本帧。
- 实时客户端在断线后使用同一令牌重新认证并自动重新进入原房间；服务端恢复座位、棋盘、棋钟及最近 100 条聊天记录。客户端请求采用单调递增序号，服务端拒绝重复或倒退序号。
- Qt 客户端支持大厅、房间码、快速匹配、对局聊天、观战、人机可用状态和标准 SGF 的可视逐手回放。

`goweb/`、`goweb-spring/` 仍只作为历史迁移参照。待完整双客户端局域网验收及 Release 构建完成后，才会统一归档或移出最终源码交付目录。
