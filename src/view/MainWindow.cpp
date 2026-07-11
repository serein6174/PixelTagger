#include "view/MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      canvas_(new ImageCanvas(this)),
      imageViewModel_(projectModel_),
      annotationViewModel_(projectModel_),
      labelViewModel_(projectModel_)
{
    setWindowTitle(QStringLiteral("AnnotaVision"));
    setCentralWidget(canvas_);
    createMenus();
    createToolBar();
    connectViewModel();
    statusBar()->showMessage(QStringLiteral("就绪"));
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
    QAction* exitAction = fileMenu->addAction(QStringLiteral("退出"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* navigateMenu = menuBar()->addMenu(QStringLiteral("浏览"));
    QAction* previousAction = navigateMenu->addAction(QStringLiteral("上一张"));
    previousAction->setShortcut(QKeySequence(QStringLiteral("Left")));
    connect(previousAction, &QAction::triggered, &imageViewModel_, &ImageViewModel::previousImage);

    QAction* nextAction = navigateMenu->addAction(QStringLiteral("下一张"));
    nextAction->setShortcut(QKeySequence(QStringLiteral("Right")));
    connect(nextAction, &QAction::triggered, &imageViewModel_, &ImageViewModel::nextImage);
}

void MainWindow::createToolBar()
{
    QToolBar* toolBar = addToolBar(QStringLiteral("标注"));
    toolBar->setMovable(false);
    toolBar->addWidget(new QLabel(QStringLiteral("当前类别："), this));

    labelComboBox_ = new QComboBox(this);
    labelComboBox_->setEditable(true);
    labelComboBox_->setMinimumWidth(180);
    labelComboBox_->addItem(labelViewModel_.currentLabelName());
    labelComboBox_->setCurrentText(labelViewModel_.currentLabelName());
    toolBar->addWidget(labelComboBox_);
}

void MainWindow::connectViewModel()
{
    connect(&imageViewModel_, &ImageViewModel::imageChanged,
            canvas_, &ImageCanvas::setImage);
    connect(&imageViewModel_, &ImageViewModel::currentImageChanged,
            &annotationViewModel_, &AnnotationViewModel::onCurrentImageChanged);
    connect(&imageViewModel_, &ImageViewModel::statusChanged,
            this, [this](const QString& message) { statusBar()->showMessage(message); });
    connect(&imageViewModel_, &ImageViewModel::errorOccurred,
            this, &MainWindow::showError);

    connect(canvas_, &ImageCanvas::annotationCreated,
            &annotationViewModel_, &AnnotationViewModel::createAnnotation);
    connect(&annotationViewModel_, &AnnotationViewModel::annotationsChanged,
            canvas_, &ImageCanvas::setAnnotations);
    connect(&annotationViewModel_, &AnnotationViewModel::errorOccurred,
            this, &MainWindow::showError);

    connect(labelComboBox_, &QComboBox::activated, this, [this]() {
        labelViewModel_.setCurrentLabelName(labelComboBox_->currentText());
    });
    connect(labelComboBox_->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        labelViewModel_.setCurrentLabelName(labelComboBox_->currentText());
    });
    connect(&labelViewModel_, &LabelViewModel::currentLabelNameChanged,
            this, [this](const QString& name) {
                QSignalBlocker blocker(labelComboBox_);
                labelComboBox_->setCurrentText(name);
            });
    connect(&labelViewModel_, &LabelViewModel::labelsChanged,
            &annotationViewModel_, &AnnotationViewModel::onLabelsChanged);
    connect(&labelViewModel_, &LabelViewModel::errorOccurred,
            this, &MainWindow::showError);
}

void MainWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开图片"), QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!path.isEmpty()) {
        imageViewModel_.loadImage(path);
    }
}

void MainWindow::openFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, QStringLiteral("打开图片文件夹"));
    if (!path.isEmpty()) {
        imageViewModel_.loadFolder(path);
    }
}

void MainWindow::showError(const QString& message)
{
    QMessageBox::warning(this, QStringLiteral("错误"), message);
}
