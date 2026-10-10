import { useState } from 'react'
import { PlayersPanel } from '../components/PlayersPanel'
import { ChatPanel } from '../components/ChatPanel'
import { resultText } from '../lib/resultText'
import { Board } from '../components/Board'
import { Icon } from '../components/Icon'
import { Modal } from '../components/Modal'
import type { WeiqiModel } from '../hooks/useWeiqi'

export function Game({ model }: { model: WeiqiModel }) {
  const [resigning, setResigning] = useState(false)
  const state = model.snapshot
  if (!state)
    return (
      <div className="empty-state page-empty">
        <Icon name="board" size={40} />
        <h1>棋盘在这里等你</h1>
        <p>从大厅创建棋局、加入朋友房间，或寻找一位棋手。</p>
        <button className="button primary" onClick={() => model.setPage('lobby')}>
          前往棋局大厅
          <Icon name="arrow" />
        </button>
      </div>
    )
  const waiting = !state.whiteUserId && !state.aiWhite
  const stateText = state.finished
    ? resultText(state.result)
    : waiting
      ? '等待另一位棋手加入'
      : !model.connected
        ? '正在恢复实时连接'
        : model.spectating
          ? '观战中 · ' + (state.currentPlayer === 'black' ? '黑棋' : '白棋') + '行棋'
          : model.canPlay
            ? '轮到你了，落下一子吧'
            : state.aiWhite && state.currentPlayer === 'white'
              ? 'KataGo 正在思考'
              : '等待对手行棋'
  return (
    <>
      <div className="page-heading game-heading">
        <div>
          <p className="intro-kicker">{model.spectating ? '一起看懂每一手' : '专注当下这一手'}</p>
          <h1>
            {state.boardSize} 路对弈{' '}
            <span className="heading-room-code" data-testid="current-room-code">
              {state.roomCode}
            </span>
          </h1>
        </div>
        <div className="inline-actions">
          <button
            className="button secondary small"
            onClick={() => {
              void navigator.clipboard
                ?.writeText(state.roomCode)
                .then(() => model.notify('房间码已复制：' + state.roomCode))
                .catch(() => model.notify('房间码：' + state.roomCode))
              if (!navigator.clipboard) model.notify('房间码：' + state.roomCode)
            }}
          >
            复制房间码
          </button>
          <button
            className="button subtle small"
            onClick={() => void model.leave()}
            disabled={model.busy}
          >
            返回大厅
          </button>
        </div>
      </div>
      {state.finished && (
        <div className="result-banner">
          <div>
            <strong>{resultText(state.result)}</strong>
            <span>棋谱已保存，可以到棋谱库逐手回看。</span>
          </div>
          <button className="button secondary small" onClick={() => model.setPage('replay')}>
            查看棋谱
            <Icon name="arrow" />
          </button>
        </div>
      )}
      <div className="game-layout">
        <section className="board-workspace">
          <div className="board-caption">
            <span className={'turn-dot ' + state.currentPlayer} />
            <strong data-testid="game-state">{stateText}</strong>
            <span className="move-counter">第 {state.moveCount} 手</span>
          </div>
          <Board
            size={state.boardSize}
            stones={state.stones}
            lastMove={state.lastMove}
            canPlay={model.canPlay}
            playerColor={state.currentPlayer}
            onPlay={(x, y) => void model.action('game.move', { x, y })}
          />
          <div className="board-bottom">
            <span>中国数子规则 · 白贴 7.5</span>
            <span>{state.consecutivePasses ? '已有一方停一手' : '末手以圆点标记'}</span>
          </div>
          {!model.spectating && !state.finished && (
            <div className="game-actions">
              <button
                className="button secondary"
                onClick={() => void model.action('game.pass')}
                disabled={!model.canPlay}
              >
                停一手
              </button>
              <button
                className="button text-button danger-text"
                onClick={() => setResigning(true)}
                disabled={!model.connected || model.busy || !model.myColor}
              >
                认输
              </button>
              {state.aiWhite && state.currentPlayer === 'white' && (
                <button
                  className="button subtle"
                  disabled={!model.connected || model.busy}
                  onClick={() => void model.action('ai.retry')}
                >
                  重试 AI 思考
                </button>
              )}
            </div>
          )}
        </section>
        <aside className="game-sidebar">
          <PlayersPanel state={state} myColor={model.myColor} spectating={model.spectating} />
          <ChatPanel model={model} />
        </aside>
      </div>
      {resigning && (
        <Modal title="结束这一局？" onClose={() => setResigning(false)}>
          <p className="dialog-copy">认输后，对手获胜，棋谱会保存在你的棋谱库中。</p>
          <div className="dialog-actions">
            <button className="button secondary" onClick={() => setResigning(false)}>
              继续对弈
            </button>
            <button
              className="button danger"
              onClick={() =>
                void model.action('game.resign').then((success) => {
                  if (success) setResigning(false)
                })
              }
              disabled={model.busy}
            >
              确认认输
            </button>
          </div>
        </Modal>
      )}
    </>
  )
}
