#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "exporter/YoloExporter.h"
#include "repository/JsonProjectRepository.h"
#include "viewmodel/ProjectViewModel.h"

namespace {

ImageModel makeViewModelExportImage(const QString& path)
{
    ImageModel image;
    image.id = 1;
    image.filePath = path;
    image.fileName = QFileInfo(path).fileName();
    image.relativePath = image.fileName;
    image.width = 100;
    image.height = 50;
    return image;
}

} // namespace

class ProjectExportViewModelTests final : public QObject {
    Q_OBJECT

private slots:
    void publishesSuccessAfterExport();
    void publishesExporterError();
    void publishesProjectChangedAfterOpen();
};

void ProjectExportViewModelTests::publishesSuccessAfterExport()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/project-export-vm-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    const QString imagePath = workspace.filePath(QStringLiteral("image.jpg"));
    QFile imageFile(imagePath);
    QVERIFY(imageFile.open(QIODevice::WriteOnly));
    imageFile.write("image");
    imageFile.close();

    ProjectModel project;
    QVERIFY(project.replaceProjectData(
        {makeViewModelExportImage(imagePath)},
        {LabelModel{}},
        0
    ).isSuccess());

    JsonProjectRepository repository;
    YoloExporter exporter;
    ProjectViewModel viewModel(project, repository, exporter);

    QSignalSpy statusSpy(&viewModel, &ProjectViewModel::statusChanged);
    QSignalSpy errorSpy(&viewModel, &ProjectViewModel::errorOccurred);
    const QString output = workspace.filePath(QStringLiteral("dataset"));
    viewModel.exportYolo(output);

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 1);
    QVERIFY(QFileInfo::exists(
        QDir(output).filePath(QStringLiteral("classes.txt"))
    ));
}

void ProjectExportViewModelTests::publishesExporterError()
{
    ProjectModel project;
    JsonProjectRepository repository;
    YoloExporter exporter;
    ProjectViewModel viewModel(project, repository, exporter);

    QSignalSpy statusSpy(&viewModel, &ProjectViewModel::statusChanged);
    QSignalSpy errorSpy(&viewModel, &ProjectViewModel::errorOccurred);
    viewModel.exportYolo(QStringLiteral("unused-output"));

    QCOMPARE(statusSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);
}

void ProjectExportViewModelTests::publishesProjectChangedAfterOpen()
{
    QTemporaryDir workspace(
        QDir::currentPath() + QStringLiteral("/project-open-vm-test-XXXXXX")
    );
    QVERIFY(workspace.isValid());

    const QString imagePath = workspace.filePath(QStringLiteral("image.jpg"));
    QFile imageFile(imagePath);
    QVERIFY(imageFile.open(QIODevice::WriteOnly));
    imageFile.write("image");
    imageFile.close();

    ProjectModel sourceProject;
    QVERIFY(sourceProject.replaceProjectData(
        {makeViewModelExportImage(imagePath)},
        {LabelModel{}},
        0
    ).isSuccess());

    const QString projectPath = workspace.filePath(QStringLiteral("project.json"));
    JsonProjectRepository repository;
    QVERIFY(repository.save(sourceProject, projectPath).isSuccess());

    ProjectModel targetProject;
    YoloExporter exporter;
    ProjectViewModel viewModel(targetProject, repository, exporter);
    QSignalSpy projectChangedSpy(
        &viewModel,
        &ProjectViewModel::projectChanged
    );
    QSignalSpy errorSpy(&viewModel, &ProjectViewModel::errorOccurred);

    viewModel.openProject(projectPath);

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(projectChangedSpy.count(), 1);
    QCOMPARE(targetProject.images().size(), 1);
}

QTEST_APPLESS_MAIN(ProjectExportViewModelTests)

#include "ProjectExportViewModelTests.moc"
