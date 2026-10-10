export type Page = 'lobby' | 'game' | 'ai' | 'replay'
export type Color = 'black' | 'white'
export interface User {
  id: number
  nickname: string
}
export interface Session {
  token: string
  user: User
}
export interface Point {
  x: number
  y: number
}
export interface Snapshot {
  roomCode: string
  boardSize: number
  stones: number[]
  currentPlayer: Color
  blackTime: number
  whiteTime: number
  blackByoRemaining: number
  whiteByoRemaining: number
  byoYomiSeconds: number
  finished: boolean
  sequence: number
  consecutivePasses: number
  blackUserId: number
  whiteUserId: number
  blackName: string
  whiteName: string
  aiWhite: boolean
  result: string
  finishReason: string
  moveCount: number
  lastMove: Point | null
}
export interface Room {
  roomCode: string
  boardSize: number
  status: 'waiting' | 'playing'
}
export interface RecordItem {
  id: number
  result: string
  finishReason: string
  endedAt: string
}
export interface Chat {
  nickname: string
  content: string
  createdAt?: string
}
export interface ReplayFrame {
  stones: number[]
  currentPlayer: Color
  lastMove: Point | null
}
export interface ReplayData {
  boardSize: number
  komi: number
  result: string
  moves: (Point & { color: Color })[]
  frames: ReplayFrame[]
}
export interface WireEvent {
  type: string
  requestId?: string
  sequence?: number
  payload: Record<string, unknown>
}
export interface AiStatus {
  available: boolean
  reason: string
}
export type ConnectionStatus = 'offline' | 'connecting' | 'connected' | 'reconnecting'
