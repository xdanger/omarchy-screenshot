// SPDX-FileCopyrightText: 2026 Andy Stewart
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Quickshell
import qs.Ui

BarWidget {
    id: root
    moduleName: "io.github.manateelazycat.screenshot"
    implicitWidth: button.implicitWidth
    implicitHeight: button.implicitHeight

    function capture(delay) {
        const command = String(root.setting("command", "omarchy-screenshot"))
        Quickshell.execDetached(delay > 0
            ? [command, "--delay", String(delay)] : [command])
    }

    BarIconButton {
        id: button
        objectName: "screenshotButton"
        anchors.fill: parent
        bar: root.bar
        text: ""
        tooltipText: "Screenshot · Left: capture · Right: 5-second countdown"
        onPressed: buttonCode => {
            if (buttonCode === Qt.LeftButton) root.capture(0)
            else if (buttonCode === Qt.RightButton) root.capture(5)
        }
    }
}
