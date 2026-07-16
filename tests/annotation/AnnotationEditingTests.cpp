#include <QtTest>

#include "TestFixtures.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/LabelViewModel.h"

class AnnotationEditingTests final : public QObject {
    Q_OBJECT

private slots:
    void modelEditsAnnotationsWithValidation();
    void viewModelEditsSelectedAnnotation();
    void viewModelClearsSelectionWhenImageChanges();
};

void AnnotationEditingTests::modelEditsAnnotationsWithValidation()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeTestImage());

    const LabelId objectId = project.defaultLabel()->id;
    const Result<LabelId> personResult = project.addLabel(
        QStringLiteral("person"), QColor(20, 200, 100)
    );
    QVERIFY(personResult.isSuccess());
    const LabelId personId = personResult.value();

    QVERIFY(project.addAnnotationToCurrentImage(
        QRectF(10, 20, 100, 80), objectId
    ));
    const AnnotationId annotationId =
        project.currentImage()->annotations.front().id;
    project.markSaved();

    const Result<void> rectResult = project.updateAnnotationRect(
        annotationId, QRectF(30, 40, 120, 90)
    );
    QVERIFY2(rectResult.isSuccess(), qPrintable(rectResult.error()));
    QCOMPARE(
        project.findAnnotationInCurrentImage(annotationId)->imageRect,
        QRectF(30, 40, 120, 90)
    );
    QVERIFY(project.isModified());
    QVERIFY(project.currentImage()->modified);

    const Result<void> outOfBounds = project.updateAnnotationRect(
        annotationId, QRectF(600, 440, 100, 80)
    );
    QVERIFY(!outOfBounds.isSuccess());
    QCOMPARE(
        project.findAnnotationInCurrentImage(annotationId)->imageRect,
        QRectF(30, 40, 120, 90)
    );

    const Result<void> labelResult = project.changeAnnotationLabel(
        annotationId, personId
    );
    QVERIFY2(labelResult.isSuccess(), qPrintable(labelResult.error()));
    QCOMPARE(
        project.findAnnotationInCurrentImage(annotationId)->labelId,
        personId
    );

    const Result<void> missingLabel = project.changeAnnotationLabel(
        annotationId, 9999
    );
    QVERIFY(!missingLabel.isSuccess());

    const Result<void> removal = project.removeAnnotationFromCurrentImage(
        annotationId
    );
    QVERIFY2(removal.isSuccess(), qPrintable(removal.error()));
    QVERIFY(project.findAnnotationInCurrentImage(annotationId) == nullptr);
    QVERIFY(!project.isLabelInUse(personId));
}

void AnnotationEditingTests::viewModelEditsSelectedAnnotation()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeTestImage());

    LabelViewModel labels(project);
    AnnotationViewModel annotations(project);
    QObject::connect(
        &labels, &LabelViewModel::currentLabelChanged,
        &annotations, &AnnotationViewModel::setCurrentLabelId
    );

    labels.addLabel(QStringLiteral("person"), QColor(20, 200, 100));
    const LabelId personId = labels.currentLabelId();
    labels.setCurrentLabel(project.defaultLabel()->id);
    annotations.createAnnotation(QRectF(10, 20, 100, 80));
    const AnnotationId annotationId =
        project.currentImage()->annotations.front().id;

    QSignalSpy selectionSpy(&annotations, &AnnotationViewModel::selectionChanged);
    QSignalSpy changedSpy(&annotations, &AnnotationViewModel::changed);
    QSignalSpy errorSpy(&annotations, &AnnotationViewModel::errorOccurred);

    annotations.selectAnnotation(annotationId);
    QVERIFY(annotations.selectedAnnotationId().has_value());
    QCOMPARE(annotations.selectedAnnotationId().value(), annotationId);
    QCOMPARE(selectionSpy.count(), 1);
    QVERIFY(annotations.annotationItems().front().selected);

    annotations.updateSelectedAnnotationRect(QRectF(30, 40, 120, 90));
    QCOMPARE(
        project.findAnnotationInCurrentImage(annotationId)->imageRect,
        QRectF(30, 40, 120, 90)
    );

    annotations.setSelectedAnnotationLabel(personId);
    QCOMPARE(
        project.findAnnotationInCurrentImage(annotationId)->labelId,
        personId
    );
    QCOMPARE(annotations.annotationItems().front().labelName, QStringLiteral("person"));

    annotations.updateSelectedAnnotationRect(QRectF(0, 0, 2, 2));
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(
        project.findAnnotationInCurrentImage(annotationId)->imageRect,
        QRectF(30, 40, 120, 90)
    );

    annotations.deleteSelectedAnnotation();
    QVERIFY(!annotations.selectedAnnotationId().has_value());
    QCOMPARE(selectionSpy.count(), 2);
    QVERIFY(project.currentImage()->annotations.isEmpty());
    QVERIFY(changedSpy.count() >= 4);
}

void AnnotationEditingTests::viewModelClearsSelectionWhenImageChanges()
{
    ProjectModel project;
    QVector<ImageModel> images;
    images.push_back(makeTestImage());
    images.push_back(makeTestImage(QStringLiteral("second.png")));
    project.replaceImages(images);

    AnnotationViewModel annotations(project);
    annotations.createAnnotation(QRectF(10, 20, 100, 80));
    const AnnotationId annotationId =
        project.currentImage()->annotations.front().id;
    annotations.selectAnnotation(annotationId);
    QVERIFY(annotations.selectedAnnotationId().has_value());

    QSignalSpy selectionSpy(&annotations, &AnnotationViewModel::selectionChanged);
    QVERIFY(project.moveNextImage());
    annotations.onImageViewModelChanged(ViewModelChange::CurrentImage);

    QVERIFY(!annotations.selectedAnnotationId().has_value());
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.at(0).at(0).toBool(), false);
    QVERIFY(annotations.annotationItems().isEmpty());
}

QTEST_APPLESS_MAIN(AnnotationEditingTests)

#include "AnnotationEditingTests.moc"
