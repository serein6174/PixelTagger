# 已有标注修改类别：并行开发交接说明

## 当前状态

“已有标注修改类别”的 Model 和 ViewModel 已完成并通过自动测试，但 View 和 Application 尚未接入。

当前已有接口：

```cpp
ProjectModel::findAnnotationInCurrentImage(AnnotationId annotationId)
ProjectModel::changeAnnotationLabel(
    AnnotationId annotationId,
    LabelId labelId
)

AnnotationViewModel::selectAnnotation(AnnotationId annotationId)
AnnotationViewModel::clearSelection()
AnnotationViewModel::setSelectedAnnotationLabel(LabelId labelId)
AnnotationViewModel::selectedAnnotationId()
```

已有规则：

- 标注只保存稳定 `LabelId`，不重复保存类别名称和颜色；
- 目标标注必须属于当前图片；
- 目标类别必须存在；
- 当前选中标注 ID 只保存在 `AnnotationViewModel`；
- 类别名称和颜色由 `AnnotationViewModel::annotationItems()` 根据 `LabelId` 生成；
- 修改成功后图片和项目进入 dirty 状态；
- 修改失败时 Model 数据保持不变；
- View 不负责检查标注和类别是否存在。

---

# App 层负责人

## 负责范围

主要修改：

```text
src/app/Application.cpp
```

通常不需要修改：

```text
src/model/
src/viewmodel/
src/repository/
src/common/
```

## 标注选择绑定

Canvas 命中已有标注后，应把稳定 `AnnotationId` 交给
`AnnotationViewModel`：

```cpp
QObject::connect(
    &canvas,
    &ImageCanvas::annotationSelectRequested,
    &annotationViewModel_,
    &AnnotationViewModel::selectAnnotation
);
```

点击空白区域时清除选择：

```cpp
QObject::connect(
    &canvas,
    &ImageCanvas::annotationSelectionClearRequested,
    &annotationViewModel_,
    &AnnotationViewModel::clearSelection
);
```

## 修改选中标注类别

推荐由 View 发出：

```cpp
void selectedAnnotationLabelChangeRequested(LabelId labelId);
```

Application 只负责连接：

```cpp
QObject::connect(
    &mainWindow_,
    &MainWindow::selectedAnnotationLabelChangeRequested,
    &annotationViewModel_,
    &AnnotationViewModel::setSelectedAnnotationLabel
);
```

## 选择状态同步

`AnnotationViewModel::annotationsChanged()` 已经会使
Application 重新拉取 `annotationItems()`，Canvas 可以由其中的
`selected` 字段绘制高亮。

如果 View 需要启用或禁用“应用类别”按钮，可以绑定：

```cpp
QObject::connect(
    &annotationViewModel_,
    &AnnotationViewModel::selectionChanged,
    &mainWindow_,
    &MainWindow::setAnnotationEditEnabled
);
```

也可以让 View 根据选中展示数据控制按钮，但不能自己保存第二份选中
`AnnotationId` 作为业务状态。

## App 层禁止事项

Application 不允许：

- 判断点击坐标落在哪个标注框；
- 根据类别名称查找 `LabelId`；
- 直接修改 `AnnotationModel::labelId`；
- 判断目标类别是否合法；
- 在绑定 lambda 中实现修改类别规则；
- 保存当前选中标注 ID。

Application 只负责对象装配、信号连接和展示数据转交。

## App 层验收

1. Canvas 发送的是稳定 `AnnotationId`。
2. View 发送的是稳定 `LabelId`。
3. 请求最终进入 `AnnotationViewModel::setSelectedAnnotationLabel()`。
4. 修改成功后 Canvas 自动重新拉取并显示新类别名称和颜色。
5. Application 中没有标注或类别业务判断。

---

# View 层负责人

## 负责范围

主要修改：

```text
src/view/canvas/ImageCanvas.h
src/view/canvas/ImageCanvas.cpp
src/view/MainWindow.h
src/view/MainWindow.cpp
```

如果增加属性面板，可以新增：

```text
src/view/annotation/AnnotationPropertyPanel.h
src/view/annotation/AnnotationPropertyPanel.cpp
```

不要修改：

```text
src/model/
src/viewmodel/
src/repository/
```

## Canvas 标注命中

Canvas 已持有：

```cpp
QVector<AnnotationRenderItem> annotations_;
```

鼠标点击时：

1. 将标注的 `imageRect` 转换成 Widget 坐标；
2. 从后往前遍历，优先选择最后绘制的标注；
3. 命中后发送该标注的稳定 ID；
4. 未命中时发送清除选择请求。

