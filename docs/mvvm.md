# MVVM 落地约束

本项目不把 MVVM 当作目录命名，而是按调用边界约束代码。

## View

- 只创建界面控件、连接按钮和信号槽。
- 可以读取用户输入。
- 可以绘制图片和标注框。
- 不直接修改 `ProjectModel`。
- 不直接保存标注、类别、项目等业务状态。

## Command

- View 不直接调用 ViewModel 的业务方法，而是取得 `ICommandBase` 接口并调用 `execute()`。
- 命令参数统一使用 `std::any`；参数集合使用 `std::tuple`，无参数命令使用 `std::tuple<>`。
- `TupleCommand<Args...>` 负责把 `std::any` 还原为对应的 tuple，再调用 ViewModel 提供的处理函数。
- 命令对象由 ViewModel 独占持有，View 得到的接口指针不拥有对象，也不得释放对象。
- Command 表达从 View 到 ViewModel 的操作请求；Qt signals 表达从 ViewModel 到 View 的状态通知，两者职责不同。

## ViewModel

- 向 View 暴露命令接口，例如 `loadImageCommand()`、`nextPageCommand()`。
- 实际业务方法保持私有，只允许对应命令调用。
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
  -> ImageViewModel::loadImageCommand()
  -> ICommandBase::execute(std::tuple<QString>{path})
  -> TupleCommand<QString> 解包参数
  -> ImageViewModel::loadImage(path)
  -> ProjectModel::images / currentIndex
  -> ImageViewModel::imageChanged(image)
  -> ImageCanvas::setImage(image)
```

“下一页”只提供导航语义接口 `nextPageCommand()`，当前复用既有的下一张图片命令，不引入新的分页状态或业务功能。

命令参数的具体类型由命令接口约定。调用方传入错误类型时，`std::any_cast` 会抛出 `std::bad_any_cast`；View 应始终通过 ViewModel 暴露的命令接口并按文档规定构造 tuple。
