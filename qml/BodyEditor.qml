import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "transparent"

    property BodyModel model: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Content type selector
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: qsTr("Content Type:")
            }

            ComboBox {
                id: contentTypeCombo
                Layout.fillWidth: true
                model: ["application/json", "text/plain", "application/xml", "application/x-www-form-urlencoded"]
                currentIndex: {
                    if (!root.model) return 0;
                    const type = root.model.getContentType();
                    const idx = model.indexOf(type);
                    return idx >= 0 ? idx : 0;
                }
                onCurrentTextChanged: if (root.model) root.model.setContentType(currentText)
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Format")
                enabled: contentTypeCombo.currentText === "application/json"
                onClicked: if (root.model) root.model.formatJson()
            }
        }

        // Body content
        // ScrollView {
        //     Layout.fillWidth: true
        //     Layout.fillHeight: true

            TextArea {
                id: bodyText
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.model ? root.model.getContent() : ""
                onTextChanged: {
                    if (root.model) {
                        root.model.content = text
                        console.log("Body content changed:", text);
                    }
                }
                font.family: "monospace"
                wrapMode: TextArea.Wrap
            }
        // }
    }
} 
