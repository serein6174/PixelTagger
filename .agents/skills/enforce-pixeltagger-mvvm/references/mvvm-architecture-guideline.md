# PixelTagger MVVM 架构规划（按课程 PPT 简化版）

> 技术栈：C++17、Qt Widgets、OpenCV、CMake、vcpkg。  
> 架构目标：尽量贴合课程 PPT 中的 MVVM 思路，不引入 FunctionModel，不设置独立 Service 层，不提前加入 Command 层。

---

# 1. 总体结构

项目采用：

```text
App
├── Common
├── Model
├── ViewModel
├── View
├── Repository
├── Processor
└── Exporter
```

总体数据流：

```text
View
  ↓ 用户操作
ViewModel
  ↓ 修改状态或调用功能模块
Model / Repository / Processor / Exporter
  ↓ 返回结果
ViewModel
  ↓ 属性、展示数据、信号
View
```

核心依赖方向：

```text
App          -> 所有层
View         -> Common
ViewModel    -> Common + Model + Repository + Processor + Exporter
Repository   -> Common + Model
Processor    -> Common
Exporter     -> Common + Model
Model        -> Common
Common       -> 不依赖其他业务层
```

禁止依赖：

```text
View       -X-> Model
View       -X-> Repository / Processor / Exporter
Model      -X-> View / ViewModel
Repository -X-> View
Processor  -X-> View
Exporter   -X-> View
Common     -X-> App / View / ViewModel / Model
```

---

# 2. 推荐目录结构

```text
src/
├── app/
│   ├── Application.h
│   └── Application.cpp
│
├── common/
│   ├── EntityIds.h
│   ├── Result.h
│   ├── ImageViewData.h
│   ├── AnnotationRenderItem.h
│   ├── LabelPresentationData.h
│   └── ProjectViewData.h
│
├── model/
│   ├── ProjectModel.h
│   ├── ProjectModel.cpp
│   ├── ImageModel.h
│   ├── ImageModel.cpp
│   ├── AnnotationModel.h
│   ├── AnnotationModel.cpp
│   ├── LabelModel.h
│   └── LabelModel.cpp
│
├── viewmodel/
│   ├── ProjectViewModel.h
│   ├── ProjectViewModel.cpp
│   ├── ImageViewModel.h
│   ├── ImageViewModel.cpp
│   ├── AnnotationViewModel.h
│   ├── AnnotationViewModel.cpp
│   ├── LabelViewModel.h
│   ├── LabelViewModel.cpp
│   ├── ProcessViewModel.h
│   └── ProcessViewModel.cpp
│
├── view/
│   ├── MainWindow.h
│   ├── MainWindow.cpp
│   ├── canvas/
│   │   ├── ImageCanvas.h
│   │   ├── ImageCanvas.cpp
│   │   ├── CoordinateMapper.h
│   │   └── CoordinateMapper.cpp
│   ├── label/
│   │   ├── LabelPanel.h
│   │   └── LabelPanel.cpp
│   ├── property/
│   │   ├── PropertyPanel.h
│   │   └── PropertyPanel.cpp
│   └── process/
│       ├── ProcessPanel.h
│       └── ProcessPanel.cpp
│
├── repository/
│   ├── IProjectRepository.h
│   ├── JsonProjectRepository.h
│   └── JsonProjectRepository.cpp
│
├── processor/
│   ├── ImageProcessor.h
│   └── ImageProcessor.cpp
│
├── exporter/
│   ├── YoloExporter.h
│   └── YoloExporter.cpp
│
└── main.cpp
```

说明：

- 不设置 `functionmodel/`。
- 不设置独立 `service/`。
- 不设置泛化 `utils/`。
- 不提前加入 `command/`。
- `CoordinateMapper` 属于 View 层。
- JSON 持久化属于 Repository。
- OpenCV 算法属于 Processor。
- YOLO 输出属于 Exporter。

---

# 3. App 层

## 必须负责

App 层是对象装配层，负责：

- 创建唯一 `ProjectModel`；
- 创建 Repository；
- 创建 Processor；
- 创建 Exporter；
- 创建所有 ViewModel；
- 创建 MainWindow；
- 管理对象生命周期；
- 建立 View 与 ViewModel 的信号槽绑定；
- 启动和关闭应用。

推荐所有权：

```text
Application
├── owns ProjectModel
├── owns JsonProjectRepository
├── owns ImageProcessor
├── owns YoloExporter
├── owns ProjectViewModel
├── owns ImageViewModel
├── owns AnnotationViewModel
├── owns LabelViewModel
├── owns ProcessViewModel
└── owns MainWindow
```

## 绝对不能做

