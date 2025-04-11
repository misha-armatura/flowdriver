import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "transparent"
    
    property HeadersModel model: null
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16
        
        Button {
            Layout.fillWidth: true
            text: "Add Header"
            onClicked: {
                if (root.model) {
                    root.model.append("", "")
                }
            }
        }

        ListView {
            id: headersList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.model
            spacing: 4
            clip: true
            
            delegate: HeaderRow {
                width: headersList.width
                rowIndex: index
                name: model.name
                value: model.value
                
                onHeaderRemoved: function(index) {
                    root.model.remove(index)
                }
                
                onHeaderValueChanged: function(index, name, value) {
                    root.model.updateValue(index, name, value)
                }
            }
        }
    }
} 
