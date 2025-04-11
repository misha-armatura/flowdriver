import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "#ffffff"
    
    required property RequestManager requestManager

    readonly property bool isWSorZMQ: {
        if (protocolCombo && protocolCombo.currentText) {
            return protocolCombo.currentText === "WebSocket" || protocolCombo.currentText === "ZeroMQ"
        }
        return false
    }

    // Models
    BodyModel { id: bodyModel }
    QueryModel { id: queryModel }
    HeadersModel { id: headersModel }

    // ZeroMQ properties
    property string currentPattern: "REQ-REP"
    property string currentRole: "REQUESTER"

    // Messages model
    ListModel { id: messagesModel }

    // File dialog for proto files
    FileDialog {
        id: protoFileDialog
        title: "Select Proto File"
        nameFilters: ["Proto files (*.proto)"]
        onAccepted: {
            let path = selectedFile.toString()
            // Handle both Windows and Unix paths
            if (Qt.platform.os === "windows") {
                // Remove file:/// for Windows
                path = path.replace(/^(file:\/{3})/,"")
            } else {
                // For Unix, keep the leading slash
                path = path.replace(/^(file:\/{2})/,"")
            }
            requestManager.protoFilePath = path
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // Protocol Selection Row
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                id: protocolCombo
                Layout.preferredWidth: 120
                model: ["REST", "gRPC", "WebSocket", "ZeroMQ"]
                currentIndex: model.indexOf(requestManager.currentProtocol)
                onCurrentTextChanged: requestManager.currentProtocol = currentText
            }

            ComboBox {
                id: methodCombo
                model: ["GET", "POST", "PUT", "DELETE", "PATCH"]
                visible: protocolCombo.currentText === "REST"
            }

            TextField {
                id: urlField
                Layout.fillWidth: true
                visible: protocolCombo.currentText !== "gRPC"
                placeholderText: {
                    switch(protocolCombo.currentText) {
                        case "WebSocket": return "Enter WebSocket URL (ws:// or wss://)"
                        case "ZeroMQ": return "Enter ZMQ endpoint (tcp://127.0.0.1:5555)"
                        default: return "Enter URL"
                    }
                }
                
                // Add validation for WebSocket URLs
                property bool isValid: {
                    if (protocolCombo.currentText === "WebSocket") {
                        return text.length === 0 || text.startsWith("ws://") || text.startsWith("wss://");
                    }
                    if (protocolCombo.currentText === "REST") {
                        return text.length === 0 || text.startsWith("http://") || text.startsWith("https://");
                    }
                    return true;
                }
                
                // Visual feedback for validation
                color: isValid ? "black" : "#d32f2f"
                
                // Show tooltip with hint
                ToolTip {
                    visible: !urlField.isValid && urlField.activeFocus
                    text: protocolCombo.currentText === "WebSocket" ? 
                        "URL must start with ws:// or wss://" :
                        "URL must start with http:// or https://"
                    delay: 500
                }
                
                // Add validation hint below the field
                Label {
                    id: urlHintLabel
                    anchors.left: parent.left
                    anchors.top: parent.bottom
                    anchors.topMargin: 2
                    text: protocolCombo.currentText === "WebSocket" ? 
                        "URL must start with ws:// or wss://" :
                        "URL must start with http:// or https://"
                    color: "#d32f2f"
                    font.pixelSize: 10
                    visible: !urlField.isValid && urlField.text.length > 0
                }
            }
        }

        // Connection controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // ZeroMQ Pattern/Role selection (only visible for ZeroMQ)
            RowLayout {
                visible: protocolCombo.currentText === "ZeroMQ"
                spacing: 5

                ComboBox {
                    id: zmqPatternCombo
                    model: ["REQ-REP", "PUB-SUB", "PUSH-PULL", "DEALER-ROUTER"]
                    onCurrentTextChanged: {
                        // Update the role combo box based on the selected pattern
                        switch(currentText) {
                            case "REQ-REP":
                                zmqRoleCombo.model = ["REQUESTER", "REPLIER"]
                                break
                            case "PUB-SUB":
                                zmqRoleCombo.model = ["PUBLISHER", "SUBSCRIBER"]
                                break
                            case "PUSH-PULL":
                                zmqRoleCombo.model = ["PUSHER", "PULLER"]
                                break
                            case "DEALER-ROUTER":
                                zmqRoleCombo.model = ["DEALER", "ROUTER"]
                                break
                        }
                        zmqRoleCombo.currentIndex = 0
                    }
                }

                ComboBox {
                    id: zmqRoleCombo
                    model: ["REQUESTER", "REPLIER"]  // Default for REQ-REP
                }
            }

            // Connect/Send button
            Button {
                text: {
                    if (requestManager.isConnected) return "Disconnect"
                    if (isWSorZMQ) return "Connect"
                    return "Send"
                }
                visible: protocolCombo.currentText !== "gRPC"
                enabled: !requestManager.isLoading && 
                         urlField.text.length > 0 && 
                         urlField.isValid
                onClicked: {
                    if (requestManager) {
                        if (requestManager.isConnected) {
                            requestManager.disconnect()
                            return
                        }

                        // Add URL validation before making the request
                        if (!urlField.isValid) {
                            let errorMsg = protocolCombo.currentText === "WebSocket" ?
                                "WebSocket URL must start with ws:// or wss://" :
                                "URL must start with http:// or https://"
                            requestManager.errorOccurred(errorMsg)
                            return
                        }

                        if (protocolCombo.currentText === "WebSocket") {
                            requestManager.connectWebSocket(urlField.text, headersModel.headers)
                        } else if (protocolCombo.currentText === "ZeroMQ") {
                            requestManager.connectZMQ(
                                urlField.text,
                                zmqPatternCombo.currentText,
                                zmqRoleCombo.currentText
                            )
                        } else if (protocolCombo.currentText === "gRPC") {
                            requestManager.executeGrpcRequest()
                        } else {
                            requestManager.executeRequest(
                                methodCombo.currentText,
                                urlField.text,
                                headersModel.headers,
                                bodyModel.content
                            )
                        }
                    }
                }
            }
        }

        // gRPC Controls
        ColumnLayout {
            visible: protocolCombo.currentText === "gRPC"
            spacing: 5
            Layout.fillWidth: true

            // Proto File Selection
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Label { text: "Proto File:" }
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

            // gRPC Service and Method Selection
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Label { text: "Service:" }
                ComboBox {
                    id: serviceCombo
                    Layout.fillWidth: true
                    model: requestManager.availableGrpcServices
                    enabled: model.length > 0
                    
                    Component.onCompleted: {
                        if (model.length > 0) {
                            currentIndex = 0
                        }
                    }
                    
                    onCurrentTextChanged: {
                        if (currentText) {
                            let methods = requestManager.getGrpcMethods(currentText)
                            grpcMethodCombo.model = methods
                            if (methods.length > 0) {
                                grpcMethodCombo.currentIndex = 0
                            }
                            requestManager.setGrpcService(currentText)
                        }
                    }
                }

                Label { text: "Method:" }
                ComboBox {
                    id: grpcMethodCombo
                    Layout.fillWidth: true
                    enabled: serviceCombo.currentText !== ""
                    onCurrentTextChanged: {
                        if (currentText) {
                            requestManager.setGrpcMethod(currentText)
                        }
                    }
                }
            }

            // Request Body Editor
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5

                Label { text: "Request Body:" }
                TextArea {
                    id: grpcRequestBody
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    placeholderText: "Enter JSON request body"
                    enabled: serviceCombo.currentIndex >= 0 && 
                            grpcMethodCombo.currentIndex >= 0 &&
                            endpointField.text.length > 0
                    font.family: "monospace"
                }
            }

            // Endpoint Configuration
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Label { text: "Endpoint:" }
                TextField {
                    id: endpointField
                    Layout.fillWidth: true
                    text: requestManager.grpcEndpoint
                    placeholderText: "localhost:50051"
                    onTextChanged: requestManager.grpcEndpoint = text
                }

                Label { text: "SSL/TLS:" }
                CheckBox {
                    checked: requestManager.grpcUseSSL
                    onCheckedChanged: requestManager.grpcUseSSL = checked
                }
            }

            // Execute Button
            Button {
                text: "Execute"
                enabled: serviceCombo.currentIndex >= 0 && 
                        grpcMethodCombo.currentIndex >= 0 &&
                        endpointField.text.length > 0 &&
                        serviceCombo.model.length > 0 &&
                        grpcMethodCombo.model.length > 0 &&
                        !requestManager.isLoading
                onClicked: {
                    requestManager.executeGrpcRequest(grpcRequestBody.text)
                }
            }

            // Status Label
            Label {
                text: {
                    if (!serviceCombo.model.length) return "Please load a proto file first"
                    if (!grpcMethodCombo.model.length) return "Please select a service"
                    if (endpointField.text === "") return "Please enter gRPC endpoint"
                    return ""
                }
                color: "#ff0000"
                visible: text !== ""
                font.italic: true
            }
        }

        // Stack for different protocol views
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: protocolCombo.currentText === "REST" ? 0 : 1

            // REST View
            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: protocolCombo.currentText === "REST"
                
                TabBar {
                    id: tabBar
                    Layout.fillWidth: true
                    currentIndex: 0
                    z: 2
                    
                    TabButton { 
                        text: "Headers" 
                        visible: protocolCombo.currentText === "REST"
                    }
                    TabButton { 
                        text: "Body" 
                        visible: protocolCombo.currentText === "REST"
                    }
                    TabButton { 
                        text: "Query" 
                        visible: protocolCombo.currentText === "REST"
                    }
                    TabButton { 
                        text: "Auth" 
                        visible: protocolCombo.currentText === "REST"
                    }
                }

                StackLayout {
                    currentIndex: tabBar.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    HeadersEditor { 
                        id: headersEditor
                        model: headersModel 
                        clip: true
                        visible: protocolCombo.currentText === "REST"
                    }
                    BodyEditor { 
                        id: bodyEditor
                        model: bodyModel 
                        clip: true
                        visible: protocolCombo.currentText === "REST"
                    }
                    QueryEditor { 
                        id: queryEditor
                        model: queryModel 
                        clip: true
                    }
                    AuthEditor { 
                        id: authEditor
                        model: authModel 
                        clip: true
                    }
                }
            }

            // WebSocket/ZeroMQ View
            SplitView {
                orientation: Qt.Vertical
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: protocolCombo.currentText === "WebSocket" || protocolCombo.currentText === "ZeroMQ"
                
                // Messages list Rectangle
                Rectangle {
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 100
                    color: "#ffffff"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 4

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4
                        
                        // Add a toolbar with clear button
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.margins: 4
                            
                            Label {
                                text: "Messages"
                                font.bold: true
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Button {
                                text: "Clear"
                                onClicked: messagesModel.clear()
                            }
                        }
                        
                        ListView {
                            id: messagesListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.margins: 8
                            model: messagesModel
                            spacing: 8
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar {}
                            
                            // Auto-scroll logic
                            onContentHeightChanged: {
                                if (atYEnd || contentHeight <= height) {
                                    positionViewAtEnd();
                                }
                            }
                            
                            // Track if we're at the bottom when user scrolls manually
                            property bool atYEnd: true
                            onMovementStarted: {
                                atYEnd = contentY + height >= contentHeight;
                            }

                            delegate: Rectangle {
                                width: messagesListView.width
                                height: messageColumn.height + 16
                                color: model.direction === "Sent" ? "#e8f5e9" : "#f3e5f5"
                                radius: 4

                                ColumnLayout {
                                    id: messageColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: 8
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Label {
                                            text: model.direction
                                            font.bold: true
                                        }

                                        Label {
                                            text: model.timestamp
                                            font.pixelSize: 12
                                            color: "#666666"
                                        }

                                        Item { Layout.fillWidth: true }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: model.message
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }

                            // Debug: Add count display
                            Label {
                                anchors.top: parent.top
                                anchors.right: parent.right
                                text: "Messages: " + messagesListView.count
                                z: 1
                            }

                            // Debug: Add scroll indicator
                            ScrollIndicator.vertical: ScrollIndicator { }
                        }
                    }
                }

                // Message input Rectangle
                Rectangle {
                    SplitView.fillWidth: true
                    SplitView.preferredHeight: 80
                    SplitView.minimumHeight: 50
                    color: "#f5f5f5"
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8
                        enabled: requestManager.isConnected

                        TextField {
                            id: messageInput
                            Layout.fillWidth: true
                            placeholderText: "Enter message"
                            visible: protocolCombo.currentText !== "gRPC"
                            onAccepted: sendButton.clicked()
                        }

                        Button {
                            id: sendButton
                            text: "Send"
                            visible: protocolCombo.currentText !== "gRPC"
                            enabled: messageInput.text.length > 0
                            onClicked: {
                                if (protocolCombo.currentText === "WebSocket") {
                                    requestManager.sendWebSocketMessage(messageInput.text)
                                } else if (protocolCombo.currentText === "ZeroMQ") {
                                    requestManager.executeZMQRequest(
                                        "SEND",
                                        urlField.text,
                                        zmqRoleCombo.currentText,
                                        messageInput.text
                                    )
                                }
                                messageInput.clear()
                            }
                        }
                    }
                }
            }
        }

        // Add this near the other controls, perhaps in the response area
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            visible: protocolCombo.currentText === "REST" // Only show for REST initially
            
            Label {
                text: "Export:"
            }
            
            ComboBox {
                id: exportFormatCombo
                model: ["JSON", "CSV", "PDF", "HTML"]
                Layout.preferredWidth: 100
            }
            
            Button {
                text: "Export"
                onClicked: {
                    // Connect to a method in RequestManager
                    requestManager.exportResponse(exportFormatCombo.currentText)
                }
            }
        }
    }

    // Add connections to handle responses
    Connections {
        target: requestManager
        
        function onResponseReceived(result) {
            logMessage("Response", JSON.stringify(result))
            if (protocolCombo.currentText === "REST") {
                // Handle REST response display
            } else {
                messagesModel.append({
                    direction: "Received",
                    message: result.body,
                    timestamp: new Date().toLocaleTimeString()
                })
            }
        }

        function onMessageReceived(message) {
            console.log("QML received message:", message);
            logMessage("Received", message);
            
            // Queue the update on the main thread
            messageUpdateTimer.pendingMessage = {
                direction: "Received",
                message: message,
                timestamp: new Date().toLocaleTimeString()
            }
            messageUpdateTimer.start()
        }

        function onMessageSent(message) {
            logMessage("Sent", message);
            
            // Queue the update on the main thread
            messageUpdateTimer.pendingMessage = {
                direction: "Sent",
                message: message,
                timestamp: new Date().toLocaleTimeString()
            }
            messageUpdateTimer.start()
        }

        function onErrorOccurred(error) {
            logMessage("Error", error);
            messagesModel.append({
                direction: "Error",
                message: error,
                timestamp: new Date().toLocaleTimeString()
            })
        }

        function onConnected() {
            logMessage("System", "Connected to " + urlField.text);
            messagesModel.append({
                direction: "System",
                message: "Connected to " + urlField.text,
                timestamp: new Date().toLocaleTimeString()
            })
        }

        function onDisconnected() {
            logMessage("System", "Disconnected");
            messagesModel.append({
                direction: "System",
                message: "Disconnected",
                timestamp: new Date().toLocaleTimeString()
            })
        }

        function onMessagesCleared() {
            messagesModel.clear();
        }
    }

    // Add this to handle protocol changes
    Connections {
        target: protocolCombo
        function onCurrentTextChanged() {
            // Clear all models when protocol changes
            headersModel.clear()
            bodyModel.clear()
            queryModel.clear()
            authModel.clear()
            messagesModel.clear()
            
            // Reset tab bar to first tab
            tabBar.currentIndex = 0
        }
    }

    // Add this function to determine when to show the Send button
    function shouldShowSendButton() {
        const role = zmqRoleCombo.currentText
        return ["DEALER", "REQUESTER", "PUBLISHER", "PUSHER"].includes(role)
    }

    // Add message logging function
    function logMessage(type, message) {
        console.log(`[${new Date().toISOString()}] ${type}: ${message}`);
    }

    // Add this to handle message updates on the main thread
    Timer {
        id: messageUpdateTimer
        interval: 1  // Update as soon as possible
        repeat: false
        property var pendingMessage: null
        
        onTriggered: {
            if (pendingMessage) {
                messagesModel.append(pendingMessage)
                pendingMessage = null
            }
        }
    }

    // Add a loading overlay
    Rectangle {
        id: loadingOverlay
        anchors.fill: parent
        color: "#80000000"  // Semi-transparent black
        visible: requestManager.isLoading
        z: 100  // Ensure it's on top
        
        BusyIndicator {
            anchors.centerIn: parent
            running: parent.visible
        }
        
        Label {
            anchors.top: parent.verticalCenter
            anchors.topMargin: 40
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Processing request..."
            color: "white"
        }
    }

    // Improve error display
    Rectangle {
        id: errorBanner
        height: errorText.contentHeight + 20
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: "#ffebee"  // Light red
        visible: errorText.text !== ""
        opacity: visible ? 1.0 : 0.0
        
        function hideError() {
            errorText.text = "";
        }

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }
        
        Label {
            id: errorText
            anchors.fill: parent
            anchors.margins: 10
            wrapMode: Text.WordWrap
            color: "#d32f2f"  // Dark red
            text: ""
        }
        
        Button {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 5
            text: "×"
            onClicked: errorBanner.hideError()
        }
        
        Timer {
            id: errorTimer
            interval: 5000
            running: errorText.text !== ""
            onTriggered: errorBanner.hideError()
        }
        
        Connections {
            target: requestManager
            function onErrorOccurred(error) {
                errorText.text = error;
                errorTimer.restart();
            }
        }
    }
} 
