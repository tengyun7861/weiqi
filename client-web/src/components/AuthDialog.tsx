import { useState } from 'react'
import { api, errorMessage } from '../lib/api'
import type { Session } from '../lib/types'
import { Modal } from './Modal'
export function AuthDialog({
  onClose,
  onAuthenticated,
}: {
  onClose: () => void
  onAuthenticated: (session: Session) => void
}) {
  const [mode, setMode] = useState<'login' | 'register'>('login')
  const [nickname, setNickname] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState('')
  const [busy, setBusy] = useState(false)
  return (
    <Modal title={mode === 'login' ? '登录弈境' : '创建棋手账号'} onClose={onClose}>
      <div className="segmented">
        <button
          type="button"
          className={mode === 'login' ? 'selected' : ''}
          onClick={() => {
            setMode('login')
            setError('')
          }}
        >
          登录
        </button>
        <button
          type="button"
          className={mode === 'register' ? 'selected' : ''}
          onClick={() => {
            setMode('register')
            setError('')
          }}
        >
          注册
        </button>
      </div>
      <form
        onSubmit={async (event) => {
          event.preventDefault()
          setBusy(true)
          setError('')
          try {
            if (mode === 'register')
              await api('/auth/register', '', { nickname: nickname.trim(), password })
            const session = await api<Session>('/auth/login', '', {
              nickname: nickname.trim(),
              password,
            })
            onAuthenticated(session)
            onClose()
          } catch (failure) {
            setError(errorMessage(failure))
          } finally {
            setBusy(false)
          }
        }}
      >
        <label>
          昵称
          <input
            value={nickname}
            onChange={(event) => setNickname(event.target.value)}
            autoComplete="username"
            minLength={2}
            maxLength={24}
            required
            autoFocus
            placeholder="你的棋手昵称"
          />
        </label>
        <label>
          密码
          <input
            type="password"
            value={password}
            onChange={(event) => setPassword(event.target.value)}
            autoComplete={mode === 'register' ? 'new-password' : 'current-password'}
            minLength={8}
            maxLength={128}
            required
            placeholder="至少 8 个字符"
          />
        </label>
        {error && (
          <p className="form-error" role="alert">
            {error}
          </p>
        )}
        <button className="button primary wide" disabled={busy} data-testid="auth-submit">
          {busy ? '正在连接…' : mode === 'register' ? '注册并开始' : '登录并开始'}
        </button>
        <p className="form-note">账号用于保存对局与棋谱。密码不会显示给其他棋手。</p>
      </form>
    </Modal>
  )
}
