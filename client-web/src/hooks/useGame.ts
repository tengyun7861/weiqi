import { useCallback, useRef, useState } from 'react'
import type { Session, Snapshot, WireEvent } from '../lib/types'
import type { ActionRunner, ConnectionRef } from './shared'

const snapshotEvents = new Set([
  'room.snapshot',
  'game.snapshot',
  'game.updated',
  'game.finished',
  'clock.updated',
  'match.found',
])
export function useGame(session: Session | null, connection: ConnectionRef, run: ActionRunner) {
  const [snapshot, setSnapshot] = useState<Snapshot | null>(null)
  const [spectating, setSpectating] = useState(false)
  const roomCode = useRef('')
  const reset = useCallback(() => {
    roomCode.current = ''
    setSnapshot(null)
    setSpectating(false)
  }, [])
  const receive = useCallback((event: WireEvent, isSpectating: boolean) => {
    if (!snapshotEvents.has(event.type)) return false
    const state = event.payload as unknown as Snapshot
    const changed = roomCode.current !== state.roomCode
    roomCode.current = state.roomCode
    setSnapshot(state)
    setSpectating(isSpectating)
    return changed
  }, [])
  const action = (type: string, payload: Record<string, unknown> = {}) =>
    run(async () => {
      if (!connection.current) throw new Error('请先登录')
      await connection.current.send(type, payload)
    })
  const myColor =
    snapshot && session
      ? snapshot.blackUserId === session.user.id
        ? 'black'
        : snapshot.whiteUserId === session.user.id
          ? 'white'
          : null
      : null
  return { snapshot, receive, reset, action, myColor, spectating }
}
