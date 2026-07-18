#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "exporter/YoloExporter.h"

namespace {

QString createSourceImage(
    const QString& directory,
    const QString& fileName
)
{
    const QString path = QDir(directory).filePath(fileName);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("test-image-data");
    }
    return path;
}

ImageModel makeExportImage(
    ImageId id,
    const QString& path,
    int width = 640,
    int height = 480
)
{
    ImageModel image;
    image.id = id;
    image.filePath = path;
    image.fileName = QFileInfo(path).fileName();
    image.relativePath = image.fileName;
    image.width = width;
    image.height = height;
    return image;
}

QString readUtf8(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

} // namespace

class YoloExporterTests final : public QObject {
    Q_OBJECT

private slots:
    void exportsCompleteDatasetWithContinuousClassIds();
    void exportsEmptyLabelFileForUnannotatedImage();
    void rejectsDuplicateOutputNames();
    void rejectsNonEmptyOutputDirectoryWithoutChangingIt();
    void rejectsMissingSourceImageWithoutCreatingDataset();
    void rejectsInvalidAnnotationWithoutCreatingDataset();
};

void YoloExporterTests::exportsCompleteDatasetWithContinuousClassIds()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/yolo-export-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    LabelModel car;
    car.id = 4;
    car.name = QStringLiteral("car");
    car.color = Qt::red;

    LabelModel person;
    person.id = 9;
    person.name = QStringLiteral("person");
    person.color = Qt::green;

    const QString imagePath = createSourceImage(
        workspace.path(),
        QStringLiteral("scene.jpg")
    );
    ImageModel image = makeExportImage(1, imagePath);

    AnnotationModel first;
    first.id = 1;
    first.labelId = 9;
    first.imageRect = QRectF(64, 48, 128, 96);
    image.annotations.push_back(first);

    AnnotationModel second;
    second.id = 2;
    second.labelId = 4;
    second.imageRect = QRectF(320, 120, 64, 240);
    image.annotations.push_back(second);

    ProjectModel project;
    const Result<void> replaceResult = project.replaceProjectData(
        {image},
        {car, person},
        0
    );
    QVERIFY2(replaceResult.isSuccess(), qPrintable(replaceResult.error()));

    const QString output = workspace.filePath(QStringLiteral("dataset"));
    YoloExporter exporter;
    const Result<void> result = exporter.exportDataset(project, output);
    QVERIFY2(result.isSuccess(), qPrintable(result.error()));

    QVERIFY(QFileInfo::exists(QDir(output).filePath(QStringLiteral("images/scene.jpg"))));
    QVERIFY(QFileInfo::exists(QDir(output).filePath(QStringLiteral("labels/scene.txt"))));

    QCOMPARE(
        readUtf8(QDir(output).filePath(QStringLiteral("classes.txt"))),
        QStringLiteral("car\nperson\n")
    );
    QCOMPARE(
        readUtf8(QDir(output).filePath(QStringLiteral("labels/scene.txt"))),
        QStringLiteral(
            "1 0.200000 0.200000 0.200000 0.200000\n"
            "0 0.550000 0.500000 0.100000 0.500000\n"
        )
    );

    const QString yaml = readUtf8(
        QDir(output).filePath(QStringLiteral("data.yaml"))
    );
    QVERIFY(yaml.contains(QStringLiteral("train: images")));
    QVERIFY(yaml.contains(QStringLiteral("val: images")));
    QVERIFY(yaml.contains(QStringLiteral("0: 'car'")));
    QVERIFY(yaml.contains(QStringLiteral("1: 'person'")));

    QCOMPARE(project.labels().at(0).id, 4);
    QCOMPARE(project.labels().at(1).id, 9);
}

