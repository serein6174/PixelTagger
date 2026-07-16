# 图像处理单项预览：并行开发交接说明

## 当前状态

图像处理的 Processor 和 ViewModel 已完成并通过自动测试，但尚未接入
Application 和 View。

当前实现 8 项操作：

1. 灰度化；
2. 二值化；
3. 均值滤波；
4. 高斯滤波；
5. 中值滤波；
6. Canny 边缘检测；
7. 亮度调整；
8. 对比度调整。

主要文件：

```text
src/processor/ImageConverter.h
src/processor/ImageConverter.cpp
src/processor/ImageProcessor.h
src/processor/ImageProcessor.cpp
src/viewmodel/ProcessViewModel.h
src/viewmodel/ProcessViewModel.cpp
```

自动测试：

```text
tests/processor/ImageProcessorTests.cpp
tests/viewmodel/ProcessViewModelTests.cpp
```

## 当前产品规则

- 每次处理都基于当前原图；
- 不将上一次预览作为下一次处理的输入；
- 原图始终保留；
- 处理结果默认只用于预览；
- 可以将预览结果另存为新图片；
- 另存后的图片不自动加入当前项目；
- 项目 JSON 不保存处理参数或预览图；
- 切换图片时应清除上一张图片的预览；
- 处理预览不改变图片尺寸；
- 预览期间继续叠加当前图片的标注框；
- 当前阶段不实现可叠加处理流水线。

---

# App 层负责人

## 负责范围

主要修改：

```text
src/app/Application.h
src/app/Application.cpp
```

不要修改：

```text
src/processor/
src/viewmodel/ProcessViewModel.*
```

## 对象所有权

Application 应创建并持有：

```cpp
ImageProcessor imageProcessor_;
ProcessViewModel processViewModel_;
```

成员顺序必须保证 `ImageProcessor` 先构造：

```cpp
ProjectModel projectModel_;
JsonProjectRepository projectRepository_;
ImageProcessor imageProcessor_;

ImageViewModel imageViewModel_;
AnnotationViewModel annotationViewModel_;
LabelViewModel labelViewModel_;
ProjectViewModel projectViewModel_;
ProcessViewModel processViewModel_;
MainWindow mainWindow_;
```

构造时注入：

```cpp
processViewModel_(imageProcessor_)
```

## 当前图片同步

`ImageViewModel` 发布 `CurrentImage` 后，应将原图交给
`ProcessViewModel`：

```cpp
QObject::connect(
    &imageViewModel_,
    &ImageViewModel::changed,
    &mainWindow_,
    [this](ViewModelChange change) {
        if (change != ViewModelChange::CurrentImage) {
            return;
        }

        processViewModel_.setSourceImage(
            imageViewModel_.currentQImage()
        );
    }
);
```

也可以放入已有的 `CurrentImage` 绑定 lambda，但只能做状态转交，不能
在 Application 中执行 OpenCV 算法。

`setSourceImage()` 会自动清除上一张图片的处理预览。

## View 请求到 ProcessViewModel 的绑定

推荐信号对应关系：

```text
grayscaleRequested()
  -> ProcessViewModel::previewGrayscale()

binaryRequested(threshold)
  -> ProcessViewModel::previewBinary(threshold)

meanBlurRequested(kernelSize)
  -> ProcessViewModel::previewMeanBlur(kernelSize)

gaussianBlurRequested(kernelSize)
  -> ProcessViewModel::previewGaussianBlur(kernelSize)

medianBlurRequested(kernelSize)
  -> ProcessViewModel::previewMedianBlur(kernelSize)

cannyRequested(lowThreshold, highThreshold)
  -> ProcessViewModel::previewCanny(lowThreshold, highThreshold)

brightnessRequested(brightness)
  -> ProcessViewModel::previewBrightness(brightness)

contrastRequested(contrast)
  -> ProcessViewModel::previewContrast(contrast)

processPreviewResetRequested()
  -> ProcessViewModel::resetPreview()

processPreviewSaveRequested(path)
  -> ProcessViewModel::savePreview(path)
```

## 预览图到 Canvas

连接 `previewChanged()`：

```cpp
QObject::connect(
    &processViewModel_,
    &ProcessViewModel::previewChanged,
    &mainWindow_,
    [this]() {
        mainWindow_.imageCanvas().setImage(
            processViewModel_.displayImage()
        );
    }
);
```

