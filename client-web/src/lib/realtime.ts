import type { ConnectionStatus, WireEvent } from './types'

type Pending = {
  resolve: (event: WireEvent) => void
  reject: (error: Error) => void
  timer: ReturnType<typeof setTimeout>
}
export class Realtime {
  private socket: WebSocket | null = null
  private stopped = false
  private ready = false
  private sequence = 0
  private retry = 0
  private retryTimer: ReturnType<typeof setTimeout> | undefined
  private pending = new Map<string, Pending>()
  private versions = new Map<string, number>()
  private room: { code: string; spectating: boolean } | null = null
  private matchingBoard: number | null = null
  private token: string
  private onEvent: (event: WireEvent) => void
  private onStatus: (status: ConnectionStatus) => void

  constructor(
    token: string,
    onEvent: (event: WireEvent) => void,
    onStatus: (status: ConnectionStatus) => void,
  ) {
    this.token = token
    this.onEvent = onEvent
    this.onStatus = onStatus
    try {
      this.room = JSON.parse(sessionStorage.getItem('weiqi.room') || 'null')
    } catch {
      /* Invalid saved room is ignored. */
    }
    this.connect()
  }
  private connect() {
    if (this.stopped) return
    this.ready = false
    this.sequence = 0
    this.versions.clear()
    this.onStatus(this.retry ? 'reconnecting' : 'connecting')
    const url = new URL('/ws', window.location.origin)
    url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:'
    const socket = new WebSocket(url)
    this.socket = socket
    socket.onopen = () =>
      socket.send(
        JSON.stringify({
          type: 'auth',
          requestId: 'auth',
          sequence: ++this.sequence,
          payload: { token: this.token },
        }),
      )
    socket.onmessage = (message) => {
      let event: WireEvent
      try {
        event = JSON.parse(message.data) as WireEvent
      } catch {
        return
      }
      if (event.type === 'session.ready' && event.payload.userId) {
        this.ready = true
        this.retry = 0
        this.onStatus('connected')
        if (this.room)
          void this.send(this.room.spectating ? 'spectate.join' : 'room.join', {
            roomCode: this.room.code,
          }).catch(() => {
            this.room = null
            sessionStorage.removeItem('weiqi.room')
          })
        if (this.matchingBoard)
          void this.send('match.join', { boardSize: this.matchingBoard }).catch(() => {})
      }
      const waiting = event.requestId ? this.pending.get(event.requestId) : undefined
      if (waiting) {
        clearTimeout(waiting.timer)
        this.pending.delete(event.requestId!)
        if (event.type === 'error') waiting.reject(new Error(String(event.payload.message)))
        else waiting.resolve(event)
      }
      if (event.type === 'match.found') {
        this.matchingBoard = null
        this.room = { code: String(event.payload.roomCode), spectating: false }
        sessionStorage.setItem('weiqi.room', JSON.stringify(this.room))
      }
      const snapshots = [
        'room.snapshot',
        'game.snapshot',
        'game.updated',
        'game.finished',
        'clock.updated',
        'match.found',
      ]
      if (snapshots.includes(event.type)) {
        const code = String(event.payload.roomCode)
        if (this.room && code !== this.room.code) return
        const version = Number(event.payload.sequence ?? event.sequence ?? 0)
        const previous = this.versions.get(code)
        if (previous !== undefined && version < previous) return
        this.versions.set(code, version)
      }
      // Control, chat, errors and authentication are never filtered by a game version.
      this.onEvent(event)
    }
    socket.onclose = () => {
      this.ready = false
      this.rejectPending('连接已断开，未确认的操作请检查棋盘后重试')
      if (this.stopped) return
      this.onStatus('reconnecting')
      this.retry++
      this.retryTimer = setTimeout(
        () => this.connect(),
        Math.min(1000 * 2 ** Math.min(this.retry - 1, 4), 15000),
      )
    }
    socket.onerror = () => {
      if (!this.stopped) this.onStatus('reconnecting')
    }
  }
  send(type: string, payload: Record<string, unknown> = {}) {
    if (!this.ready || this.socket?.readyState !== WebSocket.OPEN)
      return Promise.reject(new Error('实时连接尚未就绪，请稍后重试'))
    const id = Date.now().toString(36) + '-' + (++this.sequence).toString(36)
    return new Promise<WireEvent>((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id)
        reject(new Error('操作确认超时，请检查当前棋局后重试'))
      }, 10000)
      this.pending.set(id, { resolve, reject, timer })
      this.socket!.send(JSON.stringify({ type, requestId: id, payload, sequence: this.sequence }))
    })
  }
  async join(code: string, spectating = false) {
    const previous = this.room
    this.room = { code: code.trim().toUpperCase(), spectating }
    try {
      await this.send(spectating ? 'spectate.join' : 'room.join', { roomCode: this.room.code })
      sessionStorage.setItem('weiqi.room', JSON.stringify(this.room))
    } catch (error) {
      this.room = previous
      throw error
    }
  }
  async leave() {
    await this.send('room.leave')
    this.room = null
    sessionStorage.removeItem('weiqi.room')
  }
  async match(size: number) {
    this.matchingBoard = size
    try {
      await this.send('match.join', { boardSize: size })
    } catch (error) {
      this.matchingBoard = null
      throw error
    }
  }
  async cancelMatch() {
    this.matchingBoard = null
    await this.send('match.cancel')
  }
  get spectating() {
    return this.room?.spectating ?? false
  }
  private rejectPending(message: string) {
    for (const pending of this.pending.values()) {
      clearTimeout(pending.timer)
      pending.reject(new Error(message))
    }
    this.pending.clear()
  }
  dispose() {
    this.stopped = true
    clearTimeout(this.retryTimer)
    this.rejectPending('连接已关闭')
    this.socket?.close()
  }
}
