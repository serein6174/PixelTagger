#include "repository/JsonProjectRepository.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QSaveFile>

#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr int kProjectFormatVersion = 1;

bool isNumber(const QJsonObject& object, const QString& key)
{
    return object.contains(key) && object.value(key).isDouble();
}

bool isInteger(const QJsonObject& object, const QString& key)
{
    if (!isNumber(object, key)) {
        return false;
    }

    const double value = object.value(key).toDouble();
    return std::isfinite(value) && std::floor(value) == value &&
           value >= std::numeric_limits<int>::min() &&
           value <= std::numeric_limits<int>::max();
}

bool isString(const QJsonObject& object, const QString& key)
{
    return object.contains(key) && object.value(key).isString();
}

bool isValidRectObject(const QJsonObject& object)
{
    return isNumber(object, QStringLiteral("x")) &&
           isNumber(object, QStringLiteral("y")) &&
           isNumber(object, QStringLiteral("width")) &&
           isNumber(object, QStringLiteral("height"));
}

bool isValidLabelObject(const QJsonObject& object)
{
    return isInteger(object, QStringLiteral("id")) &&
           isString(object, QStringLiteral("name")) &&
           isString(object, QStringLiteral("color")) &&
           QColor(object.value(QStringLiteral("color")).toString()).isValid();
}

bool isValidAnnotationObject(const QJsonObject& object)
{
    return isInteger(object, QStringLiteral("id")) &&
           isInteger(object, QStringLiteral("label_id")) &&
           object.value(QStringLiteral("rect")).isObject() &&
           isValidRectObject(object.value(QStringLiteral("rect")).toObject());
}

bool isValidImageObject(const QJsonObject& object)
{
    if (!isInteger(object, QStringLiteral("id")) ||
        !isString(object, QStringLiteral("path")) ||
        !isString(object, QStringLiteral("file_name")) ||
        !isInteger(object, QStringLiteral("width")) ||
        !isInteger(object, QStringLiteral("height")) ||
        !object.value(QStringLiteral("annotations")).isArray()) {
        return false;
    }

    const QJsonArray annotations = object.value(QStringLiteral("annotations")).toArray();
    for (const QJsonValue& value : annotations) {
        if (!value.isObject() || !isValidAnnotationObject(value.toObject())) {
            return false;
        }
    }
    return true;
}

QJsonObject rectToJson(const QRectF& rect)
{
    QJsonObject object;
    object["x"] = rect.x();
    object["y"] = rect.y();
    object["width"] = rect.width();
    object["height"] = rect.height();
    return object;
}

QRectF rectFromJson(const QJsonObject& object)
{
    return QRectF(
        object["x"].toDouble(),
        object["y"].toDouble(),
        object["width"].toDouble(),
        object["height"].toDouble()
    );
}

QJsonObject labelToJson(const LabelModel& label)
{
    QJsonObject object;
    object["id"] = label.id;
    object["name"] = label.name;
    object["color"] = label.color.name(QColor::HexRgb);
    return object;
}

LabelModel labelFromJson(const QJsonObject& object)
{
    LabelModel label;
    label.id = object["id"].toInt();
    label.name = object["name"].toString(QStringLiteral("object"));
    label.color = QColor(object["color"].toString(QStringLiteral("#dc4646")));
    if (!label.color.isValid()) {
        label.color = QColor(220, 70, 70);
    }
    return label;
}

QJsonObject annotationToJson(const AnnotationModel& annotation)
{
    QJsonObject object;
    object["id"] = annotation.id;
    object["label_id"] = annotation.labelId;
    object["rect"] = rectToJson(annotation.imageRect);
    return object;
}

AnnotationModel annotationFromJson(const QJsonObject& object)
{
    AnnotationModel annotation;
    annotation.id = object["id"].toInt();
    annotation.labelId = object["label_id"].toInt();
    annotation.imageRect = rectFromJson(object["rect"].toObject());
    return annotation;
}

QJsonObject imageToJson(const ImageModel& image, const QString& projectDirectory)
{
    QJsonObject object;
    object["id"] = image.id;
    object["path"] = QDir(projectDirectory).relativeFilePath(image.filePath);
    object["file_name"] = image.fileName;
    object["width"] = image.width;
    object["height"] = image.height;

    QJsonArray annotations;
    for (const AnnotationModel& annotation : image.annotations) {
        annotations.push_back(annotationToJson(annotation));
    }
    object["annotations"] = annotations;
    return object;
}

