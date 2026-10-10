import { useCallback, useState } from 'react'
import { api } from '../lib/api'
import type { Session } from '../lib/types'

function savedSession(): Session | null {
  try {
    const value = JSON.parse(sessionStorage.getItem('weiqi.session') || 'null')
    return typeof value?.token === 'string' &&
      typeof value?.user?.id === 'number' &&
      typeof value?.user?.nickname === 'string'
      ? value
      : null
  } catch {
    return null
  }
}
export function useSession() {
  const [session, setSession] = useState<Session | null>(savedSession)
  const authenticate = useCallback((value: Session) => {
    sessionStorage.setItem('weiqi.session', JSON.stringify(value))
    setSession(value)
  }, [])
  const clear = useCallback(() => {
    sessionStorage.removeItem('weiqi.session')
    sessionStorage.removeItem('weiqi.room')
    setSession(null)
  }, [])
  const revoke = async () => {
    if (session) await api('/auth/logout', session.token, {})
  }
  return { session, authenticate, clear, revoke }
}
