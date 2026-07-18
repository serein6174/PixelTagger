#include "exporter/YoloExporter.h"

#include <cmath>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>

#include <utility>

namespace {

struct ExportPlan final {
    QHash<LabelId, int> classIds;
    QStringList classNames;
};

bool containsLineBreak(const QString& text)
{
    return text.contains(QLatin1Char('\n')) ||
           text.contains(QLatin1Char('\r'));
}

bool isNormalizedCoordinate(double value)
{
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

QString yamlQuoted(const QString& text)
{
    QString escaped = text;
    escaped.replace(QLatin1Char('\''), QStringLiteral("''"));
    return QStringLiteral("'%1'").arg(escaped);
}

Result<void> writeFile(const QString& path, const QByteArray& data)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<void>::failure(
            QStringLiteral("无法创建导出文件：%1").arg(path)
        );
    }
    if (file.write(data) != data.size() || !file.commit()) {
        return Result<void>::failure(
            QStringLiteral("导出文件写入失败：%1").arg(path)
        );
    }
    return Result<void>::success();
}

Result<ExportPlan> buildExportPlan(const ProjectModel& project)
{
    if (project.images().isEmpty()) {
        return Result<ExportPlan>::failure(QStringLiteral("项目中没有可导出的图片"));
    }
    if (project.labels().isEmpty()) {
        return Result<ExportPlan>::failure(QStringLiteral("项目中没有可导出的类别"));
    }

    ExportPlan plan;
    QSet<QString> imageFileNames;
    QSet<QString> labelFileNames;

    for (int index = 0; index < project.labels().size(); ++index) {
        const LabelModel& label = project.labels().at(index);
        if (label.name.trimmed().isEmpty() || containsLineBreak(label.name)) {
            return Result<ExportPlan>::failure(
                QStringLiteral("类别名称无法写入 YOLO 类别文件：%1").arg(label.name)
            );
        }
        plan.classIds.insert(label.id, index);
        plan.classNames.push_back(label.name);
    }

    for (const ImageModel& image : project.images()) {
        const QFileInfo sourceInfo(image.filePath);
        if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable()) {
            return Result<ExportPlan>::failure(
                QStringLiteral("源图片不存在或不可读：%1").arg(image.filePath)
            );
        }
        if (image.width <= 0 || image.height <= 0) {
            return Result<ExportPlan>::failure(
                QStringLiteral("图片尺寸无效：%1").arg(image.fileName)
            );
        }

        const QString fileName = sourceInfo.fileName();
        const QString imageKey = fileName.toCaseFolded();
        const QString labelFileName = sourceInfo.completeBaseName() +
                                      QStringLiteral(".txt");
        const QString labelKey = labelFileName.toCaseFolded();
        if (imageFileNames.contains(imageKey) ||
            labelFileNames.contains(labelKey)) {
            return Result<ExportPlan>::failure(
                QStringLiteral("导出文件名冲突：%1").arg(fileName)
            );
        }
        imageFileNames.insert(imageKey);
        labelFileNames.insert(labelKey);

        const QRectF bounds(0.0, 0.0, image.width, image.height);
        for (const AnnotationModel& annotation : image.annotations) {
            const QRectF& rect = annotation.imageRect;
            const bool finite = std::isfinite(rect.x()) &&
                                std::isfinite(rect.y()) &&
                                std::isfinite(rect.width()) &&
                                std::isfinite(rect.height());
            if (!plan.classIds.contains(annotation.labelId) || !finite ||
                rect != rect.normalized() ||
                rect.width() <= 0.0 || rect.height() <= 0.0 ||
                !bounds.contains(rect)) {
                return Result<ExportPlan>::failure(
                    QStringLiteral("图片包含非法标注：%1").arg(fileName)
                );
            }

            const double xCenter =
                (rect.x() + rect.width() / 2.0) / image.width;
            const double yCenter =
                (rect.y() + rect.height() / 2.0) / image.height;
            const double normalizedWidth = rect.width() / image.width;
            const double normalizedHeight = rect.height() / image.height;
            if (!isNormalizedCoordinate(xCenter) ||
                !isNormalizedCoordinate(yCenter) ||
                !isNormalizedCoordinate(normalizedWidth) ||
                !isNormalizedCoordinate(normalizedHeight) ||
                normalizedWidth <= 0.0 || normalizedHeight <= 0.0) {
                return Result<ExportPlan>::failure(
                    QStringLiteral("标注无法转换为合法 YOLO 坐标：%1")
                        .arg(fileName)
                );
            }
        }
    }

    return Result<ExportPlan>::success(std::move(plan));
}

QByteArray labelFileData(
    const ImageModel& image,
    const QHash<LabelId, int>& classIds
)
{
    QByteArray data;
    for (const AnnotationModel& annotation : image.annotations) {
        const QRectF& rect = annotation.imageRect;
        const double xCenter = (rect.x() + rect.width() / 2.0) / image.width;
        const double yCenter = (rect.y() + rect.height() / 2.0) / image.height;
        const double width = rect.width() / image.width;
        const double height = rect.height() / image.height;

        data += QStringLiteral("%1 %2 %3 %4 %5\n")
                    .arg(classIds.value(annotation.labelId))
                    .arg(xCenter, 0, 'f', 6)
                    .arg(yCenter, 0, 'f', 6)
                    .arg(width, 0, 'f', 6)
                    .arg(height, 0, 'f', 6)
                    .toUtf8();
    }
    return data;
}