ImageModel imageFromJson(const QJsonObject& object, const QString& projectDirectory)
{
    ImageModel image;
    image.id = object["id"].toInt();
    image.relativePath = object["path"].toString();
    image.filePath = QDir(projectDirectory).absoluteFilePath(image.relativePath);
    image.fileName = object["file_name"].toString();
    image.width = object["width"].toInt();
    image.height = object["height"].toInt();
    image.modified = false;

    const QJsonArray annotations = object["annotations"].toArray();
    image.annotations.reserve(annotations.size());
    for (const QJsonValue& value : annotations) {
        image.annotations.push_back(annotationFromJson(value.toObject()));
    }
    return image;
}

} // namespace

Result<void> JsonProjectRepository::save(
    const ProjectModel& project,
    const QString& path
) const
{
    const QString projectDirectory = QFileInfo(path).absolutePath();

    QJsonObject root;
    root["format"] = QStringLiteral("PixelTaggerProject");
    root["version"] = kProjectFormatVersion;
    root["current_image_index"] = project.currentImageIndex();

    QJsonArray labels;
    for (const LabelModel& label : project.labels()) {
        labels.push_back(labelToJson(label));
    }
    root["labels"] = labels;

    QJsonArray images;
    for (const ImageModel& image : project.images()) {
        images.push_back(imageToJson(image, projectDirectory));
    }
    root["images"] = images;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<void>::failure(
            QStringLiteral("无法写入项目文件：%1").arg(path)
        );
    }

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size() || !file.commit()) {
        return Result<void>::failure(
            QStringLiteral("项目文件写入失败：%1").arg(path)
        );
    }
    return Result<void>::success();
}

Result<ProjectModel> JsonProjectRepository::load(const QString& path) const
{
    const QString projectDirectory = QFileInfo(path).absolutePath();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<ProjectModel>::failure(
            QStringLiteral("无法打开项目文件：%1").arg(path)
        );
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return Result<ProjectModel>::failure(
            QStringLiteral("项目文件 JSON 格式错误：%1").arg(parseError.errorString())
        );
    }

    const QJsonObject root = document.object();
    if (root["format"].toString() != QStringLiteral("PixelTaggerProject")) {
        return Result<ProjectModel>::failure(QStringLiteral("不是有效的 PixelTagger 项目文件"));
    }
    if (!isInteger(root, QStringLiteral("version")) ||
        root.value(QStringLiteral("version")).toInt(-1) != kProjectFormatVersion) {
        return Result<ProjectModel>::failure(QStringLiteral("不支持的项目文件版本"));
    }
    if (!root.value(QStringLiteral("labels")).isArray() ||
        !root.value(QStringLiteral("images")).isArray() ||
        !isInteger(root, QStringLiteral("current_image_index"))) {
        return Result<ProjectModel>::failure(QStringLiteral("项目文件缺少必要字段"));
    }

    QVector<LabelModel> labels;
    const QJsonArray labelArray = root["labels"].toArray();
    labels.reserve(labelArray.size());
    for (const QJsonValue& value : labelArray) {
        if (!value.isObject() || !isValidLabelObject(value.toObject())) {
            return Result<ProjectModel>::failure(QStringLiteral("项目中的类别数据格式错误"));
        }
        labels.push_back(labelFromJson(value.toObject()));
    }

    QVector<ImageModel> images;
    const QJsonArray imageArray = root["images"].toArray();
    images.reserve(imageArray.size());
    for (const QJsonValue& value : imageArray) {
        if (!value.isObject() || !isValidImageObject(value.toObject())) {
            return Result<ProjectModel>::failure(QStringLiteral("项目中的图片或标注格式错误"));
        }
        images.push_back(imageFromJson(value.toObject(), projectDirectory));
    }

    ProjectModel project;
    const Result<void> replaceResult = project.replaceProjectData(
        std::move(images),
        std::move(labels),
        root["current_image_index"].toInt(0)
    );
    if (!replaceResult.isSuccess()) {
        return Result<ProjectModel>::failure(replaceResult.error());
    }
    return Result<ProjectModel>::success(std::move(project));
}
