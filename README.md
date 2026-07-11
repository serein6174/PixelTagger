# AnnotaVision

基于 C++ / Qt Widgets / CMake / vcpkg 的轻量级图像标注与处理系统。

## 当前阶段

当前已实现一个真实 MVVM 闭环：打开单张图片、打开图片文件夹、上一张 / 下一张切换、当前类别命名，以及矩形框标注的最小闭环。

图片浏览数据流：

```text
MainWindow 菜单动作
  -> ICommandBase::execute(std::any)
  -> ImageViewModel 私有业务方法
  -> ProjectModel / ImageModel 状态更新
  -> imageChanged / currentImageChanged / statusChanged 信号
  -> ImageCanvas 刷新显示
```

矩形标注数据流：

```text
ImageCanvas 鼠标拖拽
  -> CoordinateMapper 转换为原图坐标 imageRect
  -> annotationCreated(imageRect)
  -> AnnotationViewModel::createAnnotation(imageRect)
  -> ProjectModel.currentImage().annotations 更新
  -> annotationsChanged(AnnotationViewData)
  -> ImageCanvas::setAnnotations()
  -> update()
```

约束：

- `View` 只负责界面、菜单、绘制、鼠标交互和显示坐标转换。
- `ViewModel` 负责业务操作、状态变更、Model 更新和展示数据生成。
- `Model` 只保存数据，不依赖界面控件。
- 标注框在 Model 中永远保存为原图坐标。
- 当前阶段默认标签为 object，可在工具栏的可编辑类别框中重命名。
- 新建标注自动绑定当前类别。
- 最小标注框尺寸为 3 x 3 原图像素。

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


