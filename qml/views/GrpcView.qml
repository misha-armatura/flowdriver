import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FlowDriver.UI 1.0

ColumnLayout {
    spacing: 10

    FileDialog {
        id: protoFileDialog
        title: "Select Protocol Buffer File"
        nameFilters: ["Protocol Buffer Files (*.proto)"]
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        onAccepted: {
            requestManager.setProtoFilePath(selectedFile)
        }
    }

    // Proto file selection
    RowLayout {
        Layout.fillWidth: true
        
        Label {
            text: "Proto File:"
        }
        
        TextField {
            Layout.fillWidth: true
            text: requestManager.protoFilePath
            readOnly: true
            placeholderText: "Select a .proto file..."
        }
        
        Button {
            text: "Browse..."
            onClicked: protoFileDialog.open()
        }
    }

    // Existing gRPC controls
    RowLayout {
        Layout.fillWidth: true
        
        Label {
            text: "Endpoint:"
        }
        
        TextField {
            Layout.fillWidth: true
            text: requestManager.grpcEndpoint
            onTextChanged: requestManager.grpcEndpoint = text
        }
        
        CheckBox {
            text: "Use SSL"
            checked: requestManager.grpcUseSSL
            onCheckedChanged: requestManager.grpcUseSSL = checked
        }
    }

    // Service selection
    ComboBox {
        Layout.fillWidth: true
        model: requestManager.availableGrpcServices
        enabled: requestManager.protoFilePath !== ""
        onCurrentTextChanged: requestManager.setGrpcService(currentText)
    }

    // Method selection
    ComboBox {
        Layout.fillWidth: true
        model: requestManager.currentGrpcMethods
        enabled: currentIndex >= 0
        onCurrentTextChanged: requestManager.setGrpcMethod(currentText)
    }
} 