推荐信号：

```cpp
signals:
    void annotationSelectRequested(AnnotationId annotationId);
    void annotationSelectionClearRequested();
```

命中测试属于 View，因为它依赖 Canvas 缩放和显示坐标。

不要把 Widget 坐标传给 ViewModel，也不要让 ViewModel执行命中测试。

## 点击与创建标注的冲突

当前 Canvas 左键拖动用于创建标注。接入选择时建议使用以下规则：

```text
左键按下已有标注
  -> 选择该标注
  -> 不开始创建新标注

左键按下图片空白区域
  -> 清除旧选择
  -> 开始创建新标注
```

这样可以避免点击标注时意外创建很小的新框。

## 修改类别的 UI

最小实现可以在工具栏增加：

```text
“应用当前类别到选中标注”按钮
```

点击时发送当前类别下拉框中的稳定 `LabelId`：

```cpp
emit selectedAnnotationLabelChangeRequested(currentComboLabelId());
```

推荐接口：

```cpp
signals:
    void selectedAnnotationLabelChangeRequested(LabelId labelId);

public slots:
    void setAnnotationEditEnabled(bool enabled);
```

按钮初始禁用，只有选中标注时启用。

后续如果实现属性面板，可以改成独立类别下拉框，但仍必须保存
`LabelId`，不能用类别名称或下拉索引作为业务身份。

## 选中状态显示

`AnnotationRenderItem::selected` 已经存在。

Canvas 绘制时应继续使用该字段，例如：

```cpp
QPen pen(
    annotation.color,
    annotation.selected ? 3.0 : 2.0
);
```

可以为选中框增加控制点或半透明背景，但这只属于显示效果，不能在
Canvas 中修改真实标注类别。

## View 层可以做的事情

- Widget 坐标下的标注框命中测试；
- 决定点击空白处清除选择；
- 展示选中高亮；
- 启用或禁用编辑按钮；
- 从类别下拉框读取稳定 `LabelId`；
- 发出修改选中标注类别的用户请求。

## View 层不能做的事情

- 直接修改 `AnnotationRenderItem` 并把它当成真实数据；
- 直接修改 `AnnotationModel::labelId`；
- include 或调用 `AnnotationViewModel`；
- 根据类别名称查找类别；
- 判断目标类别是否存在；
- 保存第二份真实标注集合；
- 使用下拉索引作为 `LabelId`；
- 把 Widget 坐标交给 Model 或 ViewModel。

## View 层验收

1. 点击已有框时该框高亮。
2. 点击空白区域时取消选择。
3. 点击已有框不会创建新框。
4. 未选择标注时“应用当前类别”按钮禁用。
5. 选择 `person` 并应用后，已有框显示为 `person` 的名称和颜色。
6. View 源码中不存在 Model、ViewModel 或 Repository 依赖。

边界检查：

```powershell
rg '#include "(model|viewmodel|repository|app)/' src/view
```

预期没有输出。

---

# 联调流程

建议合并顺序：

1. View 层完成 Canvas 命中、选择信号和修改类别按钮。
2. App 层完成 signals/slots 绑定。
3. 运行自动测试。
4. 手工验证完整闭环。

自动测试：

```powershell
cmake --build --preset default --config Debug
ctest --test-dir build/default -C Debug --output-on-failure
```

只运行标注编辑测试：

```powershell
ctest --test-dir build/default -C Debug \
  -R AnnotationEditingTests --output-on-failure
```

手工验证：

1. 新增 `car` 和 `person`。
2. 选择 `car` 创建一个标注。
3. 单击该标注，确认出现选中高亮。
4. 将当前类别切换为 `person`。
5. 点击“应用当前类别到选中标注”。
6. 确认标注名称和颜色变为 `person`。
7. 保存项目并重新打开。
8. 确认该标注仍然属于 `person`。
9. 切换图片，确认旧图片的选中状态被清除。

## 冲突提醒

- View 层负责人不要修改 AnnotationViewModel 来适配控件。
- App 层负责人不要在 Application 中实现命中测试。
- 不要使用类别名称代替 `LabelId`。
- 不要使用标注在 QVector 中的下标代替 `AnnotationId`。
- Repository 已保存 `annotation.labelId`，不需要修改 JSON 格式。
- 当前 Model/ViewModel 还同时提供删除和矩形更新接口，但本交接只要求
  接入“已有标注修改类别”，不要顺便扩大到拖动缩放或删除 UI。
