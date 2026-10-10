import { useCallback, useState } from 'react'
import { api } from '../lib/api'
import type { ReplayData, Session } from '../lib/types'
import type { ActionRunner } from './shared'

export function useReplay(session: Session | null, run: ActionRunner) {
  const [replay, setReplay] = useState<ReplayData | null>(null)
  const [replayStep, setReplayStep] = useState(0),
    [replayTitle, setReplayTitle] = useState('')
  const reset = useCallback(() => {
    setReplay(null)
    setReplayStep(0)
    setReplayTitle('')
  }, [])
  const openReplay = (id: number) =>
    run(async () => {
      const data = await api<ReplayData>('/games/' + id + '/replay', session?.token)
      setReplay(data)
      setReplayStep(0)
      setReplayTitle('对局 #' + id)
    })
  const importSgf = (file: File) =>
    run(async () => {
      if (file.size > 2 * 1024 * 1024) throw new Error('棋谱文件不能超过 2 MB')
      const data = await api<ReplayData>('/replay/import', session?.token, {
        sgf: await file.text(),
      })
      setReplay(data)
      setReplayStep(0)
      setReplayTitle(file.name)
    })
  return {
    replay,
    replayStep,
    setReplayStep,
    replayTitle,
    reset,
    openReplay,
    importSgf,
  }
}
