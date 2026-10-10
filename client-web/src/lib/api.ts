export class ApiError extends Error {
  status: number
  constructor(message: string, status: number) {
    super(message)
    this.status = status
  }
}
export async function api<T>(path: string, token = '', body?: unknown): Promise<T> {
  const response = await fetch('/api/v1' + path, {
    method: body === undefined ? 'GET' : 'POST',
    headers: {
      ...(token ? { Authorization: 'Bearer ' + token } : {}),
      ...(body !== undefined ? { 'Content-Type': 'application/json' } : {}),
    },
    body: body === undefined ? undefined : JSON.stringify(body),
  })
  const result = (await response.json().catch(() => null)) as {
    ok: boolean
    data: T
    error: string
  } | null
  if (!result?.ok)
    throw new ApiError(result?.error || '暂时无法连接服务，请稍后重试', response.status)
  return result.data
}
export function errorMessage(error: unknown) {
  return error instanceof Error ? error.message : '操作未完成，请重试'
}
export function downloadSgf(text: string, name: string) {
  const url = URL.createObjectURL(new Blob([text], { type: 'application/x-go-sgf;charset=utf-8' }))
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = name
  anchor.click()
  setTimeout(() => URL.revokeObjectURL(url), 1000)
}
