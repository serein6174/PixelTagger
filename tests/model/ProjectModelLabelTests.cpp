#include <QtTest>

#include "TestFixtures.h"
#include "model/ProjectModel.h"

class ProjectModelLabelTests final : public QObject {
    Q_OBJECT

private slots:
    void managesMultipleLabels();
    void protectsReferencedAndLastLabels();
    void loadedProjectContinuesStableLabelIds();
};

void ProjectModelLabelTests::managesMultipleLabels()
{
    ProjectModel project;
    QCOMPARE(project.labels().size(), 1);
    QCOMPARE(project.labels().front().name, QStringLiteral("object"));
    QVERIFY(!project.isModified());

    const QColor carColor(40, 120, 220);
    const Result<LabelId> carResult = project.addLabel(QStringLiteral(" car "), carColor);
    QVERIFY2(carResult.isSuccess(), qPrintable(carResult.error()));
    const LabelId carId = carResult.value();

    QCOMPARE(project.labels().size(), 2);
    QCOMPARE(project.findLabel(carId)->name, QStringLiteral("car"));
    QCOMPARE(project.findLabel(carId)->color, carColor);
    QVERIFY(project.isModified());

    const Result<LabelId> duplicate = project.addLabel(
        QStringLiteral("CAR"), QColor(1, 2, 3)
    );
    QVERIFY(!duplicate.isSuccess());

    const Result<void> renameResult = project.renameLabel(
        carId, QStringLiteral(" person ")
    );
    QVERIFY2(renameResult.isSuccess(), qPrintable(renameResult.error()));
    QCOMPARE(project.findLabel(carId)->name, QStringLiteral("person"));

    const Result<void> duplicateRename = project.renameLabel(
        carId, QStringLiteral("OBJECT")
    );
    QVERIFY(!duplicateRename.isSuccess());

    const QColor personColor(20, 200, 100);
    const Result<void> colorResult = project.setLabelColor(carId, personColor);
    QVERIFY2(colorResult.isSuccess(), qPrintable(colorResult.error()));
    QCOMPARE(project.findLabel(carId)->color, personColor);

    const Result<void> removeResult = project.removeLabel(carId);
    QVERIFY2(removeResult.isSuccess(), qPrintable(removeResult.error()));
    QCOMPARE(project.labels().size(), 1);
    QVERIFY(project.findLabel(carId) == nullptr);
}

void ProjectModelLabelTests::protectsReferencedAndLastLabels()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeTestImage());

    const Result<LabelId> carResult = project.addLabel(
        QStringLiteral("car"), QColor(40, 120, 220)
    );
    QVERIFY(carResult.isSuccess());
    const LabelId carId = carResult.value();

    QVERIFY(project.addAnnotationToCurrentImage(QRectF(10, 20, 100, 80), carId));
    QVERIFY(project.isLabelInUse(carId));

    const Result<void> referencedRemoval = project.removeLabel(carId);
    QVERIFY(!referencedRemoval.isSuccess());
    QVERIFY(project.findLabel(carId) != nullptr);

    const LabelId objectId = project.defaultLabel()->id;
    const Result<void> objectRemoval = project.removeLabel(objectId);
    QVERIFY2(objectRemoval.isSuccess(), qPrintable(objectRemoval.error()));
    QCOMPARE(project.labels().size(), 1);

    const Result<void> lastRemoval = project.removeLabel(carId);
    QVERIFY(!lastRemoval.isSuccess());

    project.markSaved();
    QVERIFY(!project.isModified());
    QVERIFY(!project.currentImage()->modified);
}

void ProjectModelLabelTests::loadedProjectContinuesStableLabelIds()
{
    QVector<LabelModel> labels;

    LabelModel first;
    first.id = 4;
    first.name = QStringLiteral(" car ");
    first.color = QColor(40, 120, 220);
    labels.push_back(first);

    LabelModel second;
    second.id = 9;
    second.name = QStringLiteral("person");
    second.color = QColor(20, 200, 100);
    labels.push_back(second);

    ProjectModel project;
    const Result<void> replaceResult = project.replaceProjectData({}, labels, -1);
    QVERIFY2(replaceResult.isSuccess(), qPrintable(replaceResult.error()));
    QCOMPARE(project.labels().front().name, QStringLiteral("car"));

    const Result<LabelId> nextResult = project.addLabel(
        QStringLiteral("bike"), QColor(230, 170, 30)
    );
    QVERIFY2(nextResult.isSuccess(), qPrintable(nextResult.error()));
    QCOMPARE(nextResult.value(), 10);
}

QTEST_APPLESS_MAIN(ProjectModelLabelTests)

#include "ProjectModelLabelTests.moc"
