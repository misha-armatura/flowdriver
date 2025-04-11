import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FlowDriver.UI 1.0

Rectangle {
    id: root
    color: "#f5f5f5"
    border.color: "#e0e0e0"
    border.width: 1
    radius: 4
    
    required property RequestManager requestManager
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        Label {
            text: "Benchmark Settings"
            font.bold: true
        }
        
        GridLayout {
            columns: 2
            Layout.fillWidth: true
            
            Label { text: "Concurrent Users:" }
            SpinBox {
                id: concurrentUsers
                from: 1
                to: 100
                value: 1
                Layout.fillWidth: true
            }
            
            Label { text: "Duration (seconds):" }
            SpinBox {
                id: duration
                from: 1
                to: 300
                value: 10
                Layout.fillWidth: true
            }
        }
        
        Button {
            text: "Run Benchmark"
            Layout.fillWidth: true
            onClicked: {
                // Connect to RequestManager
                requestManager.runBenchmark(concurrentUsers.value, duration.value)
            }
        }
        
        // Results display
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            color: "#ffffff"
            border.color: "#e0e0e0"
            visible: benchmarkModel.hasResults
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5
                
                Label { text: "Results:" }
                Label { text: "Total Requests: " + benchmarkModel.totalRequests }
                Label { text: "Requests/sec: " + benchmarkModel.requestsPerSecond }
                Label { text: "Success Rate: " + benchmarkModel.successRate + "%" }
            }
        }
    }
} 