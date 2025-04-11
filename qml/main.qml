import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import FlowDriver.UI 1.0
/// TODO rewrite the scructure and change the import
import "." as App  // Import local components

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1200
    height: 800
    title: "FlowDriver"

    readonly property bool showSpacer: requestBuilder ? requestBuilder.isWSorZMQ : false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Main content area
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RequestBuilder {
                id: requestBuilder
                Layout.fillWidth: true
                // Layout.preferredHeight: parent.height * 0.6
                Layout.fillHeight: true
                Layout.minimumHeight: 400
                requestManager: requestManager
            }

            // Stack for response area
            StackLayout {
                Layout.fillWidth: true
                // Layout.preferredHeight: parent.height * 0.4
                // Layout.fillHeight: true
                Layout.minimumHeight: 100
                currentIndex: requestManager.currentProtocol === "REST" ? 0 : 1
                visible: requestManager.currentProtocol === "REST" ? 1 : 0

                // Response viewer for REST
                ResponseViewer {
                    id: responseViewer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    responseModel: responseModel
                }

                // Colored rectangle for non-REST protocols
                // Rectangle {
                //     Layout.fillWidth: true
                //     Layout.fillHeight: !showSpacer
                //     visible: !showSpacer
                //     color: "#ffffff"
                // }
            }
        }
    }

    // Models for data binding
    RequestManager {
        id: requestManager
        authModel: authModel
        onResponseReceived: function(result) {
            responseModel.updateResponse(result);
        }
        onErrorOccurred: function(error) {
            // Show error notification
        }
        Component.onCompleted: {
            console.log("RequestManager initialized");
        }
    }

    BodyModel {
        id: bodyModel
    }

    HeadersModel {
        id: headersModel
    }
    
    AuthModel {
        id: authModel
    }

    ResponseModel {
        id: responseModel
    }

    QueryModel {
        id: queryModel
    }

    Connections {
        target: requestManager
        function onResponseReceived(result) {
            responseModel.updateResponse(result)
        }
    }

    ExportDialog {
        id: exportDialog
        
        onExportSaved: function(filePath, format, responseData) {
            requestManager.saveResponseToFile(filePath, format, responseData)
        }
    }

    Connections {
        target: requestManager
        function onExportRequested(format, response) {
            exportDialog.openForFormat(format, response)
        }
    }
} 
