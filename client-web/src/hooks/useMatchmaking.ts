import { useCallback, useState } from 'react'
import type { WireEvent } from '../lib/types'
import type { ActionRunner, ConnectionRef } from './shared'

export function useMatchmaking(connection: ConnectionRef, run: ActionRunner) {
  const [matching, setMatching] = useState(false)
  const reset = useCallback(() => setMatching(false), [])
  const receive = useCallback((event: WireEvent) => {
    if (event.type === 'match.waiting') setMatching(true)
    else if (event.type === 'match.found') setMatching(false)
  }, [])
  const match = (boardSize: number) =>
    run(async () => {
      if (!connection.current) throw new Error('请先登录')
      await connection.current.match(boardSize)
    })
  const cancelMatch = () =>
    run(async () => {
      await connection.current?.cancelMatch()
      setMatching(false)
    })
  return { matching, reset, receive, match, cancelMatch }
}
