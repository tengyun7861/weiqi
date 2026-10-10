import { Icon } from './Icon'
import type { Snapshot } from '../lib/types'
function clockText(seconds: number, byo: number) {
  return seconds > 0
    ? Math.floor(seconds / 60)
        .toString()
        .padStart(2, '0') +
        ':' +
        (seconds % 60).toString().padStart(2, '0')
    : '读秒 ' + byo
}
export function PlayersPanel({
  state,
  myColor,
  spectating,
}: {
  state: Snapshot
  myColor: string | null
  spectating: boolean
}) {
  const waiting = !state.whiteUserId && !state.aiWhite
  return (
    <section className="players-panel" aria-label="对弈双方">
      {(['black', 'white'] as const).map((color) => (
        <div
          key={color}
          className={
            'player-card ' +
            (state.currentPlayer === color && !waiting && !state.finished ? 'active' : '')
          }
        >
          <span className={'player-stone ' + color} />
          <div>
            <span className="player-color">
              {color === 'black' ? '黑棋' : '白棋'}
              {myColor === color && !spectating ? ' · 你' : ''}
            </span>
            <h2>
              {color === 'black' ? state.blackName || '黑方棋手' : state.whiteName || '等待加入'}
            </h2>
          </div>
          <div className="player-clock" data-testid={color + '-clock'}>
            <Icon name="clock" size={15} />
            <span>
              {clockText(
                color === 'black' ? state.blackTime : state.whiteTime,
                color === 'black' ? state.blackByoRemaining : state.whiteByoRemaining,
              )}
            </span>
          </div>
        </div>
      ))}
      <p className="timer-note">
        基本时间耗尽后，每手 {state.byoYomiSeconds} 秒读秒。离线期间继续计时。
      </p>
    </section>
  )
}
