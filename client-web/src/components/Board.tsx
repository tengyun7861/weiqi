import { useId, useState } from 'react'
import type { CSSProperties } from 'react'
import type { Color, Point } from '../lib/types'

export function Board({
  size,
  stones,
  lastMove,
  canPlay = false,
  onPlay,
  preview = false,
  playerColor = 'black',
}: {
  size: number
  stones: number[]
  lastMove?: Point | null
  canPlay?: boolean
  onPlay?: (x: number, y: number) => void
  preview?: boolean
  playerColor?: Color
}) {
  const id = useId().replaceAll(':', '')
  const [hover, setHover] = useState<Point | null>(null)
  const [focus, setFocus] = useState<Point>({ x: 0, y: 0 })
  const edge = 40,
    gap = 640 / (size - 1),
    radius = gap * 0.44
  const letters = 'ABCDEFGHJKLMNOPQRST'
  const star = size === 19 ? [3, 9, 15] : size === 13 ? [3, 6, 9] : [2, 4, 6]
  const points = Array.from({ length: size * size }, (_, index) => ({
    x: index % size,
    y: Math.floor(index / size),
    stone: stones[index] || 0,
  }))
  const coordinate = (x: number, y: number) => letters[x] + (size - y)
  const focusedIndex = focus.y * size + focus.x
  const keyboardIndex = stones[focusedIndex] ? stones.findIndex((stone) => !stone) : focusedIndex
  return (
    <div className={'board-frame' + (preview ? ' board-preview' : '')}>
      <svg
        viewBox="0 0 720 720"
        className="go-board"
        aria-label={preview ? '棋盘示意' : size + ' 路围棋棋盘'}
      >
        <defs>
          <linearGradient id={id + 'wood'} x2="1" y2=".5">
            <stop stopColor="#e8c68c" />
            <stop offset=".55" stopColor="#deb778" />
            <stop offset="1" stopColor="#d6ad6b" />
          </linearGradient>
          <radialGradient id={id + 'black'} cx=".32" cy=".25">
            <stop stopColor="#606b74" />
            <stop offset=".4" stopColor="#27323c" />
            <stop offset="1" stopColor="#111b24" />
          </radialGradient>
          <radialGradient id={id + 'white'} cx=".32" cy=".25">
            <stop stopColor="#fff" />
            <stop offset=".6" stopColor="#f5f5f1" />
            <stop offset="1" stopColor="#c5cbd0" />
          </radialGradient>
          <pattern id={id + 'grain'} width="150" height="80" patternUnits="userSpaceOnUse">
            <path
              d="M0 20Q60 8 150 21M0 25Q80 12 150 26M0 65Q90 52 150 61"
              fill="none"
              stroke="#9c763d"
              strokeOpacity=".10"
            />
          </pattern>
        </defs>
        <rect width="720" height="720" rx="9" fill={'url(#' + id + 'wood)'} />
        <rect width="720" height="720" rx="9" fill={'url(#' + id + 'grain)'} />
        <g stroke="#795b36" strokeWidth="1.1">
          {Array.from({ length: size }, (_, index) => (
            <path
              key={index}
              d={'M40 ' + (edge + index * gap) + 'H680M' + (edge + index * gap) + ' 40V680'}
            />
          ))}
        </g>
        <g fill="#604a31" textAnchor="middle" fontSize="14" fontFamily="Arial, sans-serif">
          {Array.from({ length: size }, (_, index) => (
            <g key={index}>
              <text x={edge + index * gap} y={23}>
                {letters[index]}
              </text>
              <text x={edge + index * gap} y={707}>
                {letters[index]}
              </text>
              <text x={20} y={edge + index * gap + 5}>
                {size - index}
              </text>
              <text x={700} y={edge + index * gap + 5}>
                {size - index}
              </text>
            </g>
          ))}
        </g>
        {star.flatMap((x) =>
          star.map((y) => (
            <circle
              key={x + '-' + y}
              cx={edge + x * gap}
              cy={edge + y * gap}
              r={size === 19 ? 3.5 : 4}
              fill="#674f31"
            />
          )),
        )}
        {points
          .filter((point) => point.stone)
          .map(({ x, y, stone }) => (
            <g
              key={x + '-' + y + '-' + stone}
              className="stone"
              style={
                { '--cx': edge + x * gap + 'px', '--cy': edge + y * gap + 'px' } as CSSProperties
              }
              data-testid={'stone-' + x + '-' + y}
              data-color={stone === 1 ? 'black' : 'white'}
            >
              <circle
                cx={edge + x * gap + 1.4}
                cy={edge + y * gap + 2.5}
                r={radius}
                fill="#684b30"
                opacity=".25"
              />
              <circle
                cx={edge + x * gap}
                cy={edge + y * gap}
                r={radius}
                fill={'url(#' + id + (stone === 1 ? 'black' : 'white') + ')'}
                stroke={stone === 1 ? '#19212a' : '#a5a5a0'}
                strokeWidth=".6"
              />
            </g>
          ))}
        {lastMove && stones[lastMove.y * size + lastMove.x] && (
          <circle
            cx={edge + lastMove.x * gap}
            cy={edge + lastMove.y * gap}
            r={radius * 0.24}
            fill="none"
            stroke={stones[lastMove.y * size + lastMove.x] === 1 ? '#fafafa' : '#344a5f'}
            strokeWidth="2"
          />
        )}
        {canPlay && hover && !stones[hover.y * size + hover.x] && (
          <circle
            cx={edge + hover.x * gap}
            cy={edge + hover.y * gap}
            r={radius}
            fill={playerColor === 'white' ? '#fff' : '#253d51'}
            opacity=".3"
            pointerEvents="none"
          />
        )}
        {!preview && (
          <g className="board-targets">
            {points.map(({ x, y, stone }) => (
              <rect
                key={x + '-' + y}
                x={edge + x * gap - gap / 2}
                y={edge + y * gap - gap / 2}
                width={gap}
                height={gap}
                fill="transparent"
                data-testid={'cell-' + x + '-' + y}
                role="button"
                aria-label={
                  (stone ? (stone === 1 ? '黑棋 ' : '白棋 ') : '落子 ') + coordinate(x, y)
                }
                aria-disabled={!canPlay || !!stone}
                tabIndex={canPlay && !stone && y * size + x === keyboardIndex ? 0 : -1}
                onMouseEnter={() => setHover({ x, y })}
                onMouseLeave={() => setHover(null)}
                onFocus={() => setHover({ x, y })}
                onClick={() => {
                  if (canPlay && !stone) onPlay?.(x, y)
                }}
                onKeyDown={(event) => {
                  if (!canPlay) return
                  if (event.key === 'Enter' || event.key === ' ') {
                    event.preventDefault()
                    if (!stone) onPlay?.(x, y)
                    return
                  }
                  const moves: Record<string, Point> = {
                    ArrowLeft: { x: -1, y: 0 },
                    ArrowRight: { x: 1, y: 0 },
                    ArrowUp: { x: 0, y: -1 },
                    ArrowDown: { x: 0, y: 1 },
                  }
                  const move = moves[event.key]
                  if (move) {
                    event.preventDefault()
                    const next = {
                      x: Math.max(0, Math.min(size - 1, x + move.x)),
                      y: Math.max(0, Math.min(size - 1, y + move.y)),
                    }
                    setFocus(next)
                    const target = event.currentTarget.parentElement?.querySelector<SVGElement>(
                      '[data-testid="cell-' + next.x + '-' + next.y + '"]',
                    )
                    target?.focus()
                  }
                }}
              />
            ))}
          </g>
        )}
      </svg>
    </div>
  )
}
