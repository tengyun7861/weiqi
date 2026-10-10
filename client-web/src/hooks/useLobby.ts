import { useCallback, useEffect, useState } from 'react'
import { api, errorMessage } from '../lib/api'
import type { Page, Room, Session } from '../lib/types'
import type { ActionRunner, ConnectionRef, Notify } from './shared'

export function useLobby(
  session: Session | null,
  page: Page,
  connection: ConnectionRef,
  run: ActionRunner,
  notify: Notify,
) {
  const [rooms, setRooms] = useState<Room[]>([])
  const reset = useCallback(() => {
    setRooms([])
  }, [])
  const loadRooms = useCallback(async () => {
    if (!session) return
    try {
      setRooms(await api<Room[]>('/rooms', session.token))
    } catch (error) {
      notify(errorMessage(error))
    }
  }, [session, notify])
  useEffect(() => {
    if (page !== 'lobby' || !session) return
    const initial = setTimeout(() => void loadRooms(), 0)
    const timer = setInterval(() => {
      if (!document.hidden) void loadRooms()
    }, 8000)
    return () => {
      clearTimeout(initial)
      clearInterval(timer)
    }
  }, [page, session, loadRooms])
  const join = (code: string, spectating = false) =>
    run(async () => {
      if (!/^[A-Z2-9]{6}$/i.test(code.trim())) throw new Error('请输入六位房间码')
      if (!connection.current) throw new Error('请先登录')
      await connection.current.join(code, spectating)
    })
  const createRoom = (boardSize: number, isPublic: boolean, minutes: number, byo: number) =>
    run(async () => {
      if (!session || !connection.current) throw new Error('请先登录并等待连接就绪')
      const room = await api<{ roomCode: string }>('/rooms', session.token, {
        boardSize,
        isPublic,
        mainTimeSeconds: minutes * 60,
        byoYomiSeconds: byo,
      })
      await connection.current.join(room.roomCode)
    })
  const leave = async () => {
    await connection.current?.leave()
  }
  return { rooms, reset, loadRooms, join, createRoom, leave }
}