void YoloExporterTests::exportsEmptyLabelFileForUnannotatedImage()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/yolo-empty-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    const QString imagePath = createSourceImage(
        workspace.path(),
        QStringLiteral("negative.png")
    );
    ImageModel image = makeExportImage(1, imagePath);

    ProjectModel project;
    const Result<void> replaceResult = project.replaceProjectData(
        {image},
        {LabelModel{}},
        0
    );
    QVERIFY(replaceResult.isSuccess());

    const QString output = workspace.filePath(QStringLiteral("dataset"));
    YoloExporter exporter;
    const Result<void> result = exporter.exportDataset(project, output);
    QVERIFY2(result.isSuccess(), qPrintable(result.error()));

    const QString labelPath = QDir(output).filePath(
        QStringLiteral("labels/negative.txt")
    );
    QVERIFY(QFileInfo::exists(labelPath));
    QCOMPARE(QFileInfo(labelPath).size(), static_cast<qint64>(0));
}

void YoloExporterTests::rejectsDuplicateOutputNames()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/yolo-duplicate-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());
    QVERIFY(QDir().mkpath(workspace.filePath(QStringLiteral("a"))));
    QVERIFY(QDir().mkpath(workspace.filePath(QStringLiteral("b"))));

    ImageModel first = makeExportImage(
        1,
        createSourceImage(
            workspace.filePath(QStringLiteral("a")),
            QStringLiteral("same.jpg")
        )
    );
    ImageModel second = makeExportImage(
        2,
        createSourceImage(
            workspace.filePath(QStringLiteral("b")),
            QStringLiteral("same.png")
        )
    );

    ProjectModel project;
    const Result<void> replaceResult = project.replaceProjectData(
        {first, second},
        {LabelModel{}},
        0
    );
    QVERIFY(replaceResult.isSuccess());

    const QString output = workspace.filePath(QStringLiteral("dataset"));
    YoloExporter exporter;
    const Result<void> result = exporter.exportDataset(project, output);
    QVERIFY(!result.isSuccess());
    QVERIFY(!QFileInfo::exists(output));
}

void YoloExporterTests::rejectsNonEmptyOutputDirectoryWithoutChangingIt()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/yolo-existing-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    ProjectModel project;
    ImageModel image = makeExportImage(
        1,
        createSourceImage(workspace.path(), QStringLiteral("scene.jpg"))
    );
    QVERIFY(project.replaceProjectData({image}, {LabelModel{}}, 0).isSuccess());

    const QString output = workspace.filePath(QStringLiteral("dataset"));
    QVERIFY(QDir().mkpath(output));
    const QString markerPath = QDir(output).filePath(QStringLiteral("keep.txt"));
    QFile marker(markerPath);
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.write("keep");
    marker.close();

    YoloExporter exporter;
    const Result<void> result = exporter.exportDataset(project, output);
    QVERIFY(!result.isSuccess());
    QCOMPARE(readUtf8(markerPath), QStringLiteral("keep"));
    QVERIFY(!QFileInfo::exists(QDir(output).filePath(QStringLiteral("images"))));
}

void YoloExporterTests::rejectsMissingSourceImageWithoutCreatingDataset()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/yolo-missing-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    ImageModel image = makeExportImage(
        1,
        workspace.filePath(QStringLiteral("missing.jpg"))
    );
    ProjectModel project;
    QVERIFY(project.replaceProjectData({image}, {LabelModel{}}, 0).isSuccess());

    const QString output = workspace.filePath(QStringLiteral("dataset"));
    YoloExporter exporter;
    const Result<void> result = exporter.exportDataset(project, output);
    QVERIFY(!result.isSuccess());
    QVERIFY(!QFileInfo::exists(output));
}

void YoloExporterTests::rejectsInvalidAnnotationWithoutCreatingDataset()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/yolo-invalid-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    ImageModel image = makeExportImage(
        1,
        createSourceImage(workspace.path(), QStringLiteral("scene.jpg"))
    );
    AnnotationModel annotation;
    annotation.id = 1;
    annotation.labelId = 99;
    annotation.imageRect = QRectF(10, 10, 20, 20);
    image.annotations.push_back(annotation);

    ProjectModel project;
    project.replaceImages({image});

    const QString output = workspace.filePath(QStringLiteral("dataset"));
    YoloExporter exporter;
    const Result<void> result = exporter.exportDataset(project, output);
    QVERIFY(!result.isSuccess());
    QVERIFY(!QFileInfo::exists(output));
}

QTEST_APPLESS_MAIN(YoloExporterTests)

#include "YoloExporterTests.moc"
