import { useState } from 'react'
import { Board } from '../components/Board'
import { Icon } from '../components/Icon'
import type { WeiqiModel } from '../hooks/useWeiqi'
export function Ai({ model, onLogin }: { model: WeiqiModel; onLogin: () => void }) {
  const [size, setSize] = useState(9)
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="intro-kicker">给自己一位耐心的对手。</p>
          <h1>与 KataGo 下一局。</h1>
          <p>从小棋盘练习，到完整一局。你执黑，KataGo 执白。</p>
        </div>
      </div>
      <div className="ai-layout">
        <section className="ai-settings">
          <span className={'engine-status ' + (model.ai.available ? 'available' : '')}>
            <span />
            {model.ai.available ? '对弈引擎已就绪' : '对弈引擎尚未就绪'}
          </span>
          <h2>选一方棋盘</h2>
          <div className="size-options">
            {[9, 13, 19].map((value) => (
              <button
                key={value}
                className={size === value ? 'selected' : ''}
                onClick={() => setSize(value)}
              >
                <strong>{value} 路</strong>
                <span>{value === 9 ? '轻松开局' : value === 13 ? '中盘练习' : '完整对弈'}</span>
              </button>
            ))}
          </div>
          <p className="ai-description">
            中国数子规则，白棋贴 7.5。对局结束后，棋谱会自动保存，供你逐手回看。
          </p>
          {!model.ai.available && (
            <div className="engine-empty">
              <h3>AI 对手暂未上线</h3>
              <p>网站管理员需要配置 KataGo 引擎与模型。你现在可以去大厅与朋友对弈。</p>
              <button className="button text-button" onClick={() => model.setPage('lobby')}>
                前往棋局大厅
                <Icon name="arrow" />
              </button>
            </div>
          )}
          {model.session ? (
            <button
              className="button primary wide"
              disabled={!model.ai.available || !model.connected || model.busy}
              onClick={() => void model.createAi(size)}
            >
              {model.busy ? '正在准备棋局…' : '开始人机对弈'}
              <Icon name="arrow" />
            </button>
          ) : (
            <button className="button primary wide" onClick={onLogin}>
              登录后开始
            </button>
          )}
          <button className="button subtle wide" onClick={() => void model.loadAi()}>
            <Icon name="refresh" />
            检查引擎状态
          </button>
        </section>
        <div className="ai-board">
          <Board key={size} size={size} stones={Array(size * size).fill(0)} preview />
          <span className="preview-label">选好棋盘，从第一手开始。</span>
        </div>
      </div>
    </>
  )
}
