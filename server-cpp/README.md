# C++20 网站后端

所有运行时后端功能都在本目录。Crow 提供 HTTP / WebSocket，SQLite 保存数据，Windows BCrypt 提供密码派生与令牌哈希；KataGo 通过子进程 GTP 接入。没有 JavaScript、Python 或 Qt 后端。

根 CMake 构建三个库目标和一个应用：`weiqi_rules`（棋规、SGF、复盘）、`weiqi_sqlite`（固定版本 SQLite）、`weiqi_server_core`（鉴权、领域服务、AI），以及 `weiqi_server`（网络适配和网页服务）。领域库不依赖 Crow。

## 构建与运行

```powershell
cmake --preset windows-msvc
cmake --build --preset debug
ctest --preset debug
.\build-msvc\server-cpp\Debug\weiqi_server.exe .\data\weiqi.sqlite3 8081 .\client-web\dist
```

三个参数依次为数据库路径、端口和网页目录。默认网页目录为本项目的 `client-web/dist`；安装包中若存在可执行文件旁的 `web/index.html`，自动使用该目录。

```powershell
cmake --install build-msvc --config Release --prefix build-msvc\site-package
```

安装结果为 `bin/weiqi_server.exe` 与 `bin/web/`。先构建前端，再执行安装。

## 实现归属

| 模块 | 实现位置 |
| --- | --- |
| 启动与组装 | `src/main.cpp`、`src/api_server.cpp` |
| REST 接口 | `src/api/*_routes.cpp`，每个功能单独注册 |
| 实时传输与鉴权 | `src/api/realtime_server.cpp` |
| 实时业务命令 | `src/api/realtime_rooms.cpp`、`realtime_matchmaking.cpp`、`realtime_gameplay.cpp`、`realtime_chat.cpp` |
| 棋局状态恢复 | `src/services/game_state_service.cpp` |
| 房间、对局、计时 | `src/services/rooms_service.cpp`、`gameplay_service.cpp`、`clock_service.cpp` |
| 领域持久化 | `src/services/persistence_service.cpp` |
| 匹配、聊天、记录 | 对应的 `src/services/*_service.cpp` |
| AI 对局与调度 | `src/services/ai_service.cpp`、`src/ai_worker.cpp` |
| 棋规、SGF、复盘 | `src/go_board.cpp`、`src/sgf.cpp`、`src/replay.cpp` |
| 账号与数据库 | `src/auth.cpp`、`src/database.cpp` |

`GameService` 门面维护棋局状态、互斥锁与领域事件。拆分实现文件共享这个状态所有者，避免把每个模块复制成另一份棋局状态。HTTP 与 WebSocket 调用同一个领域门面，规则验证只有一个来源。

## KataGo

```powershell
$env:WEIQI_KATAGO_COMMAND = 'D:\KataGo\katago.exe gtp -config D:\KataGo\gtp.cfg -model D:\KataGo\model.bin.gz'
$env:WEIQI_KATAGO_TIMEOUT_MS = '30000'
```

AI 队列串行执行完整的“清盘、重放该局着法、生成白棋着法”任务，避免不同房间交叉污染同一引擎。推理在后台线程执行；结果提交前再次检查轮次和结束状态。落子、停一手、认输与错误分别处理，错误不会变成停一手。stdout 只读 GTP，stderr 保留诊断；超时后终止引擎进程并发送错误事件。

鉴权令牌有效期 24 小时，数据库存令牌哈希，退出登录会撤销会话。WebSocket 每条业务命令重新校验会话、请求序号和玩家权限。静态资源仅开放白名单扩展，规范化路径限定在网页目录内。
