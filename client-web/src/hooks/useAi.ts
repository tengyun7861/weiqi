import { useCallback, useEffect, useState } from 'react'
import { api, errorMessage } from '../lib/api'
import type { AiStatus, Session } from '../lib/types'
import type { ActionRunner, ConnectionRef, Notify } from './shared'

export function useAi(
  session: Session | null,
  connection: ConnectionRef,
  run: ActionRunner,
  notify: Notify,
) {
  const [ai, setAi] = useState<AiStatus>({ available: false, reason: '' })
  const loadAi = useCallback(async () => {
    try {
      setAi(await api<AiStatus>('/katago'))
    } catch (error) {
      notify(errorMessage(error))
    }
  }, [notify])
  useEffect(() => {
    const timer = setTimeout(() => void loadAi(), 0)
    return () => clearTimeout(timer)
  }, [loadAi])
  const createAi = (boardSize: number) =>
    run(async () => {
      if (!session || !connection.current) throw new Error('请先登录并等待连接就绪')
      const room = await api<{ roomCode: string }>('/ai/games', session.token, { boardSize })
      await connection.current.join(room.roomCode)
    })
  return { ai, loadAi, createAi }
}
