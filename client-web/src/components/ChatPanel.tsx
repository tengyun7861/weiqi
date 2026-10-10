import { useEffect, useRef, useState } from 'react'
import { Icon } from './Icon'
import type { WeiqiModel } from '../hooks/useWeiqi'
export function ChatPanel({ model }: { model: WeiqiModel }) {
  const [message, setMessage] = useState('')
  const chatEnd = useRef<HTMLDivElement>(null)
  useEffect(() => {
    chatEnd.current?.scrollIntoView({ block: 'nearest' })
  }, [model.chat.length])
  return (
    <section className="chat-panel">
      <div className="chat-heading">
        <h2>棋间交流</h2>
        <span>{model.spectating ? '观战席' : '本局房间'}</span>
      </div>
      <div className="chat-messages" data-testid="chat-messages" role="log" aria-label="房间聊天">
        {!model.chat.length && <p className="chat-empty">问候一句，棋逢知己。</p>}
        {model.chat.map((line, index) => (
          <div className="chat-line" key={index}>
            <strong>{line.nickname}</strong>
            <p>{line.content}</p>
          </div>
        ))}
        <div ref={chatEnd} />
      </div>
      <form
        className="chat-form"
        onSubmit={(event) => {
          event.preventDefault()
          const content = message.trim()
          if (content)
            void model.action('chat.send', { content }).then((success) => {
              if (success) setMessage('')
            })
        }}
      >
        <input
          aria-label="聊天内容"
          placeholder="说点什么…"
          value={message}
          maxLength={500}
          onChange={(event) => setMessage(event.target.value)}
        />
        <button
          className="icon-button"
          aria-label="发送消息"
          disabled={!message.trim() || !model.connected || model.busy}
        >
          <Icon name="arrow" />
        </button>
      </form>
    </section>
  )
}
