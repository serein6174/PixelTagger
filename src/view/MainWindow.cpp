#include "view/MainWindow.h"

#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

namespace {

constexpr int kColorIconSize = 14;

enum ProcessOperation {
    GrayscaleOperation = 0,
    BinaryOperation,
    MeanBlurOperation,
    GaussianBlurOperation,
    MedianBlurOperation,
    CannyOperation,
    BrightnessOperation,
    ContrastOperation
};

QIcon makeColorIcon(const QColor& color)
{
    QPixmap pixmap(kColorIconSize, kColorIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(80, 80, 80));
    painter.setBrush(color.isValid() ? color : Qt::transparent);
    painter.drawRoundedRect(
        QRectF(1.0, 1.0, kColorIconSize - 2.0, kColorIconSize - 2.0),
        2.0,
        2.0
    );
    return QIcon(pixmap);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), canvas_(new ImageCanvas(this))
{
    setWindowTitle(QStringLiteral("AnnotaVision"));
    setCentralWidget(canvas_);
    createMenus();
    createToolBar();
    createProcessDock();
    connectView();
    canvas_->setFocus(Qt::OtherFocusReason);

    statusBar()->showMessage(QStringLiteral("就绪"));
}

ImageCanvas& MainWindow::imageCanvas() noexcept
{
    return *canvas_;
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("文件"));
    QAction* openImageAction = fileMenu->addAction(QStringLiteral("打开图片"));
    openImageAction->setShortcut(QKeySequence::Open);
    connect(openImageAction, &QAction::triggered, this, &MainWindow::openImage);

