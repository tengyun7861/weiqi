import { useCallback, useState } from 'react'
export function useNotice() {
  const [notice, setNotice] = useState('')
  const notify = useCallback((message: string) => setNotice(message), [])
  const dismissNotice = useCallback(() => setNotice(''), [])
  return { notice, notify, dismissNotice }
}
