#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "models/headers_model.hpp"
#include "models/body_model.hpp"
#include "models/auth_model.hpp"
#include "models/response_model.hpp"
#include "models/request_manager.hpp"
#include "models/query_model.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    qmlRegisterType<flowdriver::ui::HeadersModel>("FlowDriver.UI", 1, 0, "HeadersModel");
    qmlRegisterType<flowdriver::ui::BodyModel>("FlowDriver.UI", 1, 0, "BodyModel");
    qmlRegisterType<flowdriver::ui::AuthModel>("FlowDriver.UI", 1, 0, "AuthModel");
    qmlRegisterType<flowdriver::ui::QueryModel>("FlowDriver.UI", 1, 0, "QueryModel");
    qmlRegisterType<flowdriver::ui::ResponseModel>("FlowDriver.UI", 1, 0, "ResponseModel");

    // Main manager for handling requests
    qmlRegisterType<flowdriver::ui::RequestManager>("FlowDriver.UI", 1, 0, "RequestManager");
    
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    
    return app.exec();
} 
