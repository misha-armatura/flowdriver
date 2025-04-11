import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 8
    
    property alias name: nameField.text
    property alias value: valueField.text
    property int rowIndex: -1
    
    signal headerRemoved(int index)
    signal headerValueChanged(int index, string name, string value)

    TextField {
        id: nameField
        Layout.fillWidth: true
        placeholderText: "Header name"
        onTextChanged: root.headerValueChanged(root.rowIndex, text, valueField.text)
    }

    TextField {
        id: valueField
        Layout.fillWidth: true
        placeholderText: "Value"
        onTextChanged: root.headerValueChanged(root.rowIndex, nameField.text, text)
    }

    Button {
        text: "✕"
        onClicked: root.headerRemoved(root.rowIndex)
    }
} 