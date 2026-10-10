# 弈境网页前端

React / TypeScript / Vite。负责页面、SVG 棋盘、输入、动画、HTTP 与浏览器原生 WebSocket。运行时业务后端全部在 `server-cpp`；本目录没有服务端 API 或规则引擎。

```powershell
npm.cmd ci
npm.cmd run dev
npm.cmd run build
npm.cmd run lint
npm.cmd run format:check
```

开发代理目标是 `127.0.0.1:8081`；生产构建输出 `dist/`，由 C++ 服务同源提供。浏览器无需安装 Qt 或本机客户端。

- `pages/`：页面组装，大厅、对局、AI 和棋谱分别一个文件。
- `components/`：棋盘、棋手、聊天、模态框与表单各自维护。
- `hooks/`：会话、连接、房间、对局、聊天、AI 与复盘各自拥有状态；`useWeiqi` 只组合这些模块及页面联动。
- `lib/`：HTTP 包络、实时连接、协议类型和结果显示。实时请求等待确认，断线重连恢复座位；只对棋局快照比较版本，聊天、鉴权、错误与确认不套用棋局版本。
- `styles/`：各页面和组件样式独立，共用控件放在 `shared.css`，基础字体和变量放在 `index.css`。

棋盘只渲染 C++ 返回的 `stones`；SGF 复盘只选择 C++ 生成的 `frames`。不要在前端重复实现合法着法、提子、计分、匹配或复盘计算。

SessionStorage 保存当前标签页的会话和房间，刷新后恢复；不在网页中保存密码。用户内容通过 React 文本节点显示，不解释为 HTML。原生 dialog 支持键盘关闭与焦点管理；棋盘支持箭头键与回车落子，界面支持减少动画偏好。
