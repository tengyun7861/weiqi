"""Real browser regression against the C++ website. Uses a separate test database.

python scripts/web_e2e.py --url http://127.0.0.1:8130 --output verification
"""
import argparse
import json
import time
from pathlib import Path

from playwright.sync_api import expect, sync_playwright


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--url', default='http://127.0.0.1:8130')
    parser.add_argument('--output', default='verification')
    parser.add_argument('--browser', help='Optional Chromium executable path')
    args = parser.parse_args()
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    suffix = str(time.time_ns())[-9:]
    checks, errors = [], []

    def check(name):
        checks.append(name)
        print('PASS ' + name, flush=True)

    def connected(page):
        expect(page.get_by_test_id('connection-status')).to_have_text('实时连接')

    def account(page, name):
        page.goto(args.url)
        page.wait_for_load_state('networkidle')
        page.get_by_role('button', name='登录 / 注册', exact=True).click()
        page.get_by_role('button', name='注册', exact=True).click()
        page.get_by_label('昵称', exact=True).fill(name + suffix)
        page.get_by_label('密码', exact=True).fill('browser-test-password')
        page.get_by_test_id('auth-submit').click()
        connected(page)

    def no_overflow(page):
        assert page.evaluate('document.documentElement.scrollWidth <= innerWidth'), 'Horizontal overflow'

    def screenshot(page, name):
        page.evaluate('async () => { await Promise.all(document.getAnimations().map(a => a.finished.catch(() => {}))) }')
        page.screenshot(path=str(output / name), full_page=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(**({'executable_path': args.browser} if args.browser else {}))
        contexts = [browser.new_context(viewport={'width': 1440, 'height': 1000}) for _ in range(3)]
        black, white, watcher = [context.new_page() for context in contexts]
        for page in [black, white, watcher]:
            page.on('pageerror', lambda error: errors.append(str(error)))
        try:
            black.goto(args.url)
            black.wait_for_load_state('networkidle')
            expect(black).to_have_title('弈境围棋 · 每一局都值得留下')
            screenshot(black, 'lobby-desktop.png')
            no_overflow(black)
            check('C++ serves production HTML and JS; desktop lobby renders')
            account(black, '执黑')
            account(white, '执白')
            account(watcher, '观棋')
            check('Three independent browser accounts register, login and authenticate WebSocket')

            black.get_by_role('button', name='创建一局棋', exact=True).click()
            black.get_by_label('棋盘大小', exact=True).select_option('9')
            black.get_by_role('button', name='创建并进入').click()
            expect(black.get_by_test_id('current-room-code')).to_be_visible()
            code = black.get_by_test_id('current-room-code').inner_text()
            expect(black.get_by_role('button', name='停一手', exact=True)).to_be_disabled()
            white.get_by_label('房间码', exact=True).fill(code)
            white.get_by_role('button', name='加入房间', exact=True).click()
            expect(black.get_by_role('button', name='停一手', exact=True)).to_be_enabled()
            expect(white.get_by_test_id('current-room-code')).to_have_text(code)
            check('Create and join a 9x9 room; waiting player cannot play before opponent joins')

            for index, (player, x, y) in enumerate([(black, 1, 0), (white, 1, 1), (black, 0, 1), (white, 8, 8), (black, 2, 1), (white, 8, 7), (black, 1, 2)]):
                cell = player.get_by_test_id(f'cell-{x}-{y}')
                expect(cell).to_have_attribute('aria-disabled', 'false')
                if index == 0:
                    player.get_by_test_id('cell-0-0').focus()
                    player.get_by_test_id('cell-0-0').press('ArrowRight')
                    expect(cell).to_be_focused()
                    cell.press('Enter')
                else:
                    cell.click()
                for other in [black, white]:
                    expect(other.get_by_test_id(f'stone-{x}-{y}')).to_be_visible()
            expect(black.get_by_test_id('stone-1-1')).to_have_count(0)
            expect(white.get_by_test_id('stone-1-1')).to_have_count(0)
            check('Alternating moves and capture synchronized in both browsers')

            white.get_by_label('聊天内容', exact=True).fill('好棋，一起复盘！')
            white.get_by_role('button', name='发送消息', exact=True).click()
            expect(black.get_by_test_id('chat-messages')).to_contain_text('好棋，一起复盘！')
            expect(white.get_by_label('聊天内容', exact=True)).to_have_value('')
            black.reload()
            connected(black)
            expect(black.get_by_test_id('current-room-code')).to_have_text(code)
            expect(black.get_by_test_id('stone-1-2')).to_be_visible()
            expect(black.get_by_test_id('chat-messages')).to_contain_text('好棋，一起复盘！')
            check('Chat persists; reload restores seat, stones and chat despite sequence reset')

            watcher.get_by_role('button', name='刷新大厅', exact=True).click()
            row = watcher.locator('.room-row').filter(has_text=code)
            row.get_by_role('button', name='观战', exact=True).click()
            expect(watcher.get_by_test_id('game-state')).to_contain_text('观战中')
            expect(watcher.get_by_test_id('cell-4-4')).to_have_attribute('aria-disabled', 'true')
            expect(watcher.get_by_role('button', name='停一手', exact=True)).to_have_count(0)
            expect(watcher.get_by_test_id('chat-messages')).to_contain_text('好棋，一起复盘！')
            check('Spectator sees authoritative board and chat without player controls')

            screenshot(black, 'game-desktop.png')
            white.get_by_role('button', name='人机对弈', exact=True).click()
            expect(white.get_by_role('heading', name='与 KataGo 下一局。')).to_be_visible()
            white.wait_for_timeout(1400)  # clock event must not redirect navigation
            expect(white.get_by_role('heading', name='与 KataGo 下一局。')).to_be_visible()
            expect(white.get_by_role('button', name='开始人机对弈', exact=True)).to_be_disabled()
            white.get_by_role('button', name='在线对弈', exact=True).click()
            check('Clock updates preserve selected page; unconfigured KataGo is truthfully unavailable')

            white.get_by_role('button', name='停一手', exact=True).click()
            expect(black.get_by_role('button', name='停一手', exact=True)).to_be_enabled()
            black.get_by_role('button', name='停一手', exact=True).click()
            expect(black.locator('.result-banner')).to_be_visible()
            expect(white.locator('.result-banner')).to_be_visible()
            black.get_by_role('button', name='查看棋谱', exact=True).click()
            record = black.locator('.record-open').first
            expect(record).to_be_visible()
            record.click()
            expect(black.get_by_test_id('replay-step')).to_have_text('0 / 9 手')
            black.get_by_label('复盘手数', exact=True).fill('7')
            expect(black.get_by_test_id('stone-1-2')).to_be_visible()
            expect(black.get_by_test_id('stone-1-1')).to_have_count(0)
            black.get_by_role('button', name='下一手', exact=True).click()
            expect(black.get_by_test_id('replay-step')).to_have_text('8 / 9 手')
            with black.expect_download() as download:
                black.get_by_role('button', name='下载对局', exact=False).first.click()
            sgf = output / download.value.suggested_filename
            download.value.save_as(sgf)
            assert ';B[ba]' in sgf.read_text(encoding='utf-8')
            screenshot(black, 'replay-desktop.png')
            check('Two passes finish; C++ replay frames include captures and passes; SGF downloads')

            black.get_by_label('导入棋谱文件', exact=True).set_input_files({'name': 'capture.sgf', 'mimeType': 'application/x-go-sgf', 'buffer': b'(;GM[1]SZ[9]KM[7.5];B[ba];W[bb];B[ab];W[ii];B[cb];W[ih];B[bc])'})
            expect(black.locator('.replay-title h2')).to_have_text('capture.sgf')
            black.get_by_label('复盘手数', exact=True).fill('7')
            expect(black.get_by_test_id('stone-1-1')).to_have_count(0)
            black.get_by_label('导入棋谱文件', exact=True).set_input_files({'name': 'invalid.sgf', 'mimeType': 'application/x-go-sgf', 'buffer': b'(;SZ[9];B[aa];W[aa])'})
            expect(black.get_by_role('alert')).to_contain_text('非法着法')
            black.get_by_role('button', name='关闭提示', exact=True).click()
            check('Imported SGF is parsed and replayed by C++; illegal moves display explicit errors')

            for player in [black, white]:
                player.get_by_role('button', name='在线对弈', exact=True).click()
                player.get_by_role('button', name='返回大厅', exact=True).click()
                player.get_by_label('匹配棋盘大小', exact=True).select_option('13')
                player.get_by_role('button', name='开始匹配', exact=True).click()
            for player in [black, white]:
                expect(player.get_by_test_id('current-room-code')).to_be_visible()
                expect(player.get_by_role('heading', name='13 路对弈', exact=False)).to_be_visible()
            assert black.get_by_test_id('current-room-code').inner_text() == white.get_by_test_id('current-room-code').inner_text()
            check('Two independent browser sessions match on 13x13 and enter the same room')

            black.get_by_role('button', name='认输', exact=True).click()
            black.get_by_role('button', name='确认认输', exact=True).click()
            expect(white.locator('.result-banner')).to_contain_text('对手认输')
            check('Resignation reaches both browsers and saves the game')

            black.set_viewport_size({'width': 375, 'height': 812})
            no_overflow(black)
            screenshot(black, 'game-mobile.png')
            black.get_by_role('button', name='棋局大厅', exact=True).click()
            no_overflow(black)
            screenshot(black, 'lobby-mobile.png')
            check('375px mobile lobby and game have no horizontal overflow')
            token = black.evaluate("JSON.parse(sessionStorage.getItem('weiqi.session')).token")
            black.get_by_role('button', name='退出', exact=True).click()
            expect(black.get_by_role('button', name='登录 / 注册', exact=True)).to_be_visible()
            response = black.request.get(args.url + '/api/v1/auth/me', headers={'Authorization': 'Bearer ' + token})
            assert response.status == 401
            check('Logout revokes the server session token')
            assert not errors, errors
            check('No uncaught JavaScript errors')
        except Exception:
            for index, page in enumerate([black, white, watcher]):
                page.screenshot(path=str(output / f'failure-{index}.png'), full_page=True)
            raise
        finally:
            (output / 'web-e2e.json').write_text(json.dumps({'checks': checks, 'pageErrors': errors}, ensure_ascii=False, indent=2), encoding='utf-8')
            browser.close()


if __name__ == '__main__':
    main()
