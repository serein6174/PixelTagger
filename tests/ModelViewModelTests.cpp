#include <QtTest>

#include "model/ProjectModel.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/LabelViewModel.h"

namespace {

ImageModel makeImage()
{
    ImageModel image;
    image.filePath = QStringLiteral("C:/test/images/sample.png");
    image.fileName = QStringLiteral("sample.png");
    image.relativePath = QStringLiteral("images/sample.png");
    image.width = 640;
    image.height = 480;
    return image;
}

} // namespace

class ModelViewModelTests final : public QObject {
    Q_OBJECT

private slots:
    void modelManagesMultipleLabels();
    void modelProtectsReferencedAndLastLabels();
    void loadedProjectContinuesStableLabelIds();
    void viewModelsCreateAnnotationsWithSelectedLabels();
    void viewModelRejectsInvalidSelectionAndDeletion();
};

void ModelViewModelTests::modelManagesMultipleLabels()
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

    const Result<void> renameResult = project.renameLabel(carId, QStringLiteral(" person "));
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

void ModelViewModelTests::modelProtectsReferencedAndLastLabels()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeImage());

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

void ModelViewModelTests::loadedProjectContinuesStableLabelIds()
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

void ModelViewModelTests::viewModelsCreateAnnotationsWithSelectedLabels()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeImage());

    LabelViewModel labels(project);
    AnnotationViewModel annotations(project);
    QObject::connect(
        &labels, &LabelViewModel::currentLabelChanged,
        &annotations, &AnnotationViewModel::setCurrentLabelId
    );

    QSignalSpy labelsChangedSpy(&labels, &LabelViewModel::labelsChanged);
    QSignalSpy currentLabelSpy(&labels, &LabelViewModel::currentLabelChanged);
    QSignalSpy annotationsChangedSpy(&annotations, &AnnotationViewModel::changed);

    const QColor carColor(40, 120, 220);
    labels.addLabel(QStringLiteral("car"), carColor);
    const LabelId carId = labels.currentLabelId();

    QCOMPARE(labelsChangedSpy.count(), 1);
    QCOMPARE(currentLabelSpy.count(), 1);
    QCOMPARE(currentLabelSpy.at(0).at(0).toInt(), carId);
    QCOMPARE(annotations.currentLabelId(), carId);

    annotations.createAnnotation(QRectF(10, 20, 100, 80));
    QCOMPARE(project.currentImage()->annotations.size(), 1);
    QCOMPARE(project.currentImage()->annotations.back().labelId, carId);

    const QColor personColor(20, 200, 100);
    labels.addLabel(QStringLiteral("person"), personColor);
    const LabelId personId = labels.currentLabelId();
    QVERIFY(personId != carId);
    QCOMPARE(annotations.currentLabelId(), personId);

    annotations.createAnnotation(QRectF(200, 100, 70, 120));
    QCOMPARE(project.currentImage()->annotations.size(), 2);
    QCOMPARE(project.currentImage()->annotations.back().labelId, personId);
    QCOMPARE(annotationsChangedSpy.count(), 2);

    const QVector<AnnotationRenderData> items = annotations.annotationItems();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).labelName, QStringLiteral("car"));
    QCOMPARE(items.at(0).color, carColor);
    QCOMPARE(items.at(1).labelName, QStringLiteral("person"));
    QCOMPARE(items.at(1).color, personColor);

    labels.setCurrentLabel(carId);
    QCOMPARE(annotations.currentLabelId(), carId);
    annotations.createAnnotation(QRectF(320, 240, 60, 60));
    QCOMPARE(project.currentImage()->annotations.back().labelId, carId);
}

void ModelViewModelTests::viewModelRejectsInvalidSelectionAndDeletion()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeImage());

    LabelViewModel labels(project);
    AnnotationViewModel annotations(project);
    QObject::connect(
        &labels, &LabelViewModel::currentLabelChanged,
        &annotations, &AnnotationViewModel::setCurrentLabelId
    );

    QSignalSpy labelErrorSpy(&labels, &LabelViewModel::errorOccurred);
    QSignalSpy annotationErrorSpy(&annotations, &AnnotationViewModel::errorOccurred);

    labels.setCurrentLabel(9999);
    QCOMPARE(labelErrorSpy.count(), 1);
    QCOMPARE(labels.currentLabelId(), project.defaultLabel()->id);

    annotations.setCurrentLabelId(9999);
    QCOMPARE(annotationErrorSpy.count(), 1);
    QCOMPARE(annotations.currentLabelId(), project.defaultLabel()->id);

    labels.addLabel(QStringLiteral("car"), QColor(40, 120, 220));
    const LabelId carId = labels.currentLabelId();
    annotations.createAnnotation(QRectF(10, 20, 100, 80));

    labels.removeLabel(carId);
    QCOMPARE(labelErrorSpy.count(), 2);
    QVERIFY(project.findLabel(carId) != nullptr);
}

QTEST_APPLESS_MAIN(ModelViewModelTests)

#include "ModelViewModelTests.moc"
