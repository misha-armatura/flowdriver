import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "transparent"

    property QueryModel model: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Add Parameter button at the top
        Button {
            text: "Add Query Parameter"
            Layout.fillWidth: true
            onClicked: root.model.addQuery()
        }

        // Query parameters list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true  // Prevent items from showing outside bounds
            model: root.model
            spacing: 8
            
            delegate: QueryParamRow {
                width: parent.width
                rowIndex: index
                key: model.key
                value: model.value
                onParamRemoved: root.model.removeQuery(index)
                onParamChanged: root.model.updateQuery(index, key, value)
            }
        }
    }
} 