`displayImage()` 的语义是：

```text
存在预览 -> 返回预览图
没有预览 -> 返回原图
```

因此重置预览和切换图片都可以复用同一绑定。

Canvas 的标注集合不需要改变。处理结果与原图尺寸相同，现有原图坐标
标注可以继续叠加显示。

## 预览状态同步

如果 View 需要控制“重置”和“另存为”按钮：

```cpp
QObject::connect(
    &processViewModel_,
    &ProcessViewModel::previewAvailabilityChanged,
    &mainWindow_,
    &MainWindow::setProcessPreviewActionsEnabled
);
```

状态和错误继续使用既有通道：

```cpp
ProcessViewModel::statusChanged
  -> MainWindow::showStatus

ProcessViewModel::errorOccurred
  -> MainWindow::showError
```

## App 层禁止事项

Application 不允许：

- include OpenCV 头文件；
- 调用 `cv::cvtColor()`、`cv::threshold()` 等算法；
- 保存图像处理参数；
- 保存预览 `QImage`；
- 判断阈值或核大小是否合法；
- 决定算法执行顺序；
- 修改 `ProjectModel` 中的图片或标注；
- 在绑定 lambda 中实现图像处理。

## App 层验收

1. Application 持有唯一 `ImageProcessor` 和 `ProcessViewModel`。
2. 图片切换后旧预览被清除。
3. `previewChanged()` 只负责把 `displayImage()` 更新到 Canvas。
4. 预览期间已有标注仍正常绘制。
5. Application 中不存在 OpenCV 算法和参数校验。

---

# View 层负责人

## 负责范围

主要修改：

```text
src/view/MainWindow.h
src/view/MainWindow.cpp
```

推荐新增独立面板：

```text
src/view/process/ProcessPanel.h
src/view/process/ProcessPanel.cpp
```

不要修改：

```text
src/processor/
src/viewmodel/
src/model/
src/repository/
```

## 推荐 UI

建议使用“图像处理”Dock 或工具面板，包含算法选择、参数控件和操作按钮。

算法选项：

```text
灰度化
二值化
均值滤波
高斯滤波
中值滤波
Canny 边缘检测
亮度调整
对比度调整
```

按钮：

```text
应用预览
恢复原图
另存处理结果
```

## 参数范围

### 灰度化

无参数。

### 二值化

```text
threshold：0～255
默认值：128
```

### 均值、高斯、中值滤波

```text
kernelSize：3～15 的奇数
推荐选项：3、5、7、9、11、13、15
默认值：3
```

建议使用 `QComboBox`，避免用户输入偶数或越界值。

### Canny

```text
lowThreshold：0～254
默认值：50

highThreshold：1～255
默认值：150

必须满足：lowThreshold < highThreshold
```

View 可以通过控件范围减少无效输入，但最终合法性仍由 Processor 校验。

### 亮度

```text
brightness：-100～100
默认值：0
```

### 对比度

```text
contrast：0.1～3.0
默认值：1.0
步长建议：0.1
```

## 推荐 View 信号

```cpp
signals:
    void grayscaleRequested();
    void binaryRequested(int threshold);
    void meanBlurRequested(int kernelSize);
    void gaussianBlurRequested(int kernelSize);
    void medianBlurRequested(int kernelSize);
    void cannyRequested(int lowThreshold, int highThreshold);
    void brightnessRequested(int brightness);
    void contrastRequested(double contrast);
    void processPreviewResetRequested();
    void processPreviewSaveRequested(const QString& path);
```

View 只发送用户意图，不直接调用 `ProcessViewModel`。

## 预览交互

当前建议使用“参数变化后实时预览”。

但亮度、对比度和阈值滑块连续变化会产生大量信号，建议 View 使用
100～200ms 的单次 `QTimer` 防抖：

```text
滑块变化
  -> 重启单次计时器
  -> 用户停止调整约 150ms
  -> 发出处理请求
```

灰度化和滤波核选择可以立即发出请求。

如果第一版时间紧，也可以统一使用“应用预览”按钮，不影响
Processor/ViewModel 接口。

## 另存为

文件对话框属于 View：

