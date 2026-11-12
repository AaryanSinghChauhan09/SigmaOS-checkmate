#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "sculptgeometry.h"
#include <algorithm>
#include "theme.h"
#include "translator.h"

namespace {

bool write(const QString &path, const QByteArray &contents) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

int lines(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll().count('\n') : 0;
}

// A hyprctl that answers with response.json beside it and counts its calls.
QString fakeHyprctl(const QString &directory) {
    const QString command = directory + "/hyprctl";
    write(command, "#!/bin/bash\necho >> '" + directory.toUtf8() + "/calls'\ncat '" + directory.toUtf8()
                       + "/response.json'\n");
    QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return command;
}

QJsonObject readCatalog(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

// The placeholders in a text, sorted: %1 to %99, and the count (%n, %Ln).
QStringList placeholders(const QString &text) {
    static const QRegularExpression placeholder("%L?(?:[1-9][0-9]?|n)");
    QStringList found;
    for (auto it = placeholder.globalMatch(text); it.hasNext();)
        found << it.next().captured();
    found.sort();
    return found;
}

// Every text the sources hand to qsTr() or tr(), and whether a call passes it
// a count. It walks the code a token at a time, so a string never hides a
// comment or the other way round, C++'s adjacent literals join, and QML's
// single quotes count. A text that isn't a literal can't be in a catalog.
QMap<QString, bool> translatables(const QString &folder, QStringList *problems) {
    QMap<QString, bool> found;
    for (const QFileInfo &info : QDir(folder).entryInfoList({"*.qml", "*.js", "*.cpp", "*.h"}, QDir::Files)) {
        QFile file(info.filePath());
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString code = QString::fromUtf8(file.readAll());
        qsizetype i = 0;
        auto literal = [&](QString *text) {
            const QChar quote = code[i++];
            for (; i < code.size() && code[i] != quote; ++i) {
                if (code[i] == '\\' && i + 1 < code.size()) {
                    const QChar escaped = code[++i];
                    const int digits = escaped == 'u' ? 4 : escaped == 'x' ? 2 : 0;
                    bool hex = false;
                    const ushort unit = code.mid(i + 1, digits).toUShort(&hex, 16);
                    if (digits && hex) {
                        *text += QChar(unit);
                        i += digits;
                    } else {
                        *text += escaped == 'n' ? QChar('\n') : escaped == 't' ? QChar('\t') : escaped;
                    }
                } else {
                    *text += code[i];
                }
            }
            ++i;
        };
        auto skipSpace = [&] {
            while (i < code.size() && code[i].isSpace())
                ++i;
        };
        while (i < code.size()) {
            const QChar c = code[i];
            if (code.mid(i, 2) == "//") {
                i = code.indexOf('\n', i);
                if (i < 0)
                    break;
            } else if (code.mid(i, 2) == "/*") {
                i = code.indexOf("*/", i + 2);
                if (i < 0)
                    break;
                i += 2;
            } else if (c == '"' || c == '\'') {
                QString skipped;
                literal(&skipped);
            } else if (c.isLetter() || c == '_') {
                const qsizetype start = i;
                while (i < code.size() && (code[i].isLetterOrNumber() || code[i] == '_'))
                    ++i;
                const QString word = code.mid(start, i - start);
                skipSpace();
                if ((word != "qsTr" && word != "tr") || i >= code.size() || code[i] != '(')
                    continue;
                ++i;
                skipSpace();
                if (i >= code.size() || (code[i] != '"' && code[i] != '\'')) {
                    *problems << info.fileName() + ": not a literal: " + code.mid(start, 40);
                    continue;
                }
                QString text;
                do {
                    literal(&text);
                    skipSpace();
                } while (i < code.size() && code[i] == '"');
                // The arguments after the text, to the call's closing parenthesis.
                int depth = 0, commas = 0;
                while (i < code.size()) {
                    const QChar d = code[i];
                    if (d == '"' || d == '\'') {
                        QString skipped;
                        literal(&skipped);
                        continue;
                    }
                    if (d == '(' || d == '[' || d == '{')
                        ++depth;
                    else if ((d == ')' || d == ']' || d == '}') && depth-- == 0)
                        break;
                    else if (d == ',' && depth == 0)
                        ++commas;
                    ++i;
                }
                found[text] = found.value(text) || commas >= 2;
            } else {
                ++i;
            }
        }
    }
    return found;
}

} // namespace

class CheckmateTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QQuickStyle::setStyle("Material"); qmlRegisterType<SculptGeometry>("Checkmate",1,0,"SculptGeometry"); }

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QColor("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QColor("black"));
        QVERIFY(!theme.light());
        QCOMPARE(theme.radius(), 0);
    }

    void themeReadsThePalette() {
        QTemporaryDir dir;
        QVERIFY(QDir(dir.path()).mkpath("theme"));
        QVERIFY(write(dir.path() + "/theme/colors.toml", "mode = \"light\"\n"
                                                         "theme_type = \"dark\"\n"
                                                         "accent = \"#112233\"\n"
                                                         "bg = #fafafa # paper\n"
                                                         "foreground = '#202020'\n"
                                                         "color1 = \"#aa0000\"\n"));

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QColor("#112233"));
        QCOMPARE(theme.accentForeground(), QColor("white"));
        QCOMPARE(theme.background(), QColor("#fafafa"));
        QCOMPARE(theme.foreground(), QColor("#202020"));
        QCOMPARE(theme.colors().value("red").value<QColor>(), QColor("#aa0000"));
        QVERIFY(theme.light());
    }

    void themeAsksHyprlandAgainOnlyWhenItReloads() {
        // The fake Hyprland lives outside the theme directory, whose changes
        // the Theme also follows.
        QTemporaryDir dir, hypr;
        const QString command = fakeHyprctl(hypr.path());
        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":4}"));
        QLocalServer events;
        QVERIFY(events.listen(hypr.filePath("events.sock")));

        // Once on start, and once more when the event socket connects.
        Theme theme(dir.path(), nullptr, command, events.fullServerName());
        QTRY_VERIFY(events.hasPendingConnections());
        QLocalSocket *hyprland = events.nextPendingConnection();
        QTRY_COMPARE(lines(hypr.filePath("calls")), 2);
        QTRY_COMPARE(theme.radius(), 4);

        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":10}"));
        QTest::qWait(300);
        QCOMPARE(lines(hypr.filePath("calls")), 2);
        // Events arrive in pieces; only a whole configreloaded line counts.
        hyprland->write("activewindow>>kitty,~\nconfigrel");
        hyprland->flush();
        QTest::qWait(100);
        QCOMPARE(lines(hypr.filePath("calls")), 2);
        hyprland->write("oaded>>\n");
        QTRY_COMPARE(theme.radius(), 10);
        QCOMPARE(lines(hypr.filePath("calls")), 3);
    }

    void themeReachesHyprlandOnceItListens() {
        QTemporaryDir dir, hypr;
        const QString command = fakeHyprctl(hypr.path());
        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":4}"));
        Theme theme(dir.path(), nullptr, command, hypr.filePath("events.sock"));
        QTRY_COMPARE(theme.radius(), 4);

        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":8}"));
        QLocalServer events;
        QVERIFY(events.listen(hypr.filePath("events.sock")));
        QTRY_VERIFY_WITH_TIMEOUT(events.hasPendingConnections(), 5000);
        QTRY_COMPARE(theme.radius(), 8);
    }

    void translatorFallsBackToEnglish() {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile() {
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/language", "pt-BR\n"));
        QVERIFY(write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})"));
        QVERIFY(write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})"));

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorSeesAFirstPick() {
        // Before the first pick there is no ~/.config/omarchy to watch.
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/da.json", R"({"Save": "Gem"})"));
        Translator translator(dir.path() + "/omarchy/language", dir.path());
        QVERIFY(QDir(dir.path()).mkpath("omarchy"));
        QVERIFY(write(dir.path() + "/omarchy/language", "da\n"));
        QTRY_COMPARE(translator.translate("Main", "Save"), QString("Gem"));
    }

    void translatorPicksPluralForms() {
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/language", "ru\n"));
        QVERIFY(write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"},
                                                   "%n files": {"one": "%n file", "other": "%n files"}})"));
        QVERIFY(write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})"));

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        // An untranslated counted string falls back to English, with English rules.
        QCOMPARE(translator.translate("Main", "%n files", nullptr, 21), QString("%n files"));
    }

    void englishCatalogListsEveryString() {
        // en.json is what translators work from: every text the sources
        // translate is in it, as itself or, when counted, as its English
        // plural forms, and nothing else is.
        const QString sources = QFileInfo(QFINDTESTDATA("../src/Main.qml")).absolutePath();
        QStringList problems;
        const QMap<QString, bool> used = translatables(sources, &problems);
        const QJsonObject english = readCatalog(sources + "/i18n/en.json");
        for (auto it = used.begin(); it != used.end(); ++it) {
            const QJsonValue value = english.value(it.key());
            if (value.isUndefined())
                problems << "missing from en.json: " + it.key();
            else if (it.value() && (value.toObject().keys() != QStringList{"one", "other"}
                                    || value.toObject().value("other").toString() != it.key()))
                problems << "counted, so needs its one and other forms: " + it.key();
            else if (!it.value() && value.toString() != it.key())
                problems << "should be itself: " + it.key();
        }
        for (const QString &key : english.keys())
            if (!used.contains(key))
                problems << "in en.json but no source translates it: " + key;
        QVERIFY2(problems.isEmpty(), qPrintable(problems.join('\n')));
    }

    void catalogsMatchTheEnglishOne() {
        // A catalog may lack a string (it shows in English meanwhile), but what
        // it has translates something in en.json, with the same placeholders,
        // and a counted string has every form its language uses.
        const QString folder = QFileInfo(QFINDTESTDATA("../src/i18n/en.json")).absolutePath();
        const QJsonObject english = readCatalog(folder + "/en.json");
        QStringList problems;
        for (const QFileInfo &file : QDir(folder).entryInfoList({"*.json"}, QDir::Files)) {
            const QString code = file.completeBaseName();
            const QJsonObject catalog = readCatalog(file.filePath());
            if (catalog.isEmpty())
                problems << code + ": not a JSON object, or empty";
            // Some languages reach a form only at a million ("many" in French).
            QSet<QString> categories{"other", Translator::pluralCategory(code, 1000000)};
            for (long long n = 0; n <= 1000; ++n)
                categories << Translator::pluralCategory(code, n);
            for (auto it = catalog.begin(); it != catalog.end(); ++it) {
                const QString where = code + ": " + it.key() + ": ";
                const QJsonValue source = english.value(it.key());
                if (source.isUndefined()) {
                    problems << where + "not in en.json";
                } else if (source.isString()) {
                    if (!it.value().isString() || it.value().toString().isEmpty())
                        problems << where + "needs a translation";
                    else if (placeholders(it.value().toString()) != placeholders(it.key()))
                        problems << where + "has other placeholders than the English";
                } else {
                    const QJsonObject forms = it.value().toObject();
                    const QStringList wanted = placeholders(source.toObject().value("other").toString());
                    for (const QString &category : categories)
                        if (forms.value(category).toString().isEmpty())
                            problems << where + "needs its " + category + " form";
                    // A form other than "other" may say its number in words.
                    for (auto form = forms.begin(); form != forms.end(); ++form) {
                        const QStringList has = placeholders(form.value().toString());
                        QStringList needed = wanted;
                        if (form.key() != "other")
                            needed.removeIf([&](const QString &p) { return p.endsWith('n') && !has.contains(p); });
                        if (has != needed)
                            problems << where + form.key() + " has other placeholders than the English";
                    }
                }
            }
        }
        QVERIFY2(problems.isEmpty(), qPrintable(problems.join('\n')));
    }

    void initialMoveTree() {
        const auto p = Chess::Position::initial();
        QCOMPARE(Chess::perft(p,1), uint64_t(20));
        QCOMPARE(Chess::perft(p,2), uint64_t(400));
        QCOMPARE(Chess::perft(p,3), uint64_t(8902));
        QCOMPARE(Chess::perft(p,4), uint64_t(197281));
    }

    void specialMoveTrees() {
        const auto castle = Chess::Position::fromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        QCOMPARE(Chess::perft(castle,1), uint64_t(48));
        QCOMPARE(Chess::perft(castle,2), uint64_t(2039));
        QCOMPARE(Chess::perft(castle,3), uint64_t(97862));
        const auto ep = Chess::Position::fromFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
        QCOMPARE(Chess::perft(ep,3), uint64_t(2812));
        const auto promotion = Chess::Position::fromFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
        QCOMPARE(Chess::perft(promotion,3), uint64_t(9467));
    }

    void mateAndUndo() {
        Backend game(":memory:");
        game.newGame(false,0);
        QVERIFY(game.move(13,21));
        QVERIFY(game.move(52,36));
        QVERIFY(game.move(14,30));
        QVERIFY(game.move(59,31));
        QCOMPARE(game.outcome(),QString("black-wins"));
        QCOMPARE(game.moves().last(),QString("Qh4#"));
        QVERIFY(!game.move(12,20));
        game.undo();
        QVERIFY(!game.finished());
        QCOMPARE(game.moves().size(),3);
    }

    void enPassantAndCastling() {
        auto p = Chess::Position::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
        auto next = p.after({36,43});
        QCOMPARE(next.board[35].kind,int(Chess::Empty));
        QCOMPARE(next.board[43].kind,int(Chess::Pawn));
        QCOMPARE(p.san({36,43}),std::string("exd6"));
        auto castle = Chess::Position::fromFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        auto castled = castle.after({4,6});
        QCOMPARE(castled.board[5].kind,int(Chess::Rook));
        QCOMPARE(castled.board[6].kind,int(Chess::King));
        QCOMPARE(castled.castle,12);
        QCOMPARE(castle.san({4,6}),std::string("O-O"));
        const auto pinned = Chess::Position::fromFen("4r1k1/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
        const auto legal = pinned.legal();
        QVERIFY(std::find(legal.begin(),legal.end(),Chess::Move{36,43}) == legal.end());
    }

    void promotionAndDraws() {
        auto p = Chess::Position::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
        int promotions=0;
        for (auto m : p.legal()) if (m.from == 48 && m.to == 56) ++promotions;
        QCOMPARE(promotions,4);
        QCOMPARE(p.after({48,56,Chess::Knight}).board[56].kind,int(Chess::Knight));
        QVERIFY(Chess::Position::fromFen("4k3/8/8/8/8/8/8/4KB2 w - - 0 1").insufficient());
        QVERIFY(!Chess::Position::fromFen("4k3/8/8/8/8/8/8/4KBN1 w - - 0 1").insufficient());
        auto stale = Chess::Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        QVERIFY(stale.legal().empty());
        QVERIFY(!stale.check(false));
        Backend game(":memory:");
        game.newGame(false,0);
        for (int i=0;i<2;++i) {
            QVERIFY(game.move(6,21)); QVERIFY(game.move(62,45));
            QVERIFY(game.move(21,6)); QVERIFY(game.move(45,62));
        }
        QCOMPARE(game.outcome(),QString("repetition"));
    }

    void saveAndResume() {
        QTemporaryDir dir;
        QString fen;
        {
            Backend game(dir.filePath("game.json"));
            game.newGame(false,1);
            QVERIFY(game.move(12,28));
            QVERIFY(game.move(52,36));
            game.setPieceSet(3); game.setReducedMotion(true);
            fen = game.fen();
        }
        Backend restored(dir.filePath("game.json"));
        QCOMPARE(restored.fen(),fen);
        QCOMPARE(restored.moves(),QStringList({"e4","e5"}));
        QCOMPARE(restored.pieceSet(),3);
        QVERIFY(restored.reducedMotion());
        restored.undo();
        QCOMPARE(restored.moves().size(),1);
    }

    void computerMovesAndCancels() {
        Backend game(":memory:");
        game.newGame(true,0);
        QVERIFY(game.move(12,28));
        QTRY_COMPARE_WITH_TIMEOUT(game.moves().size(),2,5000);
        QVERIFY(game.whiteTurn());
        game.undo();
        QCOMPARE(game.moves().size(),0);
        game.newGame(true,2,false);
        QVERIFY(game.thinking());
        game.newGame(false,0);
        QTest::qWait(200);
        QCOMPARE(game.moves().size(),0);
        QVERIFY(!game.thinking());
        const auto p = Chess::Position::fromFen("7k/8/5KQ1/8/8/8/8/8 w - - 0 1");
        auto move = Chess::bestMove(p,1,std::make_shared<std::atomic_bool>(false));
        auto next = p.after(move);
        QVERIFY(next.check(false));
        QVERIFY(next.legal().empty());
    }

    void windowLoadsAndAnswersTheKeyboard() {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language");
        QCoreApplication::installTranslator(&translator);
        Theme theme(dir.path());
        Backend backend(":memory:");
        backend.newGame(false,0);
        backend.setReducedMotion(true);
        QQmlApplicationEngine engine;
        QStringList warnings;
        connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors) {
            for (const auto &error : errors) warnings << error.toString();
        });
        engine.rootContext()->setContextProperty("translator",&translator);
        engine.rootContext()->setContextProperty("theme",&theme);
        engine.rootContext()->setContextProperty("backend",&backend);
        engine.load(QUrl("qrc:/Main.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(window));
        QTest::keyClick(window,Qt::Key_Space);
        QCOMPARE(backend.selected(),12);
        QTest::keyClick(window,Qt::Key_Up);
        QTest::keyClick(window,Qt::Key_Up);
        QTest::keyClick(window,Qt::Key_Space);
        QCOMPARE(backend.moves(),QStringList{"e4"});
        QTest::keyClick(window,Qt::Key_Z,Qt::ControlModifier);
        QVERIFY(backend.moves().isEmpty());
        for (int set = 0; set < 4; ++set) {
            backend.setPieceSet(set);
            QTest::qWait(80);
        }
        QTest::keyClick(window,'?');
        auto *help = window->findChild<QObject *>("help");
        QTRY_VERIFY(help->property("opened").toBool());
        QTest::keyClick(window,Qt::Key_Escape);
        QTRY_VERIFY(!help->property("visible").toBool());
        QTest::keyClick(window,Qt::Key_N);
        auto *dialog = window->findChild<QObject *>("newGameDialog");
        QTRY_VERIFY(dialog->property("opened").toBool());
        QTest::keyClick(window,Qt::Key_Escape);
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
        QCoreApplication::removeTranslator(&translator);
    }
};

QTEST_MAIN(CheckmateTests)
#include "checkmate_tests.moc"