- 直接修改 `ProjectModel`；
- 新增、删除、修改标注；
- 实现类别规则；
- 读写 JSON；
- 调用 OpenCV；
- 计算 YOLO；
- 保存当前选中标注；
- 保存当前类别；
- 保存画布缩放和平移；
- 成为业务控制中心。

App 只能做对象创建、生命周期和绑定。

---

# 4. Common 层

## 必须负责

Common 只放多个层共同使用的稳定契约。

建议包括：

```text
EntityIds.h
Result.h
AnnotationRenderItem.h
LabelPresentationData.h
```

例如：

```cpp
using ImageId = std::uint64_t;
using AnnotationId = std::uint64_t;
using LabelId = std::uint32_t;
```

```cpp
struct AnnotationRenderItem {
    AnnotationId id;
    QRectF imageRect;
    QString labelName;
    QColor color;
    bool selected;
};
```

## 不放 `ICommandBase`

当前项目使用 Qt signal-slot 完成 View 到 ViewModel 的命令绑定，没有必要提前加入 `ICommandBase`。

撤销/重做以后若实现，应单独建立：

```text
command/
```

而不是现在放入 Common。

## 绝对不能做

- 持有 `ProjectModel`；
- 创建 ViewModel；
- 保存当前图片或当前类别；
- 实现业务逻辑；
- 执行 JSON、OpenCV 或 YOLO；
- 存放 `CoordinateMapper`；
- 作为杂项目录；
- 依赖其他业务层。

判断标准：

> 只有真正被两个或多个层共同依赖的稳定类型，才可以放入 Common。

---

# 5. Model 层

## 总体职责

Model 保存系统真实业务数据和领域关系。

推荐结构：

```text
ProjectModel
├── ImageModel[]
│   └── AnnotationModel[]
└── LabelModel[]
```

`ProjectModel` 是唯一聚合根和唯一真实业务数据源。

## 5.1 ProjectModel

### 必须负责

- 项目名称；
- 项目文件路径；
- 图片根目录；
- 图片集合；
- 类别集合；
- 当前图片 ID；
- dirty 状态；
- 唯一 ID 管理；
- 按 ID 查询；
- 项目级数据一致性；
- 图片、类别、标注关系合法性。

### 绝对不能做

- 打开文件对话框；
- 绘制图像；
- 保存 JSON；
- 导出 YOLO；
- 调用 OpenCV；
- 弹出消息框；
- 保存画布缩放、平移和鼠标状态；
- 保存 Widget 坐标；
- 直接通知 Canvas。

## 5.2 ImageModel

### 必须负责

- 图片 ID；
- 文件路径；
- 文件名；
- 原始宽高；
- 该图片拥有的标注集合。

### 绝对不能做

- 保存 `QPixmap` 绘制缓存；
- 保存 Canvas 缩放和平移；
- 保存拖拽状态；
- 用路径替代稳定 ID；
- 操作 View。

## 5.3 AnnotationModel

### 必须负责

- 标注 ID；
- `LabelId`；
- 原始图像坐标矩形。

```cpp
class AnnotationModel {
private:
    AnnotationId id_;
    LabelId labelId_;
    QRectF imageRect_;
};
```

### 绝对不能做

- 同时保存 `labelId` 和 `labelName`；
- 保存颜色；
- 保存选中状态；
- 保存悬停状态；
- 保存 Widget 坐标；
- 持有 Canvas。

## 5.4 LabelModel

### 必须负责

- 类别 ID；
- 类别名称；
- 类别颜色；
- 稳定类别身份。

### 绝对不能做

- 保存“当前是否选中”；
- 持有 LabelPanel；
- 绘制标注框；
- 处理用户输入；
- 保存标注集合。

---

# 6. ViewModel 层

## 总体职责

ViewModel 负责：

- 向 View 提供属性；
- 暴露业务操作；
- 保存界面语义状态；
- 修改 Model；
- 调用 Repository、Processor、Exporter；
- 将 Model 转换为 ViewData；
- 发出 View 更新通知；
- 将底层错误转换成 UI 可理解信息。

可以保存：

```text
当前选中标注 ID
当前类别 ID
当前工具模式
是否可以保存
是否可以上一张/下一张
窗口标题
处理参数
是否正在处理
```

不能保存：

```text
鼠标按下坐标
拖拽预览框
缩放倍率
平移偏移
悬停控制点
QPushButton*
ImageCanvas*
MainWindow*
```

接口命名应体现业务语义：

```cpp
createAnnotation();
deleteSelectedAnnotation();
renameLabel();
importImages();
saveProject();
exportYolo();
```

## 6.1 ProjectViewModel

