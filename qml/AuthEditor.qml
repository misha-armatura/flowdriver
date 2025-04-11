import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "transparent"

    property AuthModel model: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Auth type selection
        ComboBox {
            id: authTypeCombo
            Layout.fillWidth: true
            model: ["None", "Basic", "Bearer", "API Key"]
            currentIndex: root.model ? root.model.authType : 0
            onCurrentIndexChanged: if (root.model) root.model.authType = currentIndex
        }

        // Parameters section
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: authTypeCombo.currentIndex

            Item { }

            // Basic Auth
            GridLayout {
                columns: 2
                rowSpacing: 8
                columnSpacing: 8

                Label { text: "Username:" }
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "Enter username"
                    text: root.model ? root.model.username : ""
                    onTextChanged: if (root.model) root.model.username = text
                }

                Label { text: "Password:" }
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "Enter password"
                    echoMode: TextInput.Password
                    text: root.model ? root.model.password : ""
                    onTextChanged: if (root.model) root.model.password = text
                }
            }

            // Bearer Token
            ColumnLayout {
                spacing: 8

                Label { text: "Token:" }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Enter bearer token"
                    text: root.model ? root.model.token : ""
                    onTextChanged: if (root.model) root.model.token = text
                }
            }

            // API Key
            GridLayout {
                columns: 2
                rowSpacing: 8
                columnSpacing: 8

                Label { text: "Key Name:" }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Enter key name (e.g. 'X-API-Key')"
                    text: root.model ? root.model.apiKeyName : ""
                    onTextChanged: if (root.model) root.model.apiKeyName = text
                }

                Label { text: "Key Value:" }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Enter API key"
                    text: root.model ? root.model.apiKeyValue : ""
                    onTextChanged: if (root.model) root.model.apiKeyValue = text
                }

                Label { text: "Location:" }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Header", "Query Parameter"]
                    currentIndex: root.model ? root.model.apiKeyLocation : 0
                    onCurrentIndexChanged: if (root.model) root.model.apiKeyLocation = currentIndex
                }
            }
        }
    }
} 
