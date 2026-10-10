import { useRef } from 'react'
import { Board } from '../components/Board'
import { Icon } from '../components/Icon'
import type { WeiqiModel } from '../hooks/useWeiqi'
import { resultText } from '../lib/resultText'
export function Replay({ model, onLogin }: { model: WeiqiModel; onLogin: () => void }) {
  const fileInput = useRef<HTMLInputElement>(null)
  const replay = model.replay,
    step = model.replayStep,
    frame = replay?.frames[step]
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="intro-kicker">落子有迹，回看有所得。</p>
          <h1>把每一局，慢慢看清。</h1>
          <p>保存的对局与自己的 SGF 棋谱，都可以逐手回看。</p>
        </div>
        <button
          className="button secondary"
          onClick={() => fileInput.current?.click()}
          disabled={!model.session || model.busy}
        >
          <Icon name="upload" />
          导入 SGF 棋谱
        </button>
        <input
          ref={fileInput}
          type="file"
          accept=".sgf"
          hidden
          aria-label="导入棋谱文件"
          onChange={(event) => {
            const file = event.target.files?.[0]
            if (file) void model.importSgf(file)
            event.target.value = ''
          }}
        />
      </div>
      {!model.session ? (
        <div className="empty-state page-empty">
          <Icon name="book" size={40} />
          <h2>登录后，找回你的棋局</h2>
          <p>每场结束的对局会自动留下棋谱。</p>
          <button className="button primary" onClick={onLogin}>
            登录或注册
          </button>
        </div>
      ) : (
        <div className="replay-layout">
          <aside className="records-panel">
            <div className="panel-heading">
              <h2>我的棋谱</h2>
              <button
                className="icon-button"
                aria-label="刷新棋谱"
                onClick={() => void model.loadRecords()}
              >
                <Icon name="refresh" size={18} />
              </button>
            </div>
            {!model.records.length ? (
              <div className="records-empty">
                <p>还没有结束的对局</p>
                <span>去下一局棋，或导入自己的棋谱。</span>
              </div>
            ) : (
              model.records.map((record) => (
                <article className="record-row" key={record.id} data-testid={'record-' + record.id}>
                  <button
                    className="record-open"
                    onClick={() => void model.openReplay(record.id)}
                    disabled={model.busy}
                  >
                    <span>对局 #{record.id}</span>
                    <strong>{resultText(record.result)}</strong>
                    <small>
                      {new Date(
                        record.endedAt + (record.endedAt.includes('Z') ? '' : 'Z'),
                      ).toLocaleString('zh-CN', { hour12: false })}
                    </small>
                  </button>
                  <button
                    className="icon-button"
                    aria-label={'下载对局 ' + record.id + ' 的 SGF'}
                    onClick={() => void model.exportSgf(record.id)}
                  >
                    <Icon name="download" size={18} />
                  </button>
                </article>
              ))
            )}
          </aside>
          <section className="replay-workspace">
            {replay && frame ? (
              <>
                <div className="replay-title">
                  <h2>{model.replayTitle}</h2>
                  <span>
                    {replay.boardSize} 路 · 白贴 {replay.komi}
                    {replay.result ? ' · ' + resultText(replay.result) : ''}
                  </span>
                </div>
                <Board size={replay.boardSize} stones={frame.stones} lastMove={frame.lastMove} />
                <div className="replay-controls">
                  <button
                    className="icon-button"
                    aria-label="上一手"
                    disabled={step === 0}
                    onClick={() => model.setReplayStep(step - 1)}
                  >
                    <Icon name="back" />
                  </button>
                  <input
                    type="range"
                    aria-label="复盘手数"
                    min={0}
                    max={replay.moves.length}
                    value={step}
                    onChange={(event) => model.setReplayStep(Number(event.target.value))}
                  />
                  <button
                    className="icon-button"
                    aria-label="下一手"
                    disabled={step === replay.moves.length}
                    onClick={() => model.setReplayStep(step + 1)}
                  >
                    <Icon name="next" />
                  </button>
                  <span data-testid="replay-step">
                    {step} / {replay.moves.length} 手
                  </span>
                </div>
                <p className="replay-note">拖动进度条逐手回看，包含提子与停一手。</p>
              </>
            ) : (
              <div className="empty-state replay-empty">
                <Icon name="book" size={40} />
                <h2>从一局棋开始回看</h2>
                <p>选择左侧棋谱，或导入一个 SGF 文件。</p>
                <span className="form-note">支持 9、13、19 路主线棋谱。</span>
              </div>
            )}
          </section>
        </div>
      )}
    </>
  )
}
