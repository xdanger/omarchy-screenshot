// SPDX-FileCopyrightText: 2026 Andy Stewart
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

Item {
    id: root
    width: 240
    height: 240
    property int seconds: 5
    property real elapsed: 0
    readonly property int remaining: Math.max(1, Math.ceil(seconds - elapsed))
    readonly property real sweep: elapsed % 1
    signal finished()

    function start() {
        if (!countdown.running) countdown.start()
    }

    SequentialAnimation {
        id: countdown
        NumberAnimation {
            target: root
            property: "elapsed"
            from: 0
            to: root.seconds
            duration: root.seconds * 1000
            easing.type: Easing.Linear
        }
        NumberAnimation {
            target: root
            property: "opacity"
            to: 0
            duration: 140
            easing.type: Easing.InQuad
        }
        ScriptAction { script: root.finished() }
    }

    Canvas {
        id: dial
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const cx = width / 2
            const cy = height / 2
            const radius = 100
            const start = -Math.PI / 2
            const end = start + root.sweep * 2 * Math.PI

            ctx.beginPath()
            ctx.arc(cx, cy, radius + 8, 0, 2 * Math.PI)
            ctx.fillStyle = "#ed181b24"
            ctx.fill()

            // The sweeping sector and crosshairs echo a film leader.
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.arc(cx, cy, radius - 5, start, end)
            ctx.closePath()
            ctx.fillStyle = "#24e4ebf5"
            ctx.fill()

            ctx.lineWidth = 1
            ctx.strokeStyle = "#30ffffff"
            ctx.beginPath()
            ctx.moveTo(cx - radius, cy)
            ctx.lineTo(cx + radius, cy)
            ctx.moveTo(cx, cy - radius)
            ctx.lineTo(cx, cy + radius)
            ctx.stroke()

            for (const r of [radius, radius - 15]) {
                ctx.beginPath()
                ctx.arc(cx, cy, r, 0, 2 * Math.PI)
                ctx.lineWidth = r === radius ? 2 : 1
                ctx.strokeStyle = r === radius ? "#70ffffff" : "#40ffffff"
                ctx.stroke()
            }

            ctx.beginPath()
            ctx.arc(cx, cy, radius, start, end)
            ctx.lineWidth = 4
            ctx.lineCap = "round"
            ctx.strokeStyle = "#f0f4ff"
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx + (radius - 15) * Math.cos(end),
                       cy + (radius - 15) * Math.sin(end))
            ctx.lineWidth = 1.5
            ctx.strokeStyle = "#90ffffff"
            ctx.stroke()
        }
        Connections {
            target: root
            function onElapsedChanged() { dial.requestPaint() }
        }
    }

    Text {
        objectName: "countdownNumber"
        anchors.centerIn: parent
        text: root.remaining
        color: "#f5f7ff"
        font.pixelSize: 94
        font.weight: Font.DemiBold
        scale: 1 + (1 - root.sweep) * 0.035
    }
}
