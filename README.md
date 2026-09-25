# 弈境围棋（C++20 / Qt 6）

弈境围棋是一个面向本机与局域网使用的围棋对战工程。它将棋盘规则、账号和对局状态放在 C++ 服务端统一维护，并提供 Qt Quick 桌面客户端。工程可以用于两名玩家对弈、公开房间观战、快速匹配、与本地 KataGo 对弈，以及完成对局后的 SGF 复盘。

> 当前仓库是可继续开发的 C++ 重构工程，而不是已经发布的线上服务。构建会下载 CMake 依赖；部署前仍应完成目标环境的联机、KataGo 与防火墙验收。

## 能力范围

- **中国规则棋盘**：支持 9、13、19 路棋盘，落子合法性、提子、自杀禁着与简单劫由服务端规则引擎判定；中国数子计分使用 7.5 贴目。
- **对局流程**：注册和登录、创建公开或私有房间、房间码加入、断线离开后的座位保留、认输、双 pass 结束和读秒计时。
- **实时协作**：REST 用于账号、房间列表、棋谱等请求；WebSocket 用于落子、聊天、匹配、房间快照和观战事件。服务端是唯一可信状态源。
- **多人房间**：房间有黑白席位与观战模式；公开房间可列出，快速匹配按棋盘尺寸配对。
- **人机与复盘**：可按环境变量启动本地 KataGo GTP；完成的对局保存为 SGF，客户端可按手数前进、后退和跳转复盘。
- **本地持久化**：SQLite 保存用户、会话、房间、对局、着法、聊天和匹配队列，启用 WAL 与外键约束。

## 工程组成

```text
E:\weiqi
├── server-cpp/             C++20 服务端、棋规、SQLite 与 KataGo GTP 管理
│   ├── include/weiqi/      对外头文件：棋盘、对局、鉴权、数据库、SGF、API
│   ├── src/                Crow HTTP/WebSocket 服务与领域实现
│   └── tests/              棋规、服务核心、GTP、REST 与 WebSocket 测试
├── client-qt/              Qt 6 Quick/QML 桌面客户端
│   ├── qml/Main.qml        桌面交互界面
│   └── src/                网络、对局控制器和 SGF 复盘控制器
├── docs/                   部署与局域网运行说明
└── CMakeLists.txt          根构建入口
```

服务端按“规则核心 → 对局服务 → 网络 API”分层：`GoBoard` 不依赖网络和 UI；`GameService` 负责座位、时钟、持久化与事件；Crow 将 REST/WebSocket 请求转换为服务调用。Qt 客户端只显示服务端快照，不直接修改在线棋局。

## 环境要求

| 项目 | 要求 |
| --- | --- |
| 操作系统 | Windows 10/11 x64 |
| 编译器 | Visual Studio 2026 / MSVC（C++20） |
| CMake | 3.24 或更新版本 |
| Qt | Qt 6.6+；当前开发环境为 `D:\Qt\6.11.2\msvc2022_64` |
| 网络 | 首次配置需要访问 GitHub 与 sqlite.org 下载固定版本依赖 |
| 可选 AI | 本地 KataGo 可执行文件、GTP 配置和模型文件 |

源码路径已经是 ASCII 路径，源码和构建输出直接放在 `E:\weiqi`，不需要 Junction 或额外映射目录。

## 构建与测试

在 PowerShell 中从任意位置执行：

```powershell
cmake -S E:\weiqi -B E:\weiqi\build-msvc -G "Visual Studio 18 2026" -A x64 -DQt6_DIR=D:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6
cmake --build E:\weiqi\build-msvc --config Debug
ctest --test-dir E:\weiqi\build-msvc -C Debug --output-on-failure
```

生成目录 `build-msvc` 已被 Git 忽略，可随时删除后重新配置。构建脚本会锁定 Crow、Asio、nlohmann/json 和 SQLite amalgamation 的版本，以减少不同机器之间的依赖漂移。

## 启动与体验流程

先启动服务端；第一个参数是数据库位置，第二个参数是端口：

```powershell
Set-Location E:\weiqi
.\build-msvc\server-cpp\Debug\weiqi_server.exe .\data\weiqi.sqlite3 8081
```

再启动客户端：

```powershell
.\build-msvc\client-qt\Debug\weiqi_client.exe
```

默认服务地址为 `http://127.0.0.1:8081`。局域网使用时，将客户端中的服务地址改为运行服务端电脑的 IPv4 地址，例如 `http://192.168.1.10:8081`；并按[部署说明](docs/deployment.md)放行 TCP 8081。

最小验证流程是：注册两个不同账户 → 一方创建公开房间 → 另一方从大厅加入 → 双方落子或 pass → 在对局记录中打开 SGF 复盘。KataGo 未配置时，人机入口会返回明确的不可用原因，不会伪造 AI 着法。

## 服务端接口与实时协议

所有 REST 返回遵循如下包络：

```json
{ "ok": true, "data": {}, "error": null }
```

主要 HTTP 端点：

| 方法 | 路径 | 作用 |
| --- | --- | --- |
| GET | `/api/v1/health` | 服务健康状态 |
| GET | `/api/v1/katago` | KataGo 可用性与原因 |
| POST | `/api/v1/auth/register` | 注册账号 |
| POST | `/api/v1/auth/login` | 登录并取得 Bearer token |
| GET / POST | `/api/v1/rooms` | 获取公开房间 / 创建房间 |
| POST | `/api/v1/rooms/{roomCode}/join` | 加入房间 |
| POST | `/api/v1/rooms/{roomCode}/leave` | 离开房间 |
| GET | `/api/v1/games` | 当前用户的已结束对局记录 |
| GET | `/api/v1/games/{id}/sgf` | 读取 SGF 棋谱 |
| POST | `/api/v1/ai/games` | 创建 KataGo 人机房间 |

实时连接位于 `/ws`。客户端先发送 `auth`，随后可发送 `match.join`、`room.join`、`spectate.join`、`game.move`、`game.pass`、`game.resign` 和 `chat.send`。消息包含 `type`、可选 `requestId`、`payload` 和递增 `sequence`；服务端拒绝重复或倒退的请求序号。

## 数据与安全边界

- 密码经过哈希保存，登录令牌以哈希形式保存在 `sessions` 表并设为 24 小时过期。
- SQLite 的主要表为 `users`、`sessions`、`rooms`、`games`、`moves`、`chat_messages` 和 `matchmaking_queue`。
- 在线棋局的落子、计时、结束、战绩和 SGF 均由服务端生成；客户端传入的坐标、房间与实时序号都会再次校验。
- SQLite 使用 WAL。运行中复制数据库时必须连同 `-wal` 与 `-shm` 文件处理；停服后再做单文件备份最稳妥。

## KataGo（可选）

为启用人机对局，在启动服务端前设置完整的 GTP 命令：

```powershell
$env:WEIQI_KATAGO_COMMAND = 'D:\KataGo\katago.exe gtp -config D:\KataGo\gtp.cfg -model D:\KataGo\model.bin.gz'
```

服务端启动后可访问 `/api/v1/katago` 核对状态。模型、配置与可执行文件都不纳入仓库。

## 更多说明

- [服务端说明](server-cpp/README.md)：依赖、进程参数与服务端模块。
- [客户端说明](client-qt/README.md)：Qt 模块与桌面端构建入口。
- [部署说明](docs/deployment.md)：局域网端口、数据库备份与 KataGo 配置。
- [许可证](LICENSE)。
