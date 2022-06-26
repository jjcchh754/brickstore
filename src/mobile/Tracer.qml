import QtQuick

MouseArea {
    id: root
    property bool fill: false

    anchors.fill: parent
    visible: typeof(showTracer) != "undefined" ? showTracer : false
    acceptedButtons: Qt.RightButton
    propagateComposedEvents: true

    readonly property color color: randomColor()
    function randomColor() {
        return Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0)
    }

    onPressAndHold: {
        console.log('Tracing' + root.parent + ':')
        let indent = '* ';
        for (let p = root.parent; p; p = p.parent) {
            console.log(indent + p)
            indent = '  ' + indent;
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: .5
        color: fill ? Qt.rgba(root.color.r, root.color.g, root.color.b, 0.3) : 'transparent'
        border.color: root.color
        border.width: 1
        opacity: 0.8
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: parent.width
        anchors.margins: 4
        horizontalAlignment: Text.AlignRight
        font.pixelSize: 10
        text: root.parent.objectName
        color: root.color
    }
}