import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "#ffffff"
    
    property ResponseModel responseModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Status and time
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Label {
                text: "Status: " + (responseModel ? responseModel.statusCode : "0") + 
                      " (" + (responseModel ? responseModel.time : "0 ms") + ")"
                font.pixelSize: 14
            }
        }

        // Response tabs
        TabBar {
            id: responseTabBar
            Layout.fillWidth: true

            TabButton { text: "Body" }
            TabButton { text: "Headers" }
        }

        // Response content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: responseTabBar.currentIndex

            // Body tab
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                TextArea {
                    text: {
                        if (responseModel && responseModel.error) {
                            return "Error: " + responseModel.error;
                        }
                        return responseModel ? responseModel.formattedBody : ""
                    }
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    font.family: "Monospace"
                    color: responseModel && responseModel.error ? "#d32f2f" : "black"
                }
            }

            // Headers tab
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: responseModel ? responseModel.headers : []
                delegate: ItemDelegate {
                    width: parent.width
                    text: modelData.name + ": " + modelData.value
                }
            }
        }
    }
} 
