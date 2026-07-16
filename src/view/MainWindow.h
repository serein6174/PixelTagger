#pragma once

#include <QMainWindow>
#include <QVector>

#include "common/presentation/LabelViewData.h"
#include "common/types/EntityIds.h"
#include "view/canvas/ImageCanvas.h"

class QAction;
class QComboBox;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    ImageCanvas& imageCanvas() noexcept;

signals:
    void importImageRequested(const QString& path);
    void importFolderRequested(const QString& path);
    void openProjectRequested(const QString& path);
    void saveProjectRequested(const QString& path);
    void previousImageRequested();
    void nextImageRequested();
    void labelNameChangeRequested(const QString& name);
    void addLabelRequested(const QString& name, const QColor& color);
    void removeLabelRequested(LabelId labelId);
    void renameLabelRequested(LabelId labelId, const QString& name);
    void labelColorChangeRequested(LabelId labelId, const QColor& color);
    void currentLabelChangeRequested(LabelId labelId);

public slots:
    void showStatus(const QString& message);
    void showError(const QString& message);
    void setCurrentLabelName(const QString& name);
    void setLabels(const QVector<LabelViewData>& labels);

private slots:
    void openImage();
    void openFolder();
    void openProject();
    void saveProject();
    void addLabel();
    void removeCurrentLabel();
    void changeCurrentLabelColor();

private:
    void createMenus();
    void createToolBar();
    void connectView();
    LabelId currentComboLabelId() const noexcept;
    QColor currentComboLabelColor() const;

    ImageCanvas* canvas_ = nullptr;
    QComboBox* labelComboBox_ = nullptr;
    QAction* removeLabelAction_ = nullptr;
    QAction* changeLabelColorAction_ = nullptr;
};