### 必须负责

- 新建项目；
- 打开项目；
- 保存项目；
- 另存为；
- 导出 YOLO；
- 项目标题；
- dirty 状态；
- 保存和导出结果通知；
- 调用 Repository 和 Exporter。

### 绝对不能做

- 自己实现 JSON；
- 自己计算 YOLO；
- 打开文件对话框；
- 访问 MainWindow；
- 绘制；
- 保存 Canvas 状态。

## 6.2 ImageViewModel

### 必须负责

- 导入单张图片；
- 导入图片目录；
- 当前图片；
- 上一张、下一张；
- 图片名称、尺寸和序号；
- 是否可切换；
- 读取图片基础信息；
- 修改 ProjectModel；
- 发布图片展示数据。

由于当前不设置 Service 层，图片导入逻辑直接放在 `ImageViewModel`。

允许包含：

- 文件扩展名过滤；
- 目录遍历；
- 图片有效性检查；
- 读取图片宽高；
- 重复路径检查；
- 将图片加入 ProjectModel。

如果以后图片导入逻辑明显膨胀，再抽 `ImageLoader`，但当前不单独设 Service 层。

### 绝对不能做

- 保存 Canvas 缩放和平移；
- 处理鼠标拖拽；
- 修改标注和类别；
- 执行 OpenCV 处理；
- 访问 ImageCanvas。

## 6.3 AnnotationViewModel

### 必须负责

- 创建标注；
- 删除标注；
- 更新矩形；
- 修改标注类别；
- 管理当前选中标注；
- 校验当前图片和当前类别；
- 生成 `AnnotationRenderItem`；
- 发布标注变化；
- 图片切换时重置选择。

当前规模下，直接调用 `ProjectModel` 的受控接口。

### 绝对不能做

- Widget/Image 坐标转换；
- 绘制；
- 保存拖拽预览；
- 命中测试；
- 弹出输入框；
- 操作 Canvas；
- 保存第二份真实标注数据。

## 6.4 LabelViewModel

### 必须负责

- 新增类别；
- 删除类别；
- 重命名类别；
- 设置当前类别；
- 修改颜色；
- 当前类别 ID；
- 生成类别展示数据；
- 发布类别变化；
- 删除类别时的业务校验。

### 绝对不能做

- 持有 LabelPanel；
- 调用 QComboBox；
- 绘制颜色块；
- 保存第二份真实 LabelModel 数据。

## 6.5 ProcessViewModel

### 必须负责

- 处理参数；
- 调用 ImageProcessor；
- 重置预览；
- 提供处理后的展示图像；
- 发布处理结果；
- 发布错误。

### 绝对不能做

- 实现 OpenCV 算法；
- 修改 Canvas；
- 打开保存文件对话框；
- 默认覆盖原图；
- 保存画布状态。

推荐策略：

```text
原图始终保留
处理结果仅用于预览
明确保存后才写出新文件
```

---

# 7. View 层

## 总体职责

View 只负责：

- 控件与布局；
- 用户输入；
- 绘制；
- 显示 ViewModel 输出；
- 发出用户意图；
- 响应 ViewModel 信号。

## 7.1 MainWindow

### 必须负责

- 主窗口布局；
- 菜单栏、工具栏、状态栏；
- 组合子 View；
- 打开 `QFileDialog`；
- 打开 `QColorDialog`；
- 显示确认框；
- 显示错误信息；
- 发出用户请求信号。

### 绝对不能做

- 持有 `ProjectModel`；
- 修改标注；
- 修改类别；
- 保存 JSON；
- 导出 YOLO；
- 调用 OpenCV；
- 成为业务转发中心；
- 保存 ViewModel 业务状态。

## 7.2 ImageCanvas

### 必须负责

- 显示图片；
- 绘制 `AnnotationRenderItem`；
- 鼠标事件；
- 创建、移动、缩放的临时预览；
- 命中测试；
- 图像缩放和平移；
- Widget/Image 坐标转换；
- 发出创建、选择、移动、删除请求；
- 调用 `update()` 请求重绘。

### 绝对不能做

- 修改 `ProjectModel`；
- 创建 `AnnotationModel`；
- 生成业务 ID；
- 决定 `LabelId`；
- 保存标注；
- 调用 Repository、Processor、Exporter；
- 持有 Model 或 ViewModel。

## 7.3 CoordinateMapper

### 必须负责

- Widget ↔ Image 坐标转换；
- 矩形转换；
- 缩放和平移；
- 图片居中偏移；
- 矩形标准化；
- 图像边界裁剪。

### 绝对不能做

- 修改标注；
- 查询 ProjectModel；
- 选择类别；
- 保存和导出数据。

