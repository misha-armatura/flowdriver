import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

FileDialog {
    id: fileDialog
    title: "Save Response"
    fileMode: FileDialog.SaveFile
    
    property string exportFormat: "JSON"
    property var responseData: ({})
    
    signal exportSaved(string filePath, string format, var responseData)
    
    onAccepted: {
        // Convert the URL to a local file path
        let filePath = selectedFile.toString()
        
        // Handle file:/// prefix
        if (Qt.platform.os === "windows") {
            filePath = filePath.replace(/^(file:\/{3})/, "")
        } else {
            filePath = filePath.replace(/^(file:\/{2})/, "")
        }
        
        // Add extension if needed
        if (exportFormat === "JSON" && !filePath.endsWith(".json")) {
            filePath += ".json"
        } else if (exportFormat === "CSV" && !filePath.endsWith(".csv")) {
            filePath += ".csv"
        } else if (exportFormat === "HTML" && !filePath.endsWith(".html")) {
            filePath += ".html"
        } else if (exportFormat === "PDF" && !filePath.endsWith(".pdf")) {
            filePath += ".pdf"
        }
        
        // Emit signal to save the file
        exportSaved(filePath, exportFormat, responseData)
    }
    
    function openForFormat(format, data) {
        exportFormat = format
        responseData = data
        
        // Set appropriate name filters based on format
        if (format === "JSON") {
            nameFilters = ["JSON Files (*.json)"]
            currentFile = "response.json"
        } else if (format === "CSV") {
            nameFilters = ["CSV Files (*.csv)"]
            currentFile = "response.csv"
        } else if (format === "HTML") {
            nameFilters = ["HTML Files (*.html)"]
            currentFile = "response.html"
        } else if (format === "PDF") {
            nameFilters = ["PDF Files (*.pdf)"]
            currentFile = "response.pdf"
        }
        
        open()
    }
} 