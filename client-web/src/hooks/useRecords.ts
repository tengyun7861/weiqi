import { useCallback, useEffect, useState } from 'react'
import { api, downloadSgf, errorMessage } from '../lib/api'
import type { Page, RecordItem, Session } from '../lib/types'
import type { ActionRunner, Notify } from './shared'

export function useRecords(session: Session | null, page: Page, run: ActionRunner, notify: Notify) {
  const [records, setRecords] = useState<RecordItem[]>([])
  const reset = useCallback(() => setRecords([]), [])
  const loadRecords = useCallback(async () => {
    if (!session) return
    try {
      setRecords(await api<RecordItem[]>('/games', session.token))
    } catch (error) {
      notify(errorMessage(error))
    }
  }, [session, notify])
  useEffect(() => {
    if (page !== 'replay') return
    const timer = setTimeout(() => void loadRecords(), 0)
    return () => clearTimeout(timer)
  }, [page, loadRecords])
  const exportSgf = (id: number) =>
    run(async () => {
      const data = await api<{ sgf: string }>('/games/' + id + '/sgf', session?.token)
      downloadSgf(data.sgf, '弈境-' + id + '.sgf')
    })
  return { records, reset, loadRecords, exportSgf }
}
