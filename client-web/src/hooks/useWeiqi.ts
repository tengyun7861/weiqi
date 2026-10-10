import { useCallback, useRef, useState } from 'react'
import type { Page, WireEvent } from '../lib/types'
import type { Realtime } from '../lib/realtime'
import { useSession } from './useSession'
import { useNotice } from './useNotice'
import { useAction } from './useAction'
import { useConnection } from './useConnection'
import { useGame } from './useGame'
import { useChat } from './useChat'
import { useLobby } from './useLobby'
import { useAi } from './useAi'
import { useReplay } from './useReplay'
import { useRecords } from './useRecords'
import { useMatchmaking } from './useMatchmaking'

// Application composition only. Each hook owns one feature's state and behavior.
export function useWeiqi() {
  const [page, setPage] = useState<Page>('lobby')
  const connection = useRef<Realtime | null>(null)
  const account = useSession(),
    notices = useNotice(),
    work = useAction(notices.notify)
  const game = useGame(account.session, connection, work.run)
  const chat = useChat()
  const lobby = useLobby(account.session, page, connection, work.run, notices.notify)
  const ai = useAi(account.session, connection, work.run, notices.notify)
  const replay = useReplay(account.session, work.run)
  const records = useRecords(account.session, page, work.run, notices.notify)
  const matcher = useMatchmaking(connection, work.run)
  const { clear } = account
  const { reset: resetGame, receive: receiveGame } = game
  const { reset: resetChat, receive: receiveChat } = chat
  const { reset: resetLobby } = lobby
  const { reset: resetRecords } = records
  const { reset: resetMatcher, receive: receiveMatcher } = matcher
  const { reset: resetReplay } = replay
  const { notify } = notices
  const attach = useCallback((client: Realtime | null) => {
    connection.current = client
  }, [])
  const clearSession = useCallback(() => {
    clear()
    resetGame()
    resetChat()
    resetLobby()
    resetReplay()
    resetRecords()
    resetMatcher()
    setPage('lobby')
  }, [clear, resetGame, resetChat, resetLobby, resetReplay, resetRecords, resetMatcher])
  const receive = useCallback(
    (event: WireEvent, spectating: boolean) => {
      if (receiveGame(event, spectating)) {
        resetChat()
        setPage('game')
      }
      if (event.type === 'match.found') setPage('game')
      if (event.type === 'ai.error') notify(String(event.payload.message))
      receiveChat(event)
      receiveMatcher(event)
    },
    [receiveGame, receiveChat, resetChat, receiveMatcher, notify],
  )
  const transport = useConnection(account.session, attach, receive, clearSession, notify)
  const leave = () =>
    work.run(async () => {
      await lobby.leave()
      game.reset()
      chat.reset()
      setPage('lobby')
    })
  const logout = () =>
    work.run(async () => {
      try {
        await account.revoke()
      } finally {
        clearSession()
      }
    })
  const spectating = game.spectating
  const state = game.snapshot
  const canPlay =
    transport.connected &&
    !work.busy &&
    !!state &&
    !state.finished &&
    !spectating &&
    (state.whiteUserId !== 0 || state.aiWhite) &&
    state.currentPlayer === game.myColor
  return {
    session: account.session,
    authenticate: account.authenticate,
    logout,
    page,
    setPage,
    ...notices,
    ...transport,
    busy: work.busy,
    snapshot: game.snapshot,
    action: game.action,
    myColor: game.myColor,
    canPlay,
    spectating,
    chat: chat.chat,
    rooms: lobby.rooms,
    matching: matcher.matching,
    loadRooms: lobby.loadRooms,
    join: lobby.join,
    createRoom: lobby.createRoom,
    match: matcher.match,
    cancelMatch: matcher.cancelMatch,
    leave,
    ...ai,
    records: records.records,
    replay: replay.replay,
    replayStep: replay.replayStep,
    setReplayStep: replay.setReplayStep,
    replayTitle: replay.replayTitle,
    loadRecords: records.loadRecords,
    openReplay: replay.openReplay,
    importSgf: replay.importSgf,
    exportSgf: records.exportSgf,
  }
}
export type WeiqiModel = ReturnType<typeof useWeiqi>
