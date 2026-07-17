#include <QtTest>

#include <QAction>
#include <QKeySequence>

#include "view/MainWindow.h"

class MainWindowAnnotationTests final : public QObject {
    Q_OBJECT

private slots:
    void deleteActionFollowsSelectionAndEmitsRequest();
};

void MainWindowAnnotationTests::deleteActionFollowsSelectionAndEmitsRequest()
{
    MainWindow window;
    QAction* deleteAction = window.findChild<QAction*>(
        QStringLiteral("deleteSelectedAnnotationAction")
    );

    QVERIFY(deleteAction != nullptr);
    QVERIFY(!deleteAction->isEnabled());
    QCOMPARE(deleteAction->shortcut(), QKeySequence(QKeySequence::Delete));
    QCOMPARE(deleteAction->shortcutContext(), Qt::WindowShortcut);

    QSignalSpy deleteSpy(
        &window,
        &MainWindow::deleteSelectedAnnotationRequested
    );

    window.setAnnotationEditEnabled(true);
    QVERIFY(deleteAction->isEnabled());
    deleteAction->trigger();
    QCOMPARE(deleteSpy.count(), 1);

    window.setAnnotationEditEnabled(false);
    QVERIFY(!deleteAction->isEnabled());
}

QTEST_MAIN(MainWindowAnnotationTests)

#include "MainWindowAnnotationTests.moc"
