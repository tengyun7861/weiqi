import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 700
    visible: true
    title: "弈境围棋"
    color: "#f5f1e8"

    property int boardSize: game.boardSize
    property string currentPage: "大厅"
    property var publicRooms: []
    property var chatLines: []
    property var gameRecords: []

    Connections {
        target: network
        function onRoomsReceived(rooms) { window.publicRooms = rooms }
        function onRequestFailed(message) { notice.text = message; notice.open() }
        function onGameSnapshotReceived(snapshot) { window.currentPage = "对局" }
        function onChatReceived(nickname, content) { window.chatLines = window.chatLines.concat([{ "nickname": nickname, "content": content }]) }
        function onRecordsReceived(records) { window.gameRecords = records }
    }


    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 228
            color: "#173b32"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 12

                Label {
                    text: "弈境围棋"
                    color: "#fffaf0"
                    font.pixelSize: 28
                    font.bold: true
                }
                Label { text: "在线对弈 · 以棋会友"; color: "#b9d4c6"; font.pixelSize: 13 }
                Item { Layout.preferredHeight: 28 }

                Repeater {
                    model: ["大厅", "快速匹配", "创建房间", "对局", "人机对战", "棋谱复盘", "设置"]
                    delegate: Button {
                        required property string modelData
                        Layout.fillWidth: true
                        text: modelData
                        checkable: true
                        checked: window.currentPage === modelData
                        onClicked: window.currentPage = modelData
                        contentItem: Label { text: parent.text; color: "#f7f2e7"; font.pixelSize: 16; verticalAlignment: Text.AlignVCenter; leftPadding: 14 }
                        background: Rectangle { radius: 12; color: parent.checked ? "#396858" : "transparent" }
                    }
                }
                Item { Layout.fillHeight: true }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#426a5a" }
                Label { text: network.loggedIn ? network.nickname : "未登录棋手"; color: "#d9e7dd"; font.pixelSize: 14 }
                Button { text: network.loggedIn ? "实时已连接" : "登录 / 注册"; Layout.fillWidth: true; onClicked: loginDialog.open() }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                color: "#fffdf7"
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 34; anchors.rightMargin: 34
                    Label { text: currentPage; font.pixelSize: 26; font.bold: true; color: "#20352d" }
                    Item { Layout.fillWidth: true }
                    Label { text: network.status; color: "#61766d"; font.pixelSize: 15 }
                    Button { text: "退出"; enabled: network.loggedIn; onClicked: network.logout() }
                }
            }

            Rectangle {
                visible: currentPage === "大厅"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 28
                radius: 20
                color: "#fffdf7"
                border.color: "#e6ded0"
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 34; spacing: 18
                    Label { text: network.loggedIn ? "欢迎回来，" + network.nickname : "先登录，再开始对弈"; font.pixelSize: 28; font.bold: true; color: "#20352d" }
                    Label { text: "公开大厅 · 快速匹配 · 房间码邀请"; color: "#61766d"; font.pixelSize: 16 }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "快速匹配"; enabled: network.loggedIn; onClicked: { network.connectRealtime(); network.joinMatch(19) } }
                        Button { text: "创建公开房间"; enabled: network.loggedIn; onClicked: network.createRoom(19, true) }
                        Button { text: "刷新大厅"; enabled: network.loggedIn; onClicked: network.loadPublicRooms() }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#e7dfd2" }
                    Label { text: "公开房间"; font.pixelSize: 20; font.bold: true; color: "#20352d" }
                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: publicRooms
                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width; height: 64; radius: 14; color: "#f4f0e7"
                            RowLayout { anchors.fill: parent; anchors.margins: 14
                                Label { text: modelData.roomCode + " · " + modelData.boardSize + "路 · " + (modelData.status === "waiting" ? "等待棋手" : "对局中"); color: "#20352d"; Layout.fillWidth: true }
                                Button { text: modelData.status === "waiting" ? "加入" : "观战"; onClicked: network.joinRoom(modelData.roomCode, modelData.status !== "waiting") }
                            }
                        }
                        Label { anchors.centerIn: parent; visible: parent.count === 0; text: "暂无公开房间，创建一个吧"; color: "#8a968f" }
                    }
                }
            }

            Rectangle {
                visible: currentPage === "快速匹配"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; radius: 20; color: "#fffdf7"; border.color: "#e6ded0"
                ColumnLayout { anchors.centerIn: parent; width: 420; spacing: 18
                    Label { text: "快速匹配"; font.pixelSize: 30; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "系统会按棋盘大小为你寻找在线棋手。"; wrapMode: Text.Wrap; Layout.alignment: Qt.AlignHCenter }
                    ComboBox { id: matchSize; Layout.fillWidth: true; model: ["19 路", "13 路", "9 路"] }
                    Button { text: "开始匹配"; enabled: network.loggedIn; Layout.fillWidth: true; highlighted: true; onClicked: { network.connectRealtime(); network.joinMatch(matchSize.currentIndex === 0 ? 19 : matchSize.currentIndex === 1 ? 13 : 9) } }
                    Button { text: "取消匹配"; enabled: network.loggedIn; Layout.fillWidth: true; onClicked: network.cancelMatch() }
                }
            }

            Rectangle {
                visible: currentPage === "创建房间"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; radius: 20; color: "#fffdf7"; border.color: "#e6ded0"
                ColumnLayout { anchors.centerIn: parent; width: 430; spacing: 14
                    Label { text: "创建房间"; font.pixelSize: 30; font.bold: true }
                    ComboBox { id: createSize; Layout.fillWidth: true; model: ["19 路", "13 路", "9 路"] }
                    SpinBox { id: mainClock; Layout.fillWidth: true; from: 1; to: 120; value: 10; editable: true; prefix: "基本时间 "; suffix: " 分钟" }
                    SpinBox { id: byoClock; Layout.fillWidth: true; from: 5; to: 120; value: 30; editable: true; prefix: "读秒 "; suffix: " 秒" }
                    CheckBox { id: publicRoom; text: "在公开大厅显示"; checked: true }
                    Button { text: "创建并进入"; Layout.fillWidth: true; highlighted: true; enabled: network.loggedIn; onClicked: network.createRoom(createSize.currentIndex === 0 ? 19 : createSize.currentIndex === 1 ? 13 : 9, publicRoom.checked, mainClock.value * 60, byoClock.value) }
                    RowLayout { Layout.fillWidth: true
                        TextField { id: inviteCode; Layout.fillWidth: true; placeholderText: "输入六位房间码"; maximumLength: 6 }
                        Button { text: "加入"; onClicked: network.joinRoom(inviteCode.text, false) }
                    }
                }
            }

            RowLayout {
                visible: currentPage === "对局"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 28
                spacing: 28

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 20
                    color: "#fffdf7"
                    border.color: "#e6ded0"

                    Canvas {
                        id: board
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 64, parent.height - 64)
                        height: width
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            const margin = 34
                            const gap = (width - margin * 2) / (boardSize - 1)
                            ctx.fillStyle = "#d9ad62"; ctx.fillRect(0, 0, width, height)
                            ctx.strokeStyle = "#4d3420"; ctx.lineWidth = 1
                            for (let i = 0; i < boardSize; ++i) {
                                const p = margin + i * gap
                                ctx.beginPath(); ctx.moveTo(margin, p); ctx.lineTo(width - margin, p); ctx.stroke()
                                ctx.beginPath(); ctx.moveTo(p, margin); ctx.lineTo(p, height - margin); ctx.stroke()
                            }
                            const stars = boardSize === 19 ? [3, 9, 15] : [Math.floor(boardSize / 2)]
                            ctx.fillStyle = "#4d3420"
                            for (let x of stars) for (let y of stars) { ctx.beginPath(); ctx.arc(margin + x * gap, margin + y * gap, 4, 0, Math.PI * 2); ctx.fill() }
                            for (let stone of game.stones) {
                                const x = margin + stone.x * gap, y = margin + stone.y * gap
                                const gradient = ctx.createRadialGradient(x - 6, y - 6, 2, x, y, gap * .46)
                                gradient.addColorStop(0, stone.black ? "#606060" : "#ffffff")
                                gradient.addColorStop(1, stone.black ? "#080808" : "#d6d6d6")
                                ctx.fillStyle = gradient; ctx.beginPath(); ctx.arc(x, y, gap * .46, 0, Math.PI * 2); ctx.fill()
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                const margin = 34, gap = (board.width - margin * 2) / (boardSize - 1)
                                const col = Math.round((mouse.x - margin) / gap), row = Math.round((mouse.y - margin) / gap)
                                if (col >= 0 && row >= 0 && col < boardSize && row < boardSize) game.play(col, row)
                            }
                        }
                    }
                    Connections { target: game; function onBoardChanged() { board.requestPaint() } }
                }

                Rectangle {
                    Layout.preferredWidth: 310
                    Layout.fillHeight: true
                    radius: 20
                    color: "#fffdf7"
                    border.color: "#e6ded0"
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 24; spacing: 18
                        Label { text: "本局信息"; font.pixelSize: 20; font.bold: true; color: "#20352d" }
                        Label { text: "轮到：" + game.currentPlayer; font.pixelSize: 17; font.bold: true; color: "#20352d" }
                        Label { text: "黑棋  ·  " + (network.loggedIn ? network.nickname : "等待棋手") + "  " + (game.blackTime > 0 ? game.blackTime + " 秒" : "读秒 " + game.blackByoRemaining + " 秒"); font.pixelSize: 16; color: "#20352d" }
                        Label { text: "白棋  ·  等待棋手  " + (game.whiteTime > 0 ? game.whiteTime + " 秒" : "读秒 " + game.whiteByoRemaining + " 秒"); font.pixelSize: 16; color: "#20352d" }
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#e7dfd2" }
                        Label { text: network.currentRoomCode + " · " + boardSize + " 路" + (network.spectating ? " · 观战中" : ""); color: "#61766d" }
                        ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: chatLines; delegate: Label { required property var modelData; width: ListView.view.width; text: modelData.nickname + "：" + modelData.content; wrapMode: Text.Wrap; color: "#47584f" } }
                        RowLayout { Layout.fillWidth: true
                            TextField { id: chatInput; Layout.fillWidth: true; placeholderText: "输入聊天内容"; onAccepted: { network.sendChat(text); text = "" } }
                            Button { text: "发送"; onClicked: { network.sendChat(chatInput.text); chatInput.text = "" } }
                        }
                        Label { text: game.lastError; visible: text.length > 0; color: "#b33a2f"; wrapMode: Text.Wrap }
                        Button { text: "停一手"; Layout.fillWidth: true; onClicked: game.pass() }
                        Button { text: "认输"; Layout.fillWidth: true; highlighted: true; onClicked: { if (game.onlineGame) network.resign(); else notice.text = "本地演示棋局不记录认输结果"; if (!game.onlineGame) notice.open() } }
                    }
                }
            }

            Rectangle {
                visible: currentPage === "人机对战"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; radius: 20; color: "#fffdf7"; border.color: "#e6ded0"
                ColumnLayout { anchors.centerIn: parent; width: 440; spacing: 16
                    Label { text: "KataGo 人机对战"; font.pixelSize: 30; font.bold: true }
                    Label { text: "人机和分析只由服务端的本地 KataGo GTP 引擎执行。未配置时不会生成伪造结果。"; wrapMode: Text.Wrap; Layout.fillWidth: true; color: "#61766d" }
                    ComboBox { id: aiSize; Layout.fillWidth: true; model: ["19 路", "13 路", "9 路"] }
                    Label { text: network.katagoReason; wrapMode: Text.Wrap; Layout.fillWidth: true; color: network.aiAvailable ? "#27714e" : "#b33a2f" }
                    Button { text: network.aiAvailable ? "开始人机对局" : "KataGo 不可用"; Layout.fillWidth: true; enabled: network.loggedIn && network.aiAvailable; onClicked: network.createAiGame(aiSize.currentIndex === 0 ? 19 : aiSize.currentIndex === 1 ? 13 : 9) }
                    Button { text: "重新检查 KataGo"; Layout.fillWidth: true; onClicked: network.loadKataGoStatus() }
                }
            }

            Rectangle {
                visible: currentPage === "棋谱复盘"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; radius: 20; color: "#fffdf7"; border.color: "#e6ded0"
                RowLayout { anchors.fill: parent; anchors.margins: 26; spacing: 22
                    ColumnLayout { Layout.preferredWidth: 280; Layout.fillHeight: true
                        Label { text: "我的棋谱"; font.pixelSize: 24; font.bold: true }
                        Button { text: "刷新历史"; Layout.fillWidth: true; enabled: network.loggedIn; onClicked: network.loadGameRecords() }
                        ListView { Layout.fillWidth: true; Layout.fillHeight: true; model: gameRecords; clip: true
                            delegate: Button { required property var modelData; width: ListView.view.width; text: "#" + modelData.id + " · " + (modelData.result || "进行中"); onClicked: network.loadSgf(modelData.id) }
                        }
                    }
                    ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true
                        Label { text: "第 " + replay.step + " / " + replay.totalSteps + " 手"; font.pixelSize: 22; font.bold: true }
                        Label { text: replay.error; visible: text.length > 0; color: "#b33a2f" }
                        Item {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            Canvas {
                                id: replayBoard; anchors.centerIn: parent; width: Math.min(parent.width, parent.height); height: width
                                onPaint: {
                                    const ctx=getContext("2d"); ctx.clearRect(0,0,width,height)
                                    if (replay.totalSteps === 0) { ctx.fillStyle="#61766d"; ctx.font="16px sans-serif"; ctx.textAlign="center"; ctx.fillText("从左侧选择一局已完成的对局",width/2,height/2); return }
                                    const size=replay.boardSize, margin=34, gap=(width-margin*2)/(size-1); ctx.fillStyle="#d9ad62"; ctx.fillRect(0,0,width,height); ctx.strokeStyle="#4d3420"; ctx.lineWidth=1
                                    for(let i=0;i<size;++i){const p=margin+i*gap;ctx.beginPath();ctx.moveTo(margin,p);ctx.lineTo(width-margin,p);ctx.stroke();ctx.beginPath();ctx.moveTo(p,margin);ctx.lineTo(p,width-margin);ctx.stroke()}
                                    for(let stone of replay.stones){const x=margin+stone.x*gap,y=margin+stone.y*gap;ctx.fillStyle=stone.black?"#121212":"#f7f7f7";ctx.beginPath();ctx.arc(x,y,gap*.45,0,Math.PI*2);ctx.fill();ctx.strokeStyle=stone.black?"#000":"#a9a9a9";ctx.stroke()}
                                }
                                Connections { target: replay; function onChanged() { replayBoard.requestPaint() } }
                            }
                        }
                        RowLayout { Layout.fillWidth: true
                            Button { text: "上一步"; onClicked: replay.previous() }
                            Button { text: "下一步"; onClicked: replay.next() }
                            Slider { Layout.fillWidth: true; from: 0; to: Math.max(1, replay.totalSteps); value: replay.step; onMoved: replay.jumpTo(Math.round(value)) }
                        }
                    }
                }
            }

            Rectangle {
                visible: currentPage === "设置"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; radius: 20; color: "#fffdf7"; border.color: "#e6ded0"
                ColumnLayout { anchors.centerIn: parent; width: 460; spacing: 16
                    Label { text: "设置"; font.pixelSize: 30; font.bold: true }
                    Label { text: "服务端地址" }
                    TextField { Layout.fillWidth: true; text: network.serverUrl; onEditingFinished: network.serverUrl = text }
                    Label { text: "实时状态：" + network.status; wrapMode: Text.Wrap; Layout.fillWidth: true; color: "#61766d" }
                    Button { text: "重新连接实时服务"; Layout.fillWidth: true; enabled: network.loggedIn; onClicked: network.connectRealtime() }
                }
            }
        }
    }

    Dialog {
        id: loginDialog
        title: "登录或注册"
        modal: true
        anchors.centerIn: parent
        width: 390
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label { text: "服务端地址" }
            TextField { id: serverField; Layout.fillWidth: true; text: network.serverUrl; onEditingFinished: network.serverUrl = text }
            Label { text: "昵称" }
            TextField { id: nicknameField; Layout.fillWidth: true; placeholderText: "2–24 个字符" }
            Label { text: "密码" }
            TextField { id: passwordField; Layout.fillWidth: true; echoMode: TextInput.Password; placeholderText: "至少 8 个字符" }
            RowLayout {
                Layout.fillWidth: true
                Button { text: "注册"; Layout.fillWidth: true; onClicked: network.registerAccount(nicknameField.text, passwordField.text) }
                Button { text: "登录"; Layout.fillWidth: true; highlighted: true; onClicked: network.login(nicknameField.text, passwordField.text) }
            }
            Label { text: network.status; Layout.fillWidth: true; wrapMode: Text.Wrap; color: "#61766d" }
        }
    }
    Dialog { id: notice; property alias text: noticeLabel.text; anchors.centerIn: parent; modal: true; title: "提示"; standardButtons: Dialog.Ok; Label { id: noticeLabel; width: 280; wrapMode: Text.Wrap } }
}
