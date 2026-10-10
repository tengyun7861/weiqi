import type { RefObject } from 'react'
import type { Realtime } from '../lib/realtime'

export type ConnectionRef = RefObject<Realtime | null>
export type ActionRunner = (work: () => Promise<unknown>) => Promise<boolean>
export type Notify = (message: string) => void
