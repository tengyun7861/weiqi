import { useState } from 'react'
import { Modal } from './Modal'
import type { WeiqiModel } from '../hooks/useWeiqi'
export function CreateRoomDialog({ model, onClose }: { model: WeiqiModel; onClose: () => void }) {
  const [size, setSize] = useState(19),
    [minutes, setMinutes] = useState(10),
    [byo, setByo] = useState(30),
    [isPublic, setPublic] = useState(true)
  return (
    <Modal title="创建一局棋" onClose={onClose}>
      <form
        onSubmit={(event) => {
          event.preventDefault()
          void model.createRoom(size, isPublic, minutes, byo).then((success) => {
            if (success) onClose()
          })
        }}
      >
        <label>
          棋盘大小
          <select
            aria-label="棋盘大小"
            value={size}
            onChange={(event) => setSize(Number(event.target.value))}
          >
            <option value={19}>19 路 · 标准棋盘</option>
            <option value={13}>13 路 · 中盘练习</option>
            <option value={9}>9 路 · 快速对弈</option>
          </select>
        </label>
        <div className="form-row">
          <label>
            基本时间（分钟）
            <input
              type="number"
              min={0}
              max={120}
              value={minutes}
              onChange={(event) => setMinutes(Number(event.target.value))}
              required
            />
          </label>
          <label>
            每手读秒（秒）
            <input
              type="number"
              min={1}
              max={120}
              value={byo}
              onChange={(event) => setByo(Number(event.target.value))}
              required
            />
          </label>
        </div>
        <label className="check-label">
          <input
            type="checkbox"
            checked={isPublic}
            onChange={(event) => setPublic(event.target.checked)}
          />
          在大厅公开，其他棋手可以加入与观战
        </label>
        <p className="form-note">中国数子规则，白棋贴 7.5。你执黑棋，另一位棋手加入后开始计时。</p>
        <button className="button primary wide" disabled={model.busy || !model.connected}>
          {model.busy ? '正在创建…' : '创建并进入'}
        </button>
      </form>
    </Modal>
  )
}
