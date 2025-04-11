import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 8
    
    property alias key: keyField.text
    property alias value: valueField.text
    property int rowIndex: -1
    
    signal paramRemoved(int index)
    signal paramChanged(int index, string key, string value)

    TextField {
        id: keyField
        Layout.fillWidth: true
        placeholderText: "Parameter name"
        onTextChanged: root.paramChanged(root.rowIndex, text, valueField.text)
    }

    TextField {
        id: valueField
        Layout.fillWidth: true
        placeholderText: "Value"
        onTextChanged: root.paramChanged(root.rowIndex, keyField.text, text)
    }

    Button {
        text: "✕"
        onClicked: root.paramRemoved(root.rowIndex)
    }
} 