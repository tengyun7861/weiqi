import { useEffect, useState } from 'react'
import { AuthDialog } from './components/AuthDialog'
import { CreateRoomDialog } from './components/CreateRoomDialog'
import { Icon } from './components/Icon'
import { useWeiqi } from './hooks/useWeiqi'
import { Lobby } from './pages/Lobby'
import { Game } from './pages/Game'
import { Ai } from './pages/Ai'
import { Replay } from './pages/Replay'
import type { Page } from './lib/types'
import './App.css'

const navigation: { page: Page; label: string; icon: 'lobby' | 'board' | 'spark' | 'book' }[] = [
  { page: 'lobby', label: '棋局大厅', icon: 'lobby' },
  { page: 'game', label: '在线对弈', icon: 'board' },
  { page: 'ai', label: '人机对弈', icon: 'spark' },
  { page: 'replay', label: '棋谱库', icon: 'book' },
]
export default function App() {
  const model = useWeiqi()
  const [authOpen, setAuthOpen] = useState(false),
    [createOpen, setCreateOpen] = useState(false)
  useEffect(() => {
    if (!model.notice) return
    const timer = setTimeout(model.dismissNotice, 12000)
    return () => clearTimeout(timer)
  }, [model.notice, model.dismissNotice])
  const onLogin = () => setAuthOpen(true)
  return (
    <div className="app-shell">
      <header className="site-header">
        <div className="header-inner">
          <button
            className="brand"
            onClick={() => model.setPage('lobby')}
            aria-label="弈境围棋，返回大厅"
          >
            <span className="brand-mark">
              <i />
              <i />
            </span>
            <span>
              <strong>弈境</strong>
              <small>每一局，都值得留下</small>
            </span>
          </button>
          <nav aria-label="主导航">
            {navigation.map((item) => (
              <button
                key={item.page}
                onClick={() => model.setPage(item.page)}
                aria-current={model.page === item.page ? 'page' : undefined}
                className={model.page === item.page ? 'active' : ''}
              >
                <Icon name={item.icon} size={18} />
                {item.label}
              </button>
            ))}
          </nav>
          <div className="account-area">
            {model.session ? (
              <>
                <span
                  className={'connection-status ' + model.status}
                  data-testid="connection-status"
                >
                  <i />
                  {model.status === 'connected'
                    ? '实时连接'
                    : model.status === 'reconnecting'
                      ? '正在重连'
                      : model.status === 'connecting'
                        ? '连接中'
                        : '离线'}
                </span>
                <span className="account-name">{model.session.user.nickname}</span>
                <button className="logout-button" onClick={() => void model.logout()}>
                  退出
                </button>
              </>
            ) : (
              <button className="button primary small" onClick={onLogin}>
                登录 / 注册
              </button>
            )}
          </div>
        </div>
      </header>
      <main className="site-main">
        {model.page === 'lobby' ? (
          <Lobby model={model} onLogin={onLogin} onCreate={() => setCreateOpen(true)} />
        ) : model.page === 'game' ? (
          <Game model={model} />
        ) : model.page === 'ai' ? (
          <Ai model={model} onLogin={onLogin} />
        ) : (
          <Replay model={model} onLogin={onLogin} />
        )}
      </main>
      <footer className="site-footer">
        <span className="footer-brand">弈境围棋</span>
        <span>一方棋盘，认真相逢。</span>
        <span>9 / 13 / 19 路 · 中国数子规则</span>
      </footer>
      {model.notice && (
        <div className="notice" role="alert">
          <span>{model.notice}</span>
          <button className="icon-button" aria-label="关闭提示" onClick={model.dismissNotice}>
            <Icon name="close" size={18} />
          </button>
        </div>
      )}
      {authOpen && (
        <AuthDialog onClose={() => setAuthOpen(false)} onAuthenticated={model.authenticate} />
      )}
      {createOpen && <CreateRoomDialog model={model} onClose={() => setCreateOpen(false)} />}
    </div>
  )
}
