import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "#ffffff"
    
    required property RequestManager requestManager
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        Label {
            text: "Request History"
            font.bold: true
            padding: 10
        }
        
        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: requestManager.historyModel
            clip: true
            
            delegate: ItemDelegate {
                width: parent.width
                contentItem: ColumnLayout {
                    spacing: 2
                    
                    Label {
                        text: model.method + " " + model.url
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    
                    Label {
                        text: model.timestamp
                        font.pixelSize: 10
                        color: "#757575"
                    }
                }
                
                onClicked: {
                    requestManager.loadHistoryItem(model.id)
                }
            }
        }
    }
} 