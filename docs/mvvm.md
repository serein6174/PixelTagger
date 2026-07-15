# MVVM 落地约束

本项目不把 MVVM 当作目录命名，而是按调用边界约束代码。

## View

- 只创建界面控件、连接按钮和信号槽。
- 可以读取用户输入。
- 可以绘制图片、标注框和拖拽预览框。
- 可以执行显示坐标与原图坐标之间的转换，因为这些转换依赖 Canvas 的显示状态。
- 不直接修改 `ProjectModel`。
- 不直接保存标注、类别、项目等业务状态。

## ViewModel

- 暴露面向界面的业务方法，例如 `loadImage()`、`nextImage()`、`createAnnotation()`、`setCurrentLabelName()`。
- 维护当前图片、当前类别、当前选中标注等界面业务状态。
- 修改 Model，但通过 `ProjectModel` 的受控接口修改。
- 将 Model 转换为 View 专用展示数据，例如 `AnnotationRenderData`。
- 对大数据变化只发 `changed(ViewModelChange)` 枚举通知，View 再通过 ViewModel getter 拉取展示数据。
- 小数据提示仍可直接传参，例如 `statusChanged(QString)` 和 `errorOccurred(QString)`。

## Model

- 保存项目、图片、类别和标注数据。
- 不继承 QWidget。
- 不调用 View。
- 不发起 UI 操作。
- 标注框只保存原图坐标。
- 对外提供只读查询和受控修改接口，不暴露可写数据成员指针。

## 通知绑定

通知枚举统一定义在 `src/common/types/ViewModelChange.h`：

```cpp
enum class ViewModelChange {
    CurrentImage,
    Annotations,
    CurrentLabel,
    Project
};
```

大数据不直接通过 signal 参数传递。当前规则是：

```text
ViewModel emit changed(ViewModelChange)
  -> Application 判断变化类型
  -> Application 从 ViewModel getter 拉取展示数据
  -> View 更新界面
```

这样可以避免 `QImage`、`QVector<AnnotationRenderData>` 等对象频繁作为信号参数传递，也避免后续为每一种数据类型扩展不同的信号原型。

## 当前闭环

打开图片：

```text
MainWindow::openImage()
  -> importImageRequested(path)
  -> ImageViewModel::loadImage(path)
  -> ProjectModel 替换图片列表
  -> ImageViewModel::changed(CurrentImage)
  -> Application 拉取 ImageViewModel::currentQImage()
  -> ImageCanvas::setImage(image)
  -> AnnotationViewModel::onImageViewModelChanged(CurrentImage)
```

创建矩形标注：

```text
ImageCanvas 鼠标拖拽
  -> CoordinateMapper 生成 imageRect
  -> annotationCreateRequested(imageRect)
  -> AnnotationViewModel::createAnnotation(imageRect)
  -> ProjectModel::addAnnotationToCurrentImage(...)
  -> AnnotationViewModel::changed(Annotations)
  -> Application 拉取 AnnotationViewModel::annotationItems()
  -> ImageCanvas::setAnnotations(renderData)
```

当前类别命名：

```text
MainWindow 工具栏可编辑类别框
  -> labelNameChangeRequested(name)
  -> LabelViewModel::setCurrentLabelName(name)
  -> ProjectModel::renameDefaultLabel(name)
  -> LabelViewModel::changed(CurrentLabel)
  -> Application 拉取 LabelViewModel::currentLabelName()
  -> MainWindow::setCurrentLabelName(name)
  -> AnnotationViewModel::onLabelViewModelChanged(CurrentLabel)
  -> AnnotationViewModel::changed(Annotations)
```

项目保存/读取：

```text
MainWindow 打开/保存项目菜单
  -> openProjectRequested(path) / saveProjectRequested(path)
  -> ProjectViewModel::openProject(path) / saveProject(path)
  -> JsonProjectRepository::load(path) / save(project, path)
  -> ProjectModel 替换或序列化项目数据
  -> ProjectViewModel::changed(Project)
  -> ImageViewModel::onProjectChanged(Project)
  -> LabelViewModel::onProjectChanged(Project)
  -> AnnotationViewModel 响应图片和类别变化并重新发布标注
  -> Application 仅按绑定将各 ViewModel 的展示数据更新到 View
```
