# std::unique_ptr 设计说明

## 为什么使用 std::unique_ptr？

### 1. **明确的所有权语义**
```cpp
void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub);
```
这个签名明确表达：**父节点获得子节点的独占所有权**。当子节点被添加到父节点后，父节点负责管理其生命周期。

### 2. **自动内存管理**
```cpp
class WrapperDataSet {
  std::vector<std::unique_ptr<PandoraBoxAdapter<T>>> subs_;
};
```
当 `WrapperDataSet` 被销毁时，`subs_` 中的所有子节点自动释放，无需手动 `delete`，避免内存泄漏。

### 3. **防止意外复制**
`std::unique_ptr` 不可复制，只能移动，强制开发者明确表达所有权转移的意图：
```cpp
auto child = std::make_unique<RealDataSet<int>>();
wrapper->AddChild(std::move(child));  // 必须显式 std::move
// wrapper->AddChild(child);  // 编译错误！防止意外复制
```

### 4. **类型安全**
相比原始指针，`unique_ptr` 在编译期提供更强的类型检查，减少运行时错误。

---

## ⚠️ 常见陷阱：use-after-move

### ❌ 错误示例
```cpp
auto dataset = std::make_unique<RealDataSet<int>>();
wrapper->AddChild(std::move(dataset));

// 危险！dataset 已经是 nullptr，调用会崩溃
dataset->SetAlias("child1");  // Undefined Behavior!
```

**原因**：`std::move(dataset)` 后，`dataset` 的内部指针已被转移给 `wrapper`，`dataset` 变成空的 `unique_ptr`。

### ✅ 正确方案 1：提前保存原始指针
```cpp
auto dataset = std::make_unique<RealDataSet<int>>();
auto* datasetPtr = dataset.get();  // 保存原始指针

wrapper->AddChild(std::move(dataset));  // 转移所有权

datasetPtr->SetAlias("child1");  // 安全：通过原始指针访问
```

**注意**：`datasetPtr` 是非拥有指针，只要 `wrapper` 存活且未删除该子节点，`datasetPtr` 就有效。

### ✅ 正确方案 2：通过父节点查找
```cpp
auto dataset = std::make_unique<RealDataSet<int>>();
wrapper->AddChild(std::move(dataset));

// 通过父节点的查找功能访问子节点
auto* child = wrapper->FindByAlias("someAlias");
if (child) {
  child->SetAlias("newAlias");
}
```

### ✅ 正确方案 3：先设置属性，再添加
```cpp
auto dataset = std::make_unique<RealDataSet<int>>();
dataset->SetAlias("child1");  // 先配置

wrapper->AddChild(std::move(dataset));  // 最后转移所有权
```

---

## 设计权衡

### 为什么不用 shared_ptr？
```cpp
void AddChild(std::shared_ptr<PandoraBoxAdapter<T>> sub);  // 替代方案
```

**缺点**：
1. **性能开销**：`shared_ptr` 有引用计数的原子操作开销
2. **所有权不清晰**：多个对象可能持有同一子节点，生命周期管理复杂
3. **循环引用风险**：如果父子双向持有 `shared_ptr`，可能导致内存泄漏

**unique_ptr 更适合树形结构**：父节点独占子节点，所有权层次清晰。

### 为什么 RemoveChild 用原始指针？
```cpp
void RemoveChild(PandoraBoxAdapter<T>* sub);  // 使用原始指针
```

**原因**：
1. 调用者只需提供"要删除哪个子节点"的信息，不需要转移所有权
2. 删除操作由父节点内部完成（释放 `unique_ptr`）
3. 避免强制调用者创建临时的智能指针

---

## 实际使用模式

### 模式 1：构建后立即添加
```cpp
auto wrapper = std::make_unique<WrapperDataSet<int>>();

auto* child1Ptr = nullptr;
{
  auto child1 = std::make_unique<RealDataSet<int>>();
  child1Ptr = child1.get();
  wrapper->AddChild(std::move(child1));
}
child1Ptr->Add(100);  // 安全
```

### 模式 2：批量构建
```cpp
auto wrapper = std::make_unique<WrapperDataSet<int>>();
std::vector<RealDataSet<int>*> childPtrs;

for (int i = 0; i < 3; ++i) {
  auto child = std::make_unique<RealDataSet<int>>();
  childPtrs.push_back(child.get());
  wrapper->AddChild(std::move(child));
}

// 通过保存的指针访问
for (auto* ptr : childPtrs) {
  ptr->Add(i * 10);
}
```

### 模式 3：配置后添加
```cpp
auto child = std::make_unique<RealDataSet<int>>();
child->SetAlias("myChild");
child->Add(1);
child->Add(2);

wrapper->AddChild(std::move(child));  // 最后转移
```

---

## 总结

| 特性 | unique_ptr | shared_ptr | 原始指针 |
|------|-----------|------------|---------|
| 所有权 | 独占 | 共享 | 无 |
| 内存管理 | 自动 | 自动（引用计数） | 手动 |
| 性能 | 高 | 中等（原子操作） | 最高 |
| 安全性 | 高 | 高 | 低 |
| 适用场景 | 树形结构 | 图结构 | 临时访问 |

**Pandora-CPP 采用 unique_ptr 是正确的选择**，它在性能、安全性和语义清晰度之间取得了最佳平衡。

关键是要记住：**一旦 std::move，原对象就不能再使用，必须提前保存原始指针！**

