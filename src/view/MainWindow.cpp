#include "view/MainWindow.h"

#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVariant>

namespace {

constexpr int kColorIconSize = 14;

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
}

void MainWindow::connectView()
{
    connect(labelComboBox_, &QComboBox::activated, this, [this](int index) {
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
