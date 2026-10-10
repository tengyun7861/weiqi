const paths = {
  board: 'M4 4h16v16H4z M4 9h16M4 15h16M9 4v16M15 4v16',
  lobby: 'M3 10l9-7 9 7M5 9v12h14V9M9 21v-7h6v7',
  book: 'M3 4h6c2 0 3 1 3 3v14c0-2-1-3-3-3H3z M21 4h-6c-2 0-3 1-3 3v14c0-2 1-3 3-3h6z',
  spark: 'M12 3l2.5 6.5L21 12l-6.5 2.5L12 21l-2.5-6.5L3 12l6.5-2.5z',
  add: 'M12 5v14M5 12h14',
  arrow: 'M4 12h16M14 6l6 6-6 6',
  close: 'M6 6l12 12M6 18L18 6',
  refresh: 'M20 7v5h-5M4 17v-5h5M6 6a8 8 0 0113 3M18 18a8 8 0 01-13-3',
  clock: 'M12 8v5l3 2 M12 3a9 9 0 100 18 9 9 0 000-18',
  eye: 'M2 12s4-7 10-7 10 7 10 7-4 7-10 7S2 12 2 12 M12 9a3 3 0 100 6 3 3 0 000-6',
  download: 'M12 3v12M7 10l5 5 5-5M4 16v5h16v-5',
  upload: 'M12 16V4M7 9l5-5 5 5M4 16v5h16v-5',
  back: 'M15 5l-7 7 7 7',
  next: 'M9 5l7 7-7 7',
}
export function Icon({ name, size = 20 }: { name: keyof typeof paths; size?: number }) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <path d={paths[name]} />
    </svg>
  )
}
