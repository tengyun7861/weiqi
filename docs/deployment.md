# 网站运行与部署

当前 C++ 服务端已在 Windows x64 / MSVC 上构建与验证。网页可由电脑或手机浏览器访问；服务端 Linux 移植需要替换 BCrypt / Windows 进程适配。

## 本机与局域网

```powershell
Set-Location E:\weiqi
.\scripts\build.ps1
.\scripts\start.ps1 -Port 8081
```

本机访问 `http://127.0.0.1:8081`。其他设备访问运行服务器电脑的局域网 IPv4 地址，例如 `http://192.168.1.10:8081`。如需在局域网访问，管理员自行为该服务器放行对应 TCP 端口；项目脚本不修改系统防火墙。

C++ 服务同时提供网页、`/api/v1/*` 和 `/ws`，浏览器连接当前网站的源。无需在页面中输入另一台后端地址。

## 可搬移的网站服务包

构建脚本会生成 `build-msvc/site-package/bin/`，内含 `weiqi_server.exe` 与 `web/`。把这一目录放在服务器上，运行：

```powershell
.\weiqi_server.exe .\data\weiqi.sqlite3 8081
```

网页目录自动取可执行文件旁的 `web/`。数据库不随服务包打包，模型、口令、会话与测试数据也不会进入包中。目标服务器需要兼容的 Microsoft Visual C++ x64 运行库。

## 公网接入

公网可在 C++ 服务前放 HTTPS 反向代理；代理需同时转发 HTTP API 和 WebSocket，保留客户端 `Host`，启用 Upgrade / Connection 转发，并把 WebSocket 读取超时配置为适合长连接的值。对外域名的网页、API 和 WebSocket 保持同源。C++ 服务限制在反向代理可达的内网或本机端口。

当前改造没有选择公网主机、域名或证书，也没有向外部平台发布。正式上线前需在目标机器验证 HTTPS、WebSocket 重连、真实 KataGo 模型、备份恢复与预期并发量；语言选择不能代替这些验收。

## 数据与备份

默认数据位于 `data/weiqi.sqlite3`。SQLite 使用 WAL、外键与忙等待；运行时可能有 `-wal` 与 `-shm` 文件。停服后做完整数据库备份，或使用 SQLite 正式备份接口；不要只复制正在写入的主数据库文件。

旧数据库表结构可迁移，原有账号、对局、着法和聊天仍由 C++ 读取。替换服务文件前保留数据库备份，服务包中的 `web/` 与后端应来自同一构建版本。

## 可选 KataGo

启动前设置 `WEIQI_KATAGO_COMMAND` 为完整 GTP 命令，路径有空格时为可执行文件和参数路径加引号。`WEIQI_KATAGO_TIMEOUT_MS` 默认为 30000，范围 100–300000 毫秒。状态端点为 `/api/v1/katago`。

模型未配置、启动失败或推理超时会明确报告；服务不会生成虚假的 AI 落子。引擎推理在后台队列中，每个任务完成前不切换至另一个房间。
