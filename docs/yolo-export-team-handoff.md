# YOLO 数据集导出：并行开发交接说明

## 当前状态

YOLO 导出的 Exporter、ProjectViewModel 接口、Application 对象所有权和
自动测试已经完成，但尚未在 View 中增加导出入口，也尚未在
`Application::bindView()` 中连接导出请求。

当前已有接口：

```cpp
YoloExporter::exportDataset(
    const ProjectModel& project,
    const QString& outputDirectory
)

ProjectViewModel::exportYolo(const QString& outputDirectory)
```

主要文件：

```text
src/exporter/YoloExporter.h
src/exporter/YoloExporter.cpp
src/viewmodel/ProjectViewModel.h
src/viewmodel/ProjectViewModel.cpp
```

自动测试：

```text
tests/exporter/YoloExporterTests.cpp
tests/viewmodel/ProjectExportViewModelTests.cpp
```

## 当前产品规则

导出目录结构固定为：

```text
dataset/
├── images/
├── labels/
├── classes.txt
└── data.yaml
```

导出规则：

- 复制项目中的原始源图片，不导出图像处理预览；
- 每张图片对应一个同名 `.txt` 标注文件；
- 没有标注的图片仍然复制，并生成空标注文件；
- 每行标注格式为：

```text
class_id x_center y_center width height
```

- 坐标以原图宽高归一化；
- 浮点数固定保留 6 位小数；
- YOLO `class_id` 按 `ProjectModel::labels()` 当前顺序从 `0` 连续生成；
- Model 中稳定的 `LabelId` 不会被修改；
- `classes.txt` 按相同类别顺序逐行写入类别名称；
- `data.yaml` 写入 `train: images`、`val: images` 和类别映射；
- 目标目录不存在或为空时允许导出；
- 目标目录非空时拒绝覆盖；
- 不同源目录中的同名图片会被拒绝；
- `same.jpg` 和 `same.png` 也会因生成同名 `same.txt` 而被拒绝；
- 源图缺失、尺寸非法、类别引用非法或坐标越界时，整次导出失败；
- 导出使用暂存目录，失败时不留下不完整数据集。

---

# App 层负责人

## 负责范围

只需要修改：

```text
src/app/Application.cpp
```

通常不需要修改：

```text
src/app/Application.h
src/exporter/
src/viewmodel/ProjectViewModel.*
src/model/
```

## 已完成的对象所有权

`Application` 已经持有：

```cpp
YoloExporter yoloExporter_;
```

并已注入：

```cpp
projectViewModel_(
    projectModel_,
    projectRepository_,
    yoloExporter_
)
```

不要重复创建 exporter，也不要在 MainWindow 或临时 lambda 中创建
`YoloExporter`。

## 需要增加的请求绑定

View 推荐发出：

```cpp
void exportYoloRequested(const QString& outputDirectory);
```

Application 只负责连接：

```cpp
QObject::connect(
    &mainWindow_,
    &MainWindow::exportYoloRequested,
    &projectViewModel_,
    &ProjectViewModel::exportYolo
);
```

`ProjectViewModel::statusChanged` 和 `errorOccurred` 已经连接到
`MainWindow::showStatus`、`MainWindow::showError`，不需要增加新的反馈
通道。

## App 层禁止事项

Application 不允许：

- 遍历图片或标注；
- 计算归一化坐标；
- 将 `LabelId` 转换成 `class_id`；
- 创建 `images` 或 `labels` 目录；
- 写入 `.txt`、`classes.txt` 或 `data.yaml`；
- 判断目标目录是否为空；
- 判断图片文件名是否冲突；
- 在绑定 lambda 中实现任何导出规则；
- 把图像处理预览传给 exporter。

Application 只负责对象装配和信号连接。

## App 层验收

1. `Application` 中只有一个 `YoloExporter` 实例。
2. MainWindow 的导出请求直接进入
   `ProjectViewModel::exportYolo()`。
3. Application 中没有 YOLO 格式和文件系统业务逻辑。
4. 导出成功和失败继续复用现有状态、错误显示通道。

---

# View 层负责人

## 负责范围

主要修改：

```text
src/view/MainWindow.h
src/view/MainWindow.cpp
```

如果希望增加独立对话框，可以新增：

```text
src/view/export/YoloExportDialog.h
src/view/export/YoloExportDialog.cpp
```

不要修改：

```text
src/exporter/
src/viewmodel/
src/model/
src/repository/
```

## 推荐 UI

