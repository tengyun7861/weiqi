import { useState } from 'react'
import { Board } from '../components/Board'
import { Icon } from '../components/Icon'
import type { WeiqiModel } from '../hooks/useWeiqi'
export function Lobby({
  model,
  onLogin,
  onCreate,
}: {
  model: WeiqiModel
  onLogin: () => void
  onCreate: () => void
}) {
  const [code, setCode] = useState(''),
    [size, setSize] = useState(19)
  const ready = !!model.session && model.connected
  return (
    <>
      <section className="lobby-intro">
        <div className="intro-copy">
          <p className="intro-kicker">一方棋盘，一场相逢。</p>
          <h1>
            {model.session ? model.session.user.nickname + '，下一局见。' : '落下一子，打开一局。'}
          </h1>
          <p>
            与朋友对弈，去大厅遇见棋手。
            <br />
            从第一手到最后一手，把每一局好好留下。
          </p>
          <div className="intro-actions">
            <button className="button primary" onClick={model.session ? onCreate : onLogin}>
              <Icon name="add" />
              {model.session ? '创建一局棋' : '登录，开始对弈'}
            </button>
            <button className="button text-button" onClick={() => model.setPage('replay')}>
              查看我的棋谱
              <Icon name="arrow" />
            </button>
          </div>
          <div className="supported-sizes">
            <span>9 路</span>
            <span>13 路</span>
            <span>19 路</span>
            <span className="rule-note">中国数子规则</span>
          </div>
        </div>
        <div className="intro-board">
          <Board size={9} stones={Array(81).fill(0)} preview />
          <span className="preview-label">从空盘开始，每一手都是新的选择</span>
        </div>
      </section>
      <section className="start-strip">
        <div className="match-box">
          <div>
            <h2>寻找一位棋手</h2>
            <p>{model.matching ? '正在等待同样棋盘大小的对手…' : '选择棋盘，开始实时匹配。'}</p>
          </div>
          <div className="inline-actions">
            <select
              aria-label="匹配棋盘大小"
              value={size}
              onChange={(event) => setSize(Number(event.target.value))}
              disabled={model.matching}
            >
              <option value={19}>19 路</option>
              <option value={13}>13 路</option>
              <option value={9}>9 路</option>
            </select>
            {model.matching ? (
              <button
                className="button secondary"
                onClick={() => void model.cancelMatch()}
                disabled={model.busy}
              >
                取消匹配
              </button>
            ) : (
              <button
                className="button primary"
                disabled={!ready || model.busy}
                onClick={() => void model.match(size)}
              >
                开始匹配
              </button>
            )}
          </div>
        </div>
        <form
          className="invite-box"
          onSubmit={(event) => {
            event.preventDefault()
            void model.join(code)
          }}
        >
          <div>
            <h2>赴朋友的棋局</h2>
            <p>输入六位房间码，坐到棋盘另一侧。</p>
          </div>
          <div className="inline-actions">
            <input
              aria-label="房间码"
              value={code}
              onChange={(event) => setCode(event.target.value.toUpperCase())}
              maxLength={6}
              placeholder="六位房间码"
              required
            />
            <button className="button secondary" disabled={!ready || model.busy}>
              加入房间
            </button>
          </div>
        </form>
      </section>
      <section className="room-section">
        <div className="section-heading">
          <div>
            <h2>公开棋局</h2>
            <p>加入等待中的房间，或观看正在进行的对局。</p>
          </div>
          <button
            className="button subtle"
            onClick={() => void model.loadRooms()}
            disabled={!model.session}
          >
            <Icon name="refresh" />
            刷新大厅
          </button>
        </div>
        {!model.session ? (
          <div className="empty-state">
            <Icon name="lobby" size={34} />
            <h3>登录后，棋局为你展开</h3>
            <p>公开房间、朋友邀请与快速匹配，都从一个棋手账号开始。</p>
            <button className="button secondary" onClick={onLogin}>
              登录或注册
            </button>
          </div>
        ) : !model.rooms.length ? (
          <div className="empty-state">
            <Icon name="board" size={34} />
            <h3>大厅还没有公开棋局</h3>
            <p>创建一个房间，让下一位棋手与你相遇。</p>
            <button className="button secondary" onClick={onCreate} disabled={!ready}>
              创建公开房间
            </button>
          </div>
        ) : (
          <div className="room-list">
            {model.rooms.map((room) => (
              <article className="room-row" key={room.roomCode}>
                <div className="room-icon">
                  <Icon name="board" size={26} />
                </div>
                <div className="room-description">
                  <h3>
                    {room.boardSize} 路棋局{' '}
                    <span className={'room-status ' + room.status}>
                      {room.status === 'waiting' ? '等待棋手' : '正在对弈'}
                    </span>
                  </h3>
                  <span className="room-code">{room.roomCode}</span>
                </div>
                <div className="inline-actions">
                  {room.status === 'waiting' && (
                    <button
                      className="button primary small"
                      disabled={!ready || model.busy}
                      onClick={() => void model.join(room.roomCode)}
                    >
                      加入对弈
                    </button>
                  )}
                  <button
                    className="button secondary small"
                    disabled={!ready || model.busy}
                    onClick={() => void model.join(room.roomCode, true)}
                  >
                    <Icon name="eye" size={17} />
                    观战
                  </button>
                </div>
              </article>
            ))}
          </div>
        )}
      </section>
    </>
  )
}
