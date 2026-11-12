// checkmate — one line on what it does.

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "sculptgeometry.h"
#include <iostream>
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    const std::string command = argc > 1 ? argv[1] : "open";
    if (command == "--help" || command == "--version") {
      std::cout << (command == "--version" ? "Checkmate 0.1.0\n" : "Checkmate — 3D chess\nUsage: checkmate [open | --help | --version]\n");
      return 0;
    }
    if (command != "open" || argc > 2) { std::cerr << "Unknown command. Use checkmate --help.\n"; return 1; }
    QGuiApplication app(argc, argv);
    qmlRegisterType<SculptGeometry>("Checkmate", 1, 0, "SculptGeometry");
    app.setApplicationName("checkmate");
    app.setApplicationVersion("0.1.0");

    // Matches the window to checkmate.desktop, so the compositor (Wayland app_id)
    // and the launcher pick up the installed icon. The compiled-in copy covers
    // a run from build/, and needs qt6-svg's icon engine.
    app.setDesktopFileName("checkmate");
    app.setWindowIcon(QIcon::fromTheme("checkmate", QIcon(":/checkmate.svg")));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