最小实现：

```text
文件
└── 导出 YOLO 数据集...
```

也可以放在工具栏，但建议保留菜单入口。

用户点击后由 View 打开目录选择对话框。推荐让用户选择一个新建或空的
目录：

```cpp
const QString outputDirectory = QFileDialog::getExistingDirectory(
    this,
    QStringLiteral("选择 YOLO 数据集导出目录")
);

if (!outputDirectory.isEmpty()) {
    emit exportYoloRequested(outputDirectory);
}
```

系统原生目录对话框通常允许用户新建文件夹。Exporter 会再次检查目录，
因此 View 不需要自行判断目录是否为空。

## 推荐 View 信号

```cpp
signals:
    void exportYoloRequested(const QString& outputDirectory);
```

View 只发送用户选择的路径，不直接调用 `ProjectViewModel` 或
`YoloExporter`。

## View 可以做的事情

- 显示“导出 YOLO 数据集”菜单或按钮；
- 打开目录选择对话框；
- 用户取消选择时不发出请求；
- 把用户选择的路径作为信号参数发出；
- 显示 Application 已绑定的状态和错误消息。

## View 不能做的事情

- include 或直接调用 `ProjectViewModel`；
- include 或创建 `YoloExporter`；
- include `ProjectModel`、`ImageModel` 或 `AnnotationModel`；
- 读取项目图片和标注集合；
- 计算归一化坐标；
- 使用类别下拉框索引生成 `class_id`；
- 创建数据集目录或文件；
- 复制源图片；
- 导出当前 Canvas 截图或图像处理预览；
- 在导出前自行修改 Model 数据。

## View 层验收

1. 菜单或按钮可以打开目录选择对话框。
2. 用户取消时不会开始导出。
3. 选择新建或空目录后可以发出路径请求。
4. 选择非空目录时能显示 ViewModel 返回的错误。
5. View 中没有 YOLO 坐标、类别映射和文件写入逻辑。
6. View 源码中不存在 Model、ViewModel、Repository 或 Exporter 依赖。

边界检查：

```powershell
rg '#include "(model|viewmodel|repository|exporter|app)/' src/view
```

预期没有输出。

---

# 自动测试

运行全部测试：

```powershell
cmake --preset default
cmake --build --preset default --config Debug
ctest --test-dir build/default -C Debug --output-on-failure
```

只运行 YOLO 导出测试：

```powershell
ctest --test-dir build/default -C Debug `
  -R "YoloExporterTests|ProjectExportViewModelTests" `
  --output-on-failure
```

当前 YOLO 测试目标：

```text
YoloExporterTests
ProjectExportViewModelTests
```

当前完整测试结果：

```text
7/7 passed
```

---

# 手工联调流程

1. 导入两张不同文件名的图片。
2. 新增 `car` 和 `person` 两个类别。
3. 在第一张图创建一个 `car` 标注和一个 `person` 标注。
4. 保持第二张图没有标注。
5. 选择“导出 YOLO 数据集”。
6. 新建并选择一个空目录。
7. 确认生成 `images`、`labels`、`classes.txt` 和 `data.yaml`。
8. 确认两张原始图片都被复制到 `images`。
9. 确认第一张图片的标注文件包含两行。
10. 确认第二张图片对应的标注文件存在且为空。
11. 确认 `classes.txt` 中类别顺序与项目类别列表一致。
12. 再次选择同一个非空目录，确认导出被拒绝且原文件未被修改。
13. 临时移走一张源图片后再次导出，确认整次失败且没有半成品目录。

坐标抽查示例：

```text
原图：640 × 480
矩形：x=64, y=48, width=128, height=96

期望：
class_id 0.200000 0.200000 0.200000 0.200000
```

---

# 冲突提醒

- App 层负责人只补信号连接，不重写 exporter 所有权。
- View 层负责人不要修改 `ProjectViewModel` 来适配控件。
- 不要把 `LabelId` 直接写入 YOLO 文件。
- 不要使用类别下拉框索引替代 exporter 的类别映射。
- 不要让 View 预先创建 `images`、`labels` 等内容，否则 exporter 会把
  目标目录视为非空并拒绝导出。
- 不要导出 ProcessViewModel 的预览图；当前规则固定复制项目源图片。
- 不要在本次接入中加入覆盖确认、增量导出或 train/val 自动划分。
- 当前 `data.yaml` 的 `train` 和 `val` 都指向 `images`，数据集划分属于
  后续独立功能。
