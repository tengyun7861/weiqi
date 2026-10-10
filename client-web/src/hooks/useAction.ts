import { useCallback, useRef, useState } from 'react'
import { errorMessage } from '../lib/api'
import type { Notify } from './shared'

export function useAction(notify: Notify) {
  const [busy, setBusy] = useState(false)
  const pending = useRef(0)
  const run = useCallback(
    async (work: () => Promise<unknown>) => {
      pending.current++
      setBusy(true)
      try {
        await work()
        return true
      } catch (error) {
        notify(errorMessage(error))
        return false
      } finally {
        pending.current--
        setBusy(pending.current > 0)
      }
    },
    [notify],
  )
  return { busy, run }
}
