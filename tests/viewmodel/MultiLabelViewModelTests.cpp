#include <QtTest>

#include "TestFixtures.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/LabelViewModel.h"

class MultiLabelViewModelTests final : public QObject {
    Q_OBJECT

private slots:
    void createsAnnotationsWithSelectedLabels();
    void restoresLabelsAfterRenameFailure();
    void rejectsInvalidSelectionAndDeletion();
};

void MultiLabelViewModelTests::createsAnnotationsWithSelectedLabels()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeTestImage());

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

void MultiLabelViewModelTests::restoresLabelsAfterRenameFailure()
{
    ProjectModel project;
    LabelViewModel labels(project);

    labels.addLabel(QStringLiteral("car"), QColor(40, 120, 220));
    const LabelId carId = labels.currentLabelId();

    QSignalSpy labelsChangedSpy(&labels, &LabelViewModel::labelsChanged);
    QSignalSpy errorSpy(&labels, &LabelViewModel::errorOccurred);

    labels.renameLabel(carId, QString{});
    QCOMPARE(labelsChangedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(project.findLabel(carId)->name, QStringLiteral("car"));

    labels.renameLabel(carId, QStringLiteral("OBJECT"));
    QCOMPARE(labelsChangedSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 2);
    QCOMPARE(project.findLabel(carId)->name, QStringLiteral("car"));
}

void MultiLabelViewModelTests::rejectsInvalidSelectionAndDeletion()
{
    ProjectModel project;
    project.replaceWithSingleImage(makeTestImage());

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

QTEST_APPLESS_MAIN(MultiLabelViewModelTests)

#include "MultiLabelViewModelTests.moc"
