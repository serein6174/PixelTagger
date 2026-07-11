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
- 维护当前图片索引、当前图片、当前选中标注、当前类别等界面业务状态。
- 修改 Model。
- 将 Model 转换为 View 专用展示数据，例如 `AnnotationViewData`。
- 通过 Qt signals 通知 View 更新。

## Model

- 保存项目、图片、类别和标注数据。
- 不继承 QWidget。
- 不调用 View。
- 不发起 UI 操作。
- 标注框只保存原图坐标。

## 当前闭环

打开图片：

```text
MainWindow::openImage()
  -> ImageViewModel::loadImage(path)
  -> ProjectModel::images / currentIndex
  -> ImageViewModel::imageChanged(image)
  -> ImageCanvas::setImage(image)
```

创建矩形标注：

```text
ImageCanvas 鼠标拖拽
  -> CoordinateMapper 生成 imageRect
  -> ImageCanvas::annotationCreated(imageRect)
  -> AnnotationViewModel::createAnnotation(imageRect)
  -> ProjectModel.currentImage().annotations
  -> AnnotationViewModel::annotationsChanged(viewData)
  -> ImageCanvas::setAnnotations(viewData)
```

当前类别命名：

```text
MainWindow 工具栏可编辑类别框
  -> LabelViewModel::setCurrentLabelName(name)
  -> ProjectModel.labels 更新
  -> LabelViewModel::labelsChanged()
  -> AnnotationViewModel::onLabelsChanged()
  -> AnnotationViewModel::annotationsChanged(viewData)
  -> ImageCanvas::setAnnotations(viewData)
```
