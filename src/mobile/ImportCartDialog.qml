import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import BrickStore as BS


Page {
    id: root
    title: qsTr("Import Cart")

    property var goBackFunction

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                icon.name: "go-previous"
                onClicked: goBackFunction()
            }
            Label {
                Layout.fillWidth: true
                font.pointSize: root.font.pointSize * 1.3
                minimumPointSize: font.pointSize / 2
                fontSizeMode: Text.Fit
                text: root.title
                elide: Label.ElideLeft
                horizontalAlignment: Qt.AlignLeft
            }
            ToolButton {
                icon.name: "view-refresh"
                onClicked: BS.BrickLink.carts.startUpdate()
                enabled: BS.BrickLink.carts.updateStatus !== BS.BrickLink.UpdateStatus.Updating
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ListView {
            id: table
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            ScrollIndicator.vertical: ScrollIndicator { active: true }

            model: BS.SortFilterProxyModel {
                id: sortFilterModel
                sourceModel: BS.BrickLink.carts
                sortOrder: Qt.DescendingOrder
                sortColumn: 0
                filterSyntax: BS.SortFilterProxyModel.FixedString
                filterString: domesticOrInternational.currentIndex ? "true" : "false"

                Component.onCompleted: {
                    filterRoleName = "domestic"
                    sortRoleName = "lastUpdated"
                }
            }

            delegate: ItemDelegate {
                id: delegate
                property int xspacing: 16
                width: ListView.view.width
                height: layout.height + xspacing
                visible: cart

                required property BS.Cart cart
                required property int index

                GridLayout {
                    id: layout
                    x: xspacing
                    y: xspacing / 2
                    width: parent.width - 2 * xspacing
                    columnSpacing: xspacing
                    rowSpacing: xspacing / 2
                    columns: 3

                    Image {
                        id: flag
                        Layout.rowSpan: 2
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                        asynchronous: true
                        source: "qrc:/assets/flags/" + cart.countryCode
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: fm.height * .75
                        sourceSize.width: fm.height * 1.5
                        FontMetrics {
                            id: fm
                            font: delegate.font
                        }
                        RectangularGlow {
                            z: -1
                            anchors.fill: parent
                            color: label.color
                            cornerRadius: 4
                            glowRadius: 4
                            spread: 0
                        }
                    }
                    Label {
                        id: label
                        text: cart.storeName + ' (' + cart.sellerName + ')'
                        font.bold: true
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        Layout.fillWidth: true
                    }
                    Label {
                        text: cart.lastUpdated.toLocaleDateString(Locale.ShortFormat)
                        Layout.alignment: Qt.AlignRight
                    }
                    Label {
                        text: qsTr("%1 items (%2 lots)")
                        .arg(Number(cart.itemCount).toLocaleString())
                        .arg(Number(cart.lotCount).toLocaleString())
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: BS.Currency.format(cart.total, cart.currencyCode, 2)
                        Layout.alignment: Qt.AlignRight
                    }
                }

                Rectangle {
                    z: 1
                    antialiasing: true
                    height: 1 / Screen.devicePixelRatio
                    color: "grey"
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: parent.xspacing / 2
                    anchors.leftMargin: anchors.rightMargin
                }
                onClicked: {
                    BS.BrickLink.carts.startFetchLots(cart)
                    root.goBackFunction()
                }
            }
        }
    }
    footer: TabBar {
        id: domesticOrInternational

        TabButton { text: qsTr("Domestic") }
        TabButton { text: qsTr("International") }

        onCurrentIndexChanged: table.forceLayout()
    }

    Component.onCompleted: {
        Qt.callLater(function() { BS.BrickLink.carts.startUpdate() })
    }
}
