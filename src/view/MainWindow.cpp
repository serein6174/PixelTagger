#include "view/MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      canvas_(new ImageCanvas(this)),
      imageViewModel_(new ImageViewModel(this))
{
    setWindowTitle(QStringLiteral("AnnotaVision"));
    setCentralWidget(canvas_);

    createMenus();
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
    connect(previousAction, &QAction::triggered, imageViewModel_, &ImageViewModel::previousImage);

    QAction* nextAction = navigateMenu->addAction(QStringLiteral("下一张"));
    nextAction->setShortcut(QKeySequence(QStringLiteral("Right")));
    connect(nextAction, &QAction::triggered, imageViewModel_, &ImageViewModel::nextImage);
}

void MainWindow::connectViewModel()
{
    connect(imageViewModel_, &ImageViewModel::imageChanged,
            canvas_, &ImageCanvas::setImage);
    connect(imageViewModel_, &ImageViewModel::statusChanged,
            this, [this](const QString& message) {
                statusBar()->showMessage(message);
            });
    connect(imageViewModel_, &ImageViewModel::errorOccurred,
            this, &MainWindow::showError);
}

void MainWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("打开图片"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif)")
    );

    if (path.isEmpty()) {
        return;
    }

    imageViewModel_->loadImage(path);
}

void MainWindow::openFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("打开图片文件夹")
    );

    if (path.isEmpty()) {
        return;
    }

    imageViewModel_->loadFolder(path);
}

void MainWindow::showError(const QString& message)
{
    QMessageBox::warning(this, QStringLiteral("错误"), message);
}
