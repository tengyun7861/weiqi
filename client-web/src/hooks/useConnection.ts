import { useEffect, useState } from 'react'
import { api, ApiError } from '../lib/api'
import { Realtime } from '../lib/realtime'
import type { ConnectionStatus, Session, WireEvent } from '../lib/types'
import type { Notify } from './shared'

export function useConnection(
  session: Session | null,
  attach: (client: Realtime | null) => void,
  receive: (event: WireEvent, spectating: boolean) => void,
  expired: () => void,
  notify: Notify,
) {
  const [status, setStatus] = useState<ConnectionStatus>('offline')
  useEffect(() => {
    if (!session) return
    let alive = true
    void api('/auth/me', session.token).catch((error) => {
      if (alive && error instanceof ApiError && error.status === 401) expired()
    })
    const handle = (event: WireEvent) => {
      if (event.type === 'error' && (!event.requestId || event.requestId === 'auth')) {
        notify(String(event.payload.message))
        if (String(event.payload.message).includes('登录已过期')) expired()
      } else receive(event, client.spectating)
    }
    const client = new Realtime(session.token, handle, setStatus)
    attach(client)
    return () => {
      alive = false
      client.dispose()
      attach(null)
    }
  }, [session, attach, receive, expired, notify])
  return {
    status: session ? status : ('offline' as ConnectionStatus),
    connected: !!session && status === 'connected',
  }
}
