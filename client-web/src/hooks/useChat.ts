import { useCallback, useState } from 'react'
import type { Chat, WireEvent } from '../lib/types'

export function useChat() {
  const [chat, setChat] = useState<Chat[]>([])
  const reset = useCallback(() => setChat([]), [])
  const receive = useCallback((event: WireEvent) => {
    if (event.type === 'chat.history') setChat(event.payload.messages as Chat[])
    else if (event.type === 'chat.message' && !event.payload.historical)
      setChat((lines) => [
        ...lines.slice(-99),
        { nickname: String(event.payload.nickname), content: String(event.payload.message) },
      ])
  }, [])
  return { chat, receive, reset }
}