    QAction* openFolderAction = fileMenu->addAction(QStringLiteral("打开图片文件夹"));
    openFolderAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);

    fileMenu->addSeparator();

    QAction* openProjectAction = fileMenu->addAction(QStringLiteral("打开项目"));
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);

    QAction* saveProjectAction = fileMenu->addAction(QStringLiteral("保存项目"));
    saveProjectAction->setShortcut(QKeySequence::Save);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);

    fileMenu->addSeparator();

    QAction* exportYoloAction = fileMenu->addAction(QStringLiteral("导出 YOLO 数据集..."));
    connect(exportYoloAction, &QAction::triggered, this, &MainWindow::exportYolo);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction(QStringLiteral("退出"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* navigateMenu = menuBar()->addMenu(QStringLiteral("浏览"));
    QAction* previousAction = navigateMenu->addAction(QStringLiteral("上一张"));
    previousAction->setShortcuts({
        QKeySequence(QStringLiteral("PgUp")),
        QKeySequence(QStringLiteral("Ctrl+Left"))
    });
    previousAction->setShortcutContext(Qt::WindowShortcut);
    connect(previousAction, &QAction::triggered,
            this, &MainWindow::previousImageRequested);

    QAction* nextAction = navigateMenu->addAction(QStringLiteral("下一张"));
    nextAction->setShortcuts({
        QKeySequence(QStringLiteral("PgDown")),
        QKeySequence(QStringLiteral("Ctrl+Right"))
    });
    nextAction->setShortcutContext(Qt::WindowShortcut);
    connect(nextAction, &QAction::triggered,
            this, &MainWindow::nextImageRequested);
}

void MainWindow::createToolBar()
{
    QToolBar* toolBar = addToolBar(QStringLiteral("标注"));
    toolBar->setMovable(false);
    toolBar->addWidget(new QLabel(QStringLiteral("当前类别："), this));

    labelComboBox_ = new QComboBox(this);
    labelComboBox_->setEditable(true);
    labelComboBox_->setMinimumWidth(180);
    labelComboBox_->addItem(makeColorIcon(QColor(220, 70, 70)), QStringLiteral("object"), 0);
    toolBar->addWidget(labelComboBox_);

    QAction* addLabelAction = toolBar->addAction(
        style()->standardIcon(QStyle::SP_FileDialogNewFolder),
        QStringLiteral("新增类别")
    );
    connect(addLabelAction, &QAction::triggered, this, &MainWindow::addLabel);

    removeLabelAction_ = toolBar->addAction(
        style()->standardIcon(QStyle::SP_TrashIcon),
        QStringLiteral("删除类别")
    );
    connect(removeLabelAction_, &QAction::triggered, this, &MainWindow::removeCurrentLabel);

    changeLabelColorAction_ = toolBar->addAction(
        makeColorIcon(currentComboLabelColor()),
        QStringLiteral("修改颜色")
    );
    connect(
        changeLabelColorAction_,
        &QAction::triggered,
        this,
        &MainWindow::changeCurrentLabelColor
    );

    toolBar->addSeparator();
    applyLabelToAnnotationAction_ = toolBar->addAction(
        style()->standardIcon(QStyle::SP_DialogApplyButton),
        QStringLiteral("应用当前类别到选中标注")
    );
    applyLabelToAnnotationAction_->setEnabled(false);
    connect(
        applyLabelToAnnotationAction_,
        &QAction::triggered,
        this,
        &MainWindow::applyCurrentLabelToSelectedAnnotation
    );

    deleteSelectedAnnotationAction_ = toolBar->addAction(
        style()->standardIcon(QStyle::SP_TrashIcon),
        QStringLiteral("删除选中标注")
    );
    deleteSelectedAnnotationAction_->setObjectName(
        QStringLiteral("deleteSelectedAnnotationAction")
    );
    deleteSelectedAnnotationAction_->setShortcut(QKeySequence::Delete);
    deleteSelectedAnnotationAction_->setShortcutContext(Qt::WindowShortcut);
    deleteSelectedAnnotationAction_->setEnabled(false);
    connect(
        deleteSelectedAnnotationAction_,
        &QAction::triggered,
        this,
        &MainWindow::deleteSelectedAnnotation
    );
}

void MainWindow::createProcessDock()
{
    QDockWidget* dock = new QDockWidget(QStringLiteral("图像处理"), this);
    dock->setObjectName(QStringLiteral("ProcessDock"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* panel = new QWidget(dock);
    QVBoxLayout* layout = new QVBoxLayout(panel);

    processComboBox_ = new QComboBox(panel);
    processComboBox_->addItem(QStringLiteral("灰度化"));
    processComboBox_->addItem(QStringLiteral("二值化"));
    processComboBox_->addItem(QStringLiteral("均值滤波"));
    processComboBox_->addItem(QStringLiteral("高斯滤波"));
    processComboBox_->addItem(QStringLiteral("中值滤波"));
    processComboBox_->addItem(QStringLiteral("Canny 边缘检测"));
    processComboBox_->addItem(QStringLiteral("亮度调整"));
    processComboBox_->addItem(QStringLiteral("对比度调整"));
    layout->addWidget(processComboBox_);

    processParameterStack_ = new QStackedWidget(panel);
    processParameterStack_->addWidget(new QWidget(processParameterStack_));

    QWidget* binaryPage = new QWidget(processParameterStack_);
    QFormLayout* binaryLayout = new QFormLayout(binaryPage);
    binaryThresholdSpinBox_ = new QSpinBox(binaryPage);
    binaryThresholdSpinBox_->setRange(0, 255);
    binaryThresholdSpinBox_->setValue(128);
    binaryLayout->addRow(QStringLiteral("阈值"), binaryThresholdSpinBox_);
    processParameterStack_->addWidget(binaryPage);

    auto createKernelPage = [this](QComboBox** comboBox) {
        QWidget* page = new QWidget(processParameterStack_);
        QFormLayout* form = new QFormLayout(page);
        *comboBox = new QComboBox(page);
        for (int kernelSize = 3; kernelSize <= 15; kernelSize += 2) {
            (*comboBox)->addItem(QString::number(kernelSize), kernelSize);
        }
        form->addRow(QStringLiteral("核大小"), *comboBox);
        return page;
    };
    processParameterStack_->addWidget(createKernelPage(&meanKernelComboBox_));
    processParameterStack_->addWidget(createKernelPage(&gaussianKernelComboBox_));
    processParameterStack_->addWidget(createKernelPage(&medianKernelComboBox_));

    QWidget* cannyPage = new QWidget(processParameterStack_);
    QFormLayout* cannyLayout = new QFormLayout(cannyPage);
    cannyLowSpinBox_ = new QSpinBox(cannyPage);
    cannyLowSpinBox_->setRange(0, 254);
    cannyLowSpinBox_->setValue(50);
    cannyHighSpinBox_ = new QSpinBox(cannyPage);
    cannyHighSpinBox_->setRange(1, 255);
    cannyHighSpinBox_->setValue(150);
    cannyLayout->addRow(QStringLiteral("低阈值"), cannyLowSpinBox_);
    cannyLayout->addRow(QStringLiteral("高阈值"), cannyHighSpinBox_);
    processParameterStack_->addWidget(cannyPage);

    QWidget* brightnessPage = new QWidget(processParameterStack_);
    QFormLayout* brightnessLayout = new QFormLayout(brightnessPage);
    brightnessSpinBox_ = new QSpinBox(brightnessPage);
    brightnessSpinBox_->setRange(-100, 100);
    brightnessSpinBox_->setValue(0);
    brightnessLayout->addRow(QStringLiteral("亮度"), brightnessSpinBox_);
    processParameterStack_->addWidget(brightnessPage);

    QWidget* contrastPage = new QWidget(processParameterStack_);
    QFormLayout* contrastLayout = new QFormLayout(contrastPage);
    contrastSpinBox_ = new QDoubleSpinBox(contrastPage);
    contrastSpinBox_->setRange(0.1, 3.0);
    contrastSpinBox_->setDecimals(1);
    contrastSpinBox_->setSingleStep(0.1);
    contrastSpinBox_->setValue(1.0);
    contrastLayout->addRow(QStringLiteral("对比度"), contrastSpinBox_);
    processParameterStack_->addWidget(contrastPage);

    layout->addWidget(processParameterStack_);

    QPushButton* applyButton = new QPushButton(QStringLiteral("应用预览"), panel);
    connect(applyButton, &QPushButton::clicked, this, &MainWindow::applyProcessPreview);
    layout->addWidget(applyButton);

    resetProcessPreviewAction_ = new QAction(QStringLiteral("恢复原图"), this);
    resetProcessPreviewAction_->setEnabled(false);
    connect(resetProcessPreviewAction_, &QAction::triggered,
            this, &MainWindow::processPreviewResetRequested);
    QToolButton* resetButton = new QToolButton(panel);
    resetButton->setDefaultAction(resetProcessPreviewAction_);
    resetButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    layout->addWidget(resetButton);

    saveProcessPreviewAction_ = new QAction(QStringLiteral("另存处理结果"), this);
    saveProcessPreviewAction_->setEnabled(false);
    connect(saveProcessPreviewAction_, &QAction::triggered,
            this, &MainWindow::saveProcessPreview);
    QToolButton* saveButton = new QToolButton(panel);
    saveButton->setDefaultAction(saveProcessPreviewAction_);
    saveButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    layout->addWidget(saveButton);

    layout->addStretch(1);
    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::connectView()
{
    connect(labelComboBox_, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        const LabelId labelId = labelComboBox_->itemData(index).toInt();
        emit currentLabelChangeRequested(labelId);
        canvas_->setFocus(Qt::OtherFocusReason);
    });
    connect(labelComboBox_->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        const QString name = labelComboBox_->currentText().trimmed();
        emit renameLabelRequested(currentComboLabelId(), name);
        if (!name.isEmpty()) {
            emit labelNameChangeRequested(name);
        }
        canvas_->setFocus(Qt::OtherFocusReason);
    });

    connect(processComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            processParameterStack_, &QStackedWidget::setCurrentIndex);
    connect(cannyLowSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        cannyHighSpinBox_->setMinimum(value + 1);
    });
    connect(cannyHighSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        cannyLowSpinBox_->setMaximum(value - 1);
    });
}

void MainWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("打开图片"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif)")
    );
    if (!path.isEmpty()) {
        emit importImageRequested(path);
    }
}

void MainWindow::openFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("打开图片文件夹")
    );
    if (!path.isEmpty()) {
        emit importFolderRequested(path);
    }
}

void MainWindow::openProject()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("打开项目"),
        QString(),
        QStringLiteral("PixelTagger Project (*.json)")
    );
    if (!path.isEmpty()) {
        emit openProjectRequested(path);
    }
}

void MainWindow::saveProject()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存项目"),
        QStringLiteral("project.json"),
        QStringLiteral("PixelTagger Project (*.json)")
    );
    if (!path.isEmpty()) {
        emit saveProjectRequested(path);
    }
}

void MainWindow::exportYolo()
{
    const QString outputDirectory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择 YOLO 数据集导出目录")
    );
    if (!outputDirectory.isEmpty()) {
        emit exportYoloRequested(outputDirectory);
    }
}

void MainWindow::showStatus(const QString& message)
{
    statusBar()->showMessage(message);
}

void MainWindow::showError(const QString& message)
{
    QMessageBox::warning(this, QStringLiteral("错误"), message);
}

void MainWindow::setCurrentLabelName(const QString& name)
{
    QSignalBlocker blocker(labelComboBox_);
    labelComboBox_->setCurrentText(name);
}

void MainWindow::setLabels(const QVector<LabelViewData>& labels)
{
    const QSignalBlocker blocker(labelComboBox_);

    labelComboBox_->clear();
    int currentIndex = -1;

    for (int index = 0; index < labels.size(); ++index) {
        const LabelViewData& label = labels.at(index);
        labelComboBox_->addItem(makeColorIcon(label.color), label.name, label.id);
        labelComboBox_->setItemData(index, label.color, Qt::UserRole + 1);
        if (label.inUse) {
            labelComboBox_->setItemData(index, QStringLiteral("该类别已被标注使用"), Qt::ToolTipRole);
        }
        if (label.current) {
            currentIndex = index;
        }
    }

    if (currentIndex >= 0) {
        labelComboBox_->setCurrentIndex(currentIndex);
    } else if (labelComboBox_->count() > 0) {
        labelComboBox_->setCurrentIndex(0);
    }

    const bool hasLabel = labelComboBox_->count() > 0;
    if (removeLabelAction_) {
        removeLabelAction_->setEnabled(hasLabel);
    }
    if (changeLabelColorAction_) {
        changeLabelColorAction_->setEnabled(hasLabel);
        changeLabelColorAction_->setIcon(makeColorIcon(currentComboLabelColor()));
    }
    if (applyLabelToAnnotationAction_) {
        applyLabelToAnnotationAction_->setEnabled(annotationEditEnabled_ && hasLabel);
    }
}

