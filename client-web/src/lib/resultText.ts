export function resultText(result: string) {
  if (!result || result === '?') return '对局结束'
  if (result === '0') return '双方和棋'
  const [color, reason] = result.split('+')
  return (
    (color === 'B' ? '黑棋胜' : '白棋胜') +
    (reason === 'R'
      ? ' · 对手认输'
      : reason === 'T'
        ? ' · 对手超时'
        : reason
          ? ' · ' + reason + ' 目'
          : '')
  )
}
