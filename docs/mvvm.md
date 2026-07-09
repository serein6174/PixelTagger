# MVVM 落地约束

本项目不把 MVVM 当作目录命名，而是按调用边界约束代码。

## View

- 只创建界面控件、连接按钮和信号槽。
- 可以读取用户输入。
- 可以绘制图片和标注框。
- 不直接修改 `ProjectModel`。
- 不直接保存标注、类别、项目等业务状态。

## ViewModel

- 暴露面向界面的业务方法，例如 `loadImage()`、`nextImage()`。
- 维护当前图片索引、当前图片、当前类别等界面状态。
- 修改 Model。
- 通过 Qt signals 通知 View 更新。

## Model

- 保存数据结构。
- 不继承 QWidget。
- 不调用 View。
- 不发起 UI 操作。

## 当前闭环

第一条功能是打开单张图片：

```text
MainWindow::openImage()
  -> ImageViewModel::loadImage(path)
  -> ProjectModel::images / currentIndex
  -> ImageViewModel::imageChanged(image)
  -> ImageCanvas::setImage(image)
```