## 7.4 LabelPanel

### 必须负责

- 显示类别列表；
- 显示当前类别；
- 接收类别名称和颜色输入；
- 发出新增、删除、重命名、选择请求。

### 绝对不能做

- 修改 LabelModel；
- 判断类别引用关系；
- 修改 AnnotationModel；
- 保存真实类别列表。

## 7.5 PropertyPanel

### 必须负责

- 显示当前标注信息；
- 显示矩形和类别；
- 发出修改矩形、类别、删除请求。

### 绝对不能做

- 修改 AnnotationModel；
- 查询 ProjectModel；
- 实现业务校验；
- 保存标注状态。

## 7.6 ProcessPanel

### 必须负责

- 显示处理功能；
- 接收阈值、核大小、亮度、对比度等输入；
- 发出处理和重置请求；
- 显示加载或错误状态。

### 绝对不能做

- 调用 OpenCV；
- 修改原图；
- 访问 ProjectModel。

---

# 8. Repository 层

Repository 负责：

```text
ProjectModel <-> JSON
```

## 必须负责

- JSON 序列化；
- JSON 反序列化；
- 文件路径处理；
- 格式版本；
- 文件读写错误；
- 数据格式校验。

## 绝对不能做

- 打开文件对话框；
- 弹保存成功；
- 修改窗口标题；
- 控制状态栏；
- 绘图；
- 决定选中标注；
- 导出 YOLO。

---

# 9. Processor 层

`ImageProcessor` 只负责 OpenCV 算法：

- 灰度化；
- 二值化；
- 高斯滤波；
- 中值滤波；
- Canny；
- 亮度与对比度。

## 绝对不能做

- 持有 MainWindow 或 Canvas；
- 更新控件；
- 选择图片；
- 保存项目；
- 修改标注；
- 弹框；
- 决定是否覆盖原图。

---

# 10. Exporter 层

`YoloExporter` 负责：

- 只读访问 `ProjectModel`；
- 遍历图片和标注；
- 计算 YOLO 归一化坐标；
- 生成每张图片的 `.txt`；
- 生成类别文件；
- 创建输出目录；
- 返回导出结果。

## 绝对不能做

- 修改 ProjectModel；
- 修改标注坐标；
- 重排类别 ID；
- 打开目录选择器；
- 控制进度条；
- 弹成功对话框；
- 访问 View。

---

# 11. View 与 ViewModel 绑定

## 用户请求

```text
View
-> ViewModel
```

例如：

```cpp
connect(
    &mainWindow_.imageCanvas(),
    &ImageCanvas::annotationCreateRequested,
    &annotationViewModel_,
    &AnnotationViewModel::createAnnotation
);
```

## 展示数据

```text
ImageViewModel.currentImageChanged
-> ImageCanvas.setImage

AnnotationViewModel.annotationsChanged
-> ImageCanvas.setAnnotations

LabelViewModel.labelsChanged
-> LabelPanel.setLabels

ProjectViewModel.windowTitleChanged
-> MainWindow.setWindowTitle
```

## 错误通知

```text
ViewModel.errorOccurred
-> MainWindow.showError
```

Repository、Processor、Exporter 只返回结果，不直接弹框。

---

# 12. 多个 ViewModel 共享唯一 Model

```text
ProjectModel
├── ProjectViewModel&
├── ImageViewModel&
├── AnnotationViewModel&
├── LabelViewModel&
└── ProcessViewModel&
```

职责必须严格分开：

```text
ProjectViewModel    项目保存、读取、导出
ImageViewModel      图片导入与切换
AnnotationViewModel 标注增删改与选择
LabelViewModel      类别增删改与当前类别
ProcessViewModel    图像处理预览
```

绝对禁止：

```text
ImageViewModel::renameLabel()
LabelViewModel::deleteAnnotation()
AnnotationViewModel::saveProject()
ProjectViewModel::paintImage()
```

ViewModel 之间不应互相持有指针，通过共享 Model 和信号同步。

---

# 13. 所有权与生命周期

```text
Application
├── owns ProjectModel
├── owns Repository
├── owns Processor
├── owns Exporter
├── owns ViewModels
└── owns MainWindow

ViewModel
├── borrows ProjectModel&
└── borrows required modules

MainWindow
└── owns child Widgets

ImageCanvas
└── owns CoordinateMapper
```

推荐声明顺序：

```cpp
ProjectModel projectModel_;

JsonProjectRepository projectRepository_;
ImageProcessor imageProcessor_;
YoloExporter yoloExporter_;

ProjectViewModel projectViewModel_;
ImageViewModel imageViewModel_;
AnnotationViewModel annotationViewModel_;
LabelViewModel labelViewModel_;
ProcessViewModel processViewModel_;

MainWindow mainWindow_;
```