```cpp
const QString path = QFileDialog::getSaveFileName(
    this,
    QStringLiteral("保存处理结果"),
    QStringLiteral("processed.png"),
    QStringLiteral("PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)")
);
```

路径非空时发送：

```cpp
emit processPreviewSaveRequested(path);
```

View 不直接调用 `QImage::save()`。

## View 可以做的事情

- 显示算法列表；
- 显示和读取参数控件；
- 根据当前算法切换参数区域；
- 对滑块输入做防抖；
- 打开保存文件对话框；
- 启用或禁用重置、另存为按钮；
- 显示 ProcessViewModel 返回的状态和错误。

## View 不能做的事情

- include `opencv2/...`；
- 执行 OpenCV 算法；
- include 或直接调用 `ProcessViewModel`；
- 保存真实预览图；
- 修改 `ProjectModel`；
- 覆盖原图文件；
- 把处理后的图片自动加入项目；
- 保存处理操作列表；
- 实现可叠加流水线。

## View 层验收

1. 八项操作都可以选择。
2. 参数控件范围与本文一致。
3. 处理后 Canvas 显示预览图。
4. 标注框仍叠加在正确位置。
5. 恢复原图后标注仍存在。
6. 切换图片后不会残留上一张的预览。
7. 没有预览时，“恢复原图”和“另存为”禁用。
8. 另存为不会覆盖项目原图或自动修改项目。
9. View 源码中没有 OpenCV、Model、ViewModel 或 Repository 依赖。

边界检查：

```powershell
rg '#include "(model|viewmodel|repository|app)/' src/view
rg '#include <opencv2/' src/view
```

预期没有输出。

---

# OpenCV 环境

README 已记录已验证版本：

```text
OpenCV 4.12.0
Triplet：x64-windows
所需组件：core、imgproc
```

安装最小功能版本：

```powershell
& "$env:VCPKG_ROOT\vcpkg.exe" install "opencv4[core]:x64-windows"
```

不建议直接安装默认 `opencv4`，因为默认功能会引入 DNN、DirectML、
FlatBuffers、Protobuf 等当前项目不使用的依赖。

---

# 自动测试

运行全部测试：

```powershell
cmake --preset default -DCMAKE_PREFIX_PATH=<QtPrefix>
cmake --build --preset default --config Debug
ctest --test-dir build/default -C Debug --output-on-failure
```

只运行图像处理测试：

```powershell
ctest --test-dir build/default -C Debug \
  -R "ImageProcessorTests|ProcessViewModelTests" \
  --output-on-failure
```

当前测试目标：

```text
ImageProcessorTests
ProcessViewModelTests
```

---

# 手工联调流程

1. 打开一张带标注的彩色图片。
2. 应用灰度化，确认图片变灰但标注仍存在。
3. 应用二值化并调整阈值。
4. 分别测试均值、高斯和中值滤波。
5. 测试 Canny，并确认阈值错误会显示提示。
6. 调整亮度到 `+50` 和 `-50`。
7. 调整对比度到 `0.5` 和 `2.0`。
8. 在任一预览状态下恢复原图。
9. 另存预览结果，确认生成新文件。
10. 切换下一张图片，确认旧预览清除。
11. 切回原图片，确认项目中的原图和标注没有被修改。

---

# 流水线边界

可叠加流水线在架构上可行，但当前版本明确不实现。

当前行为：

```text
原图 -> 单个操作 -> 预览
```

不是：

```text
原图 -> 亮度 -> 滤波 -> 灰度 -> Canny -> 预览
```

不要通过“把当前预览重新作为 source”临时实现叠加，否则会失去操作列表、
参数调整、步骤删除和稳定重算能力。

未来正确实现需要显式的 `ProcessOperation` 值对象和操作列表，并由
ProcessViewModel 每次从原图按顺序重算。该功能应作为独立提交，不要
混入当前 App/View 接入。

## 冲突提醒

- App 层负责人先完成对象所有权和信号绑定。
- View 层负责人不要修改 Processor 或 ProcessViewModel 来适配控件。
- 不要在 MainWindow 中保存预览 QImage。
- 不要让 ImageProcessor 依赖 Qt Widget 或 MainWindow。
- 不要修改 ProjectModel 来保存预览状态。
- 不要把 OpenCV 类型传到 View 层。
- 当前交接只负责单项预览，不顺便实现流水线。