void MainWindow::setAnnotationEditEnabled(bool enabled)
{
    annotationEditEnabled_ = enabled;
    if (applyLabelToAnnotationAction_) {
        applyLabelToAnnotationAction_->setEnabled(enabled && labelComboBox_ &&
                                                  labelComboBox_->count() > 0);
    }
    if (deleteSelectedAnnotationAction_) {
        deleteSelectedAnnotationAction_->setEnabled(enabled);
    }
}

void MainWindow::setProcessPreviewActionsEnabled(bool enabled)
{
    if (resetProcessPreviewAction_) {
        resetProcessPreviewAction_->setEnabled(enabled);
    }
    if (saveProcessPreviewAction_) {
        saveProcessPreviewAction_->setEnabled(enabled);
    }
}

void MainWindow::addLabel()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this,
        QStringLiteral("新增类别"),
        QStringLiteral("类别名称："),
        QLineEdit::Normal,
        QString(),
        &accepted
    ).trimmed();
    if (!accepted || name.isEmpty()) {
        canvas_->setFocus(Qt::OtherFocusReason);
        return;
    }

    const QColor color = QColorDialog::getColor(
        QColor(40, 120, 220),
        this,
        QStringLiteral("选择类别颜色")
    );
    if (color.isValid()) {
        emit addLabelRequested(name, color);
    }
    canvas_->setFocus(Qt::OtherFocusReason);
}

void MainWindow::removeCurrentLabel()
{
    emit removeLabelRequested(currentComboLabelId());
    canvas_->setFocus(Qt::OtherFocusReason);
}

void MainWindow::changeCurrentLabelColor()
{
    const QColor color = QColorDialog::getColor(
        currentComboLabelColor(),
        this,
        QStringLiteral("选择类别颜色")
    );
    if (color.isValid()) {
        emit labelColorChangeRequested(currentComboLabelId(), color);
    }
    canvas_->setFocus(Qt::OtherFocusReason);
}

void MainWindow::applyCurrentLabelToSelectedAnnotation()
{
    emit selectedAnnotationLabelChangeRequested(currentComboLabelId());
    canvas_->setFocus(Qt::OtherFocusReason);
}

void MainWindow::deleteSelectedAnnotation()
{
    emit deleteSelectedAnnotationRequested();
    canvas_->setFocus(Qt::OtherFocusReason);
}

void MainWindow::applyProcessPreview()
{
    switch (processComboBox_->currentIndex()) {
    case GrayscaleOperation:
        emit grayscaleRequested();
        break;
    case BinaryOperation:
        emit binaryRequested(binaryThresholdSpinBox_->value());
        break;
    case MeanBlurOperation:
        emit meanBlurRequested(currentKernelSize(meanKernelComboBox_));
        break;
    case GaussianBlurOperation:
        emit gaussianBlurRequested(currentKernelSize(gaussianKernelComboBox_));
        break;
    case MedianBlurOperation:
        emit medianBlurRequested(currentKernelSize(medianKernelComboBox_));
        break;
    case CannyOperation:
        emit cannyRequested(cannyLowSpinBox_->value(), cannyHighSpinBox_->value());
        break;
    case BrightnessOperation:
        emit brightnessRequested(brightnessSpinBox_->value());
        break;
    case ContrastOperation:
        emit contrastRequested(contrastSpinBox_->value());
        break;
    default:
        break;
    }
    canvas_->setFocus(Qt::OtherFocusReason);
}

void MainWindow::saveProcessPreview()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存处理结果"),
        QStringLiteral("processed.png"),
        QStringLiteral("PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)")
    );
    if (!path.isEmpty()) {
        emit processPreviewSaveRequested(path);
    }
    canvas_->setFocus(Qt::OtherFocusReason);
}

LabelId MainWindow::currentComboLabelId() const noexcept
{
    return labelComboBox_ ? labelComboBox_->currentData().toInt() : -1;
}

QColor MainWindow::currentComboLabelColor() const
{
    if (!labelComboBox_) {
        return QColor(220, 70, 70);
    }

    const QVariant value = labelComboBox_->currentData(Qt::UserRole + 1);
    if (value.canConvert<QColor>()) {
        const QColor color = value.value<QColor>();
        if (color.isValid()) {
            return color;
        }
    }
    return QColor(220, 70, 70);
}

int MainWindow::currentKernelSize(QComboBox* comboBox) const noexcept
{
    return comboBox ? comboBox->currentData().toInt() : 3;
}