构造顺序：

```text
Model
-> Repository / Processor / Exporter
-> ViewModel
-> View
```

析构顺序相反。

ViewModel 使用引用表达借用关系，不需要 `shared_ptr`。

---

# 14. 核心交互流程

## 创建标注

```text
用户拖拽
-> ImageCanvas 生成临时矩形
-> CoordinateMapper 转成 imageRect
-> emit annotationCreateRequested(imageRect)
-> AnnotationViewModel 校验当前图片和当前类别
-> ProjectModel 添加标注并标记 dirty
-> AnnotationViewModel 生成 AnnotationRenderItem
-> emit annotationsChanged
-> ImageCanvas::setAnnotations
-> update()
```

## 图片切换

```text
用户点击下一张
-> ImageViewModel 修改 currentImageId
-> 发布 currentImageChanged
-> AnnotationViewModel 清除选择
-> AnnotationViewModel 重新发布标注
-> Canvas 更新图片和标注
```

## 类别重命名

```text
LabelPanel 发出请求
-> LabelViewModel 校验并修改 LabelModel
-> 发布 labelsChanged
-> AnnotationViewModel 重建 AnnotationRenderItem
-> Canvas 更新类别文字
```

## 保存项目

```text
MainWindow 提供路径
-> ProjectViewModel 调用 Repository
-> Repository 保存 JSON
-> ProjectModel 标记 saved
-> ViewModel 发布结果
```

## 导出 YOLO

```text
MainWindow 提供目录
-> ProjectViewModel 调用 YoloExporter
-> Exporter 读取 const ProjectModel&
-> 生成文件
-> ViewModel 发布结果
```

## 图像处理

```text
ProcessPanel 提交参数
-> ProcessViewModel 调用 ImageProcessor
-> Processor 返回 cv::Mat
-> ViewModel 转换为展示图像
-> Canvas 显示预览
```

---

# 15. Command 层

当前不加入。

Qt signal-slot 已经可以完成 View 到 ViewModel 的命令绑定。

正式实现撤销/重做时，再单独增加：

```text
command/
├── IUndoableCommand.h
├── CommandStack.h
├── CreateAnnotationCommand.h
├── DeleteAnnotationCommand.h
└── UpdateAnnotationGeometryCommand.h
```

只对可撤销业务修改使用：

- 创建标注；
- 删除标注；
- 移动标注；
- 调整大小；
- 修改类别。

不要对鼠标移动、拖拽预览、缩放、平移、悬停等 UI 状态使用 Command。

---

# 16. 各层规则总表

| 层 | 必须做 | 绝对不能做 |
|---|---|---|
| App | 创建对象、生命周期、绑定 | 业务逻辑、算法、持久化 |
| Common | ID、Result、ViewData、稳定契约 | 持有业务对象、业务实现、杂项 |
| Model | 真实数据、领域关系、一致性 | GUI、绘制、文件 IO、OpenCV |
| ViewModel | 属性、业务操作、通知、协调、ViewData | 鼠标细节、绘制、控件访问、底层算法 |
| View | 输入、显示、绘制、发出意图 | 访问 Model、修改业务数据 |
| Repository | JSON 持久化 | 弹框、绘制、YOLO |
| Processor | OpenCV 算法 | GUI、业务流程 |
| Exporter | 外部格式输出 | 修改 Model、控制 UI |
| main.cpp | 创建 QApplication 和 Application | 业务逻辑、复杂绑定 |

---

# 17. 最终纪律

1. `ProjectModel` 是唯一真实业务数据源。
2. `Application` 持有 Model、ViewModel、View 和功能模块。
3. View 不认识 Model。
4. View 不调用 Repository、Processor、Exporter。
5. 所有业务修改必须经过 ViewModel。
6. ViewModel 不处理鼠标、绘制、缩放和平移细节。
7. Model 永远保存原始图像坐标。
8. AnnotationModel 只保存 LabelId，不重复保存类别名称。
9. 坐标转换只在 `ImageCanvas / CoordinateMapper` 中完成。
10. JSON 只由 Repository 处理。
11. YOLO 只由 Exporter 处理。
12. OpenCV 只由 Processor 处理。
13. 文件对话框和消息框只由 View 处理。
14. App 只负责装配和生命周期。
15. 当前不设置 Service 层。
16. 当前不在 Common 中放 `ICommandBase`。
17. ViewModel 之间不互相持有指针。
18. 撤销重做以后通过独立 Command 模块扩展。
