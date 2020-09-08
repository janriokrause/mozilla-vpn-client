import QtQuick 2.0
import QtGraphicalEffects 1.0

import Mozilla.VPN 1.0

import "../components"
import "../themes/themes.js" as Theme

Item {
    VPNControllerView {
        id: box
    }

    DropShadow {
        anchors.fill: box
        horizontalOffset: 0
        verticalOffset: 1
        radius: 4
        color: "#0C0C0D1E"
        source: box
    }

    VPNControllerServer {
        id: serverInfo
        onClicked: stackview.push("ViewServers.qml")
        y: box.y + box.height + Theme.iconSize
    }

    VPNControllerDevice {
        anchors.top: serverInfo.bottom
        anchors.topMargin: 8
        onClicked: stackview.push("ViewDevices.qml")
    }
}
