# AnnotaVision

基于 C++ / Qt Widgets / CMake / vcpkg 的轻量级图像标注与处理系统。

## 当前阶段

当前已实现一个严格分层的 MVVM 最小闭环：打开单张图片、打开图片文件夹、上一张 / 下一张切换、当前类别命名、矩形框标注，以及 JSON 项目保存/读取。

`Application` 是组合根，负责创建唯一 `ProjectModel`、Repository、ViewModel 和 MainWindow，并建立绑定。MainWindow 只管理界面和窗口级交互，不持有 Model 或 ViewModel。

图片浏览数据流：

```text
MainWindow 发出导入或切换请求
  -> ImageViewModel
  -> ImageViewModel 内部执行导入和目录扫描
  -> ProjectModel 受控接口更新状态
  -> changed(CurrentImage) / statusChanged 信号
  -> Application 拉取 currentQImage()
  -> ImageCanvas 刷新显示
```

矩形标注数据流：

```text
ImageCanvas 鼠标拖拽
  -> CoordinateMapper 转换为原图坐标 imageRect
  -> annotationCreateRequested(imageRect)
  -> AnnotationViewModel::createAnnotation(imageRect)
  -> ProjectModel.currentImage().annotations 更新
  -> changed(Annotations)
  -> Application 拉取 annotationItems()
  -> ImageCanvas::setAnnotations()
  -> update()
```

项目保存/读取数据流：

```text
MainWindow 发出打开或保存项目请求
  -> ProjectViewModel
  -> JsonProjectRepository
  -> ProjectModel 与 JSON 文件互相转换
  -> changed(Project)
  -> ImageViewModel / LabelViewModel 响应项目变化并重新发布状态
  -> AnnotationViewModel 响应图片和类别变化
  -> Application 仅按绑定将展示数据更新到 View
```

约束：

- `View` 只负责界面、菜单、绘制、鼠标交互和显示坐标转换。
- `Application` 只负责对象生命周期和信号绑定，不执行业务逻辑。
- `ViewModel` 负责业务操作、状态变更、Model 更新和展示数据生成。
- `Repository` 只负责项目数据与 JSON 文件之间的持久化转换，不持有界面状态。
- 大数据变化使用 `changed(ViewModelChange)` 枚举通知，再由 Application 通过 getter 拉取展示数据。
- `Model` 是唯一真实业务数据源，通过受控接口维护 ID、关系和 dirty 状态。
- `Common` 只保存实体 ID、Result 和展示 DTO 等稳定跨层契约。
- 标注框在 Model 中永远保存为原图坐标。
- 当前阶段默认标签为 object，可在工具栏的可编辑类别框中重命名。
- 新建标注自动绑定当前类别。
- 最小标注框尺寸为 3 x 3 原图像素。
- JSON 项目文件使用相对图片路径，避免保存本机绝对路径。

## 环境

- Visual Studio 2022 C++ 桌面开发工具集，或可用的 MSVC 编译环境。
- CMake 3.20 或更高版本。
- vcpkg。
- Qt Widgets 开发包，建议安装与 MSVC 版本匹配的 64-bit Qt kit。

请在本机设置环境变量：

- `VCPKG_ROOT`：指向 vcpkg 安装目录。
- `QT_DIR` 或 `CMAKE_PREFIX_PATH`：指向 Qt kit 安装前缀，例如 `<QtRoot>/<QtVersion>/msvc2022_64`。

如果 Qt 不在 CMake 默认搜索路径中，配置时需要补充 `CMAKE_PREFIX_PATH`：

```powershell
cmake --preset vs2022-x64 -DCMAKE_PREFIX_PATH=<QtPrefix>
cmake --build --preset vs2022-x64
```

其中 `<QtPrefix>` 是包含 `lib/cmake/Qt6/Qt6Config.cmake` 的 Qt 安装前缀。

运行 Debug 版本时，如果程序启动后立即退出或提示缺少 Qt 插件，可以手动部署 Qt 运行时：

```powershell
windeployqt <path-to-build-output>/Debug/AnnotaVision.exe
```

后续接入 OpenCV 和 nlohmann-json 时，再通过 vcpkg 安装：

```powershell
vcpkg install opencv:x64-windows nlohmann-json:x64-windows
```

这个命令需要联网下载依赖，执行前应先确认。