QByteArray classesFileData(const QStringList& classNames)
{
    QByteArray data;
    for (const QString& name : classNames) {
        data += name.toUtf8();
        data += '\n';
    }
    return data;
}

QByteArray dataYamlData(const QStringList& classNames)
{
    QString yaml = QStringLiteral(
        "path: .\n"
        "train: images\n"
        "val: images\n"
        "names:\n"
    );
    for (int index = 0; index < classNames.size(); ++index) {
        yaml += QStringLiteral("  %1: %2\n")
                    .arg(index)
                    .arg(yamlQuoted(classNames.at(index)));
    }
    return yaml.toUtf8();
}

} // namespace

Result<void> YoloExporter::exportDataset(
    const ProjectModel& project,
    const QString& outputDirectory
) const
{
    const QString requestedOutput = outputDirectory.trimmed();
    if (requestedOutput.isEmpty()) {
        return Result<void>::failure(QStringLiteral("请选择有效的导出目录"));
    }
    const QString normalizedOutput = QFileInfo(requestedOutput).absoluteFilePath();
    const QFileInfo outputInfo(normalizedOutput);
    if (outputInfo.fileName().isEmpty()) {
        return Result<void>::failure(QStringLiteral("不能将文件系统根目录作为导出目录"));
    }

    Result<ExportPlan> planResult = buildExportPlan(project);
    if (!planResult.isSuccess()) {
        return Result<void>::failure(planResult.error());
    }
    const ExportPlan plan = planResult.takeValue();

    if (outputInfo.exists() && !outputInfo.isDir()) {
        return Result<void>::failure(QStringLiteral("导出路径不是目录"));
    }

    QDir existingOutput(normalizedOutput);
    if (existingOutput.exists()) {
        if (!existingOutput.entryList(
                QDir::AllEntries | QDir::Hidden | QDir::System |
                QDir::NoDotAndDotDot
            ).isEmpty()) {
            return Result<void>::failure(QStringLiteral("导出目录必须为空"));
        }
    }

    const QString parentPath = outputInfo.absolutePath();
    if (!QDir().mkpath(parentPath)) {
        return Result<void>::failure(
            QStringLiteral("无法创建导出目录的父目录：%1").arg(parentPath)
        );
    }

    QTemporaryDir stagingDirectory(
        QDir(parentPath).filePath(QStringLiteral(".pixeltagger-yolo-XXXXXX"))
    );
    if (!stagingDirectory.isValid()) {
        return Result<void>::failure(QStringLiteral("无法创建 YOLO 导出暂存目录"));
    }

    QDir output(stagingDirectory.path());
    const QString imagesDirectory = output.filePath(QStringLiteral("images"));
    const QString labelsDirectory = output.filePath(QStringLiteral("labels"));
    if (!QDir().mkpath(imagesDirectory) || !QDir().mkpath(labelsDirectory)) {
        return Result<void>::failure(QStringLiteral("无法创建 YOLO 数据集目录"));
    }

    for (const ImageModel& image : project.images()) {
        const QFileInfo sourceInfo(image.filePath);
        const QString destinationImage = QDir(imagesDirectory).filePath(
            sourceInfo.fileName()
        );
        if (!QFile::copy(image.filePath, destinationImage)) {
            return Result<void>::failure(
                QStringLiteral("无法复制源图片：%1").arg(image.filePath)
            );
        }

        const QString labelPath = QDir(labelsDirectory).filePath(
            sourceInfo.completeBaseName() + QStringLiteral(".txt")
        );
        const Result<void> labelResult = writeFile(
            labelPath,
            labelFileData(image, plan.classIds)
        );
        if (!labelResult.isSuccess()) {
            return labelResult;
        }
    }

    const Result<void> classesResult = writeFile(
        output.filePath(QStringLiteral("classes.txt")),
        classesFileData(plan.classNames)
    );
    if (!classesResult.isSuccess()) {
        return classesResult;
    }

    const Result<void> yamlResult = writeFile(
        output.filePath(QStringLiteral("data.yaml")),
        dataYamlData(plan.classNames)
    );
    if (!yamlResult.isSuccess()) {
        return yamlResult;
    }

    const bool replacedExistingDirectory = existingOutput.exists();
    if (replacedExistingDirectory &&
        !QDir(parentPath).rmdir(outputInfo.fileName())) {
        return Result<void>::failure(QStringLiteral("无法替换空的导出目录"));
    }
    if (!QDir().rename(stagingDirectory.path(), normalizedOutput)) {
        if (replacedExistingDirectory) {
            QDir().mkpath(normalizedOutput);
        }
        return Result<void>::failure(
            QStringLiteral("无法完成 YOLO 数据集导出：%1").arg(normalizedOutput)
        );
    }
    stagingDirectory.setAutoRemove(false);
    return Result<void>::success();
}
