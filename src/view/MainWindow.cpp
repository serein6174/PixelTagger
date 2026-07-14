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
    : QMainWindow(parent), canvas_(new ImageCanvas(this))
{
    setWindowTitle(QStringLiteral("AnnotaVision"));
    setCentralWidget(canvas_);
    createMenus();
    createToolBar();
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
    labelComboBox_->addItem(QStringLiteral("object"));
    toolBar->addWidget(labelComboBox_);
}

void MainWindow::connectView()
{
    connect(labelComboBox_, &QComboBox::activated, this, [this]() {
        emit labelNameChangeRequested(labelComboBox_->currentText());
        canvas_->setFocus(Qt::OtherFocusReason);
    });
    connect(labelComboBox_->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        emit labelNameChangeRequested(labelComboBox_->currentText());
        canvas_->setFocus(Qt::OtherFocusReason);
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
