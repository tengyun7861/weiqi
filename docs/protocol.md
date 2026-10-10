# HTTP 与实时协议

HTTP JSON 包络：`{ "ok": true, "data": {}, "error": null }`。鉴权使用 `Authorization: Bearer <token>`，所有业务逻辑由 C++ 执行。

| 方法 | 路径 | 功能 |
| --- | --- | --- |
| GET | `/api/v1/health` | 后端健康、版本与实现语言 |
| GET | `/api/v1/katago` | 实际引擎状态 |
| POST | `/api/v1/auth/register` | 注册 `{nickname,password}` |
| POST | `/api/v1/auth/login` | 返回 `{token,user:{id,nickname},expiresIn}` |
| GET | `/api/v1/auth/me` | 当前会话棋手 |
| POST | `/api/v1/auth/logout` | 撤销令牌 |
| GET / POST | `/api/v1/rooms` | 公开房间 / 创建房间 |
| POST | `/api/v1/rooms/{code}/join`、`/leave` | HTTP 房间操作；网页以实时加入建立订阅 |
| GET | `/api/v1/games` | 当前棋手已结束的棋谱记录 |
| GET | `/api/v1/games/{id}/sgf` | 自己的棋谱文本 |
| GET | `/api/v1/games/{id}/replay` | C++ 生成的逐手棋盘帧 |
| POST | `/api/v1/replay/import` | `{sgf}`；C++ 校验并生成复盘 |
| POST | `/api/v1/ai/games` | `{boardSize}`；引擎就绪时创建人机局 |

## WebSocket

同源 `/ws`；最大消息 16 KiB，浏览器 Origin 校验当前 Host。连接后先发送：

```json
{"type":"auth","requestId":"auth","sequence":1,"payload":{"token":"<token>"}}
```

随后每个请求使用连接内递增的 `sequence` 和唯一 `requestId`。重复或倒退序号被拒绝。支持 `room.join`、`spectate.join`、`room.leave`、`match.join`、`match.cancel`、`game.move`、`game.pass`、`game.resign`、`chat.send` 与 `ai.retry`。

操作成功返回同一 `requestId` 的 `request.ok` 或带请求标识的直接快照；错误返回 `error`。广播棋局状态事件包括 `room.snapshot`、`game.updated`、`clock.updated`、`game.finished`、`match.found`。

棋局快照含 `roomCode`、`boardSize`、`stones`、`currentPlayer`、双方用户名与 ID、基本时间与读秒、`moveCount`、`lastMove`、结束结果、原因和版本。`stones` 为行优先的 0 / 1 / 2，分别表示空、黑、白。

棋局版本只比较同一房间的快照，重连时清空本地版本基线。鉴权、聊天、错误、匹配等待和请求确认始终处理。加入房间发送 `chat.history`，旧协议兼容消息带 `historical:true`，新网页不重复添加这些消息。

重连后重新鉴权并加入保存的房间，服务器恢复玩家座位和棋盘。客户端不自动重发未确认的落子，避免重复操作；页面显示重连状态并禁用落子，恢复后以服务端快照为准。退出登录会撤销服务端会话。
