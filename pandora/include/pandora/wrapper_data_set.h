#ifndef PANDORA_WRAPPER_DATA_SET_H_
#define PANDORA_WRAPPER_DATA_SET_H_

#include "pandora_box_adapter.h"
#include <vector>
#include <algorithm>
#include <memory>

namespace pandora {

template <typename T>
class WrapperDataSet : public PandoraBoxAdapter<T> {
 public:
  WrapperDataSet() = default;
  [[nodiscard]] int GetDataCount() const override {
    int count = 0;
    for (const auto& sub : subs_) {
      if (sub) count += sub->GetDataCount();
    }
    return count;
  }
  T* GetDataByIndex(const int index) override {
    int offset = 0;
    for (auto& sub : subs_) {
      const int sub_count = sub->GetDataCount();
      if (index < offset + sub_count) {
        return sub->GetDataByIndex(index - offset);
      }
      offset += sub_count;
    }
    return nullptr;
  }
  void ClearAllData() override {
    for (auto& sub : subs_) {
      if (sub) sub->ClearAllData();
    }
  }
  void Add(const T& item) override {
    if (!subs_.empty()) subs_.back()->Add(item);
  }
  void Add(const int pos, const T& item) override {
    int offset = 0;
    for (auto& sub : subs_) {
      const int sub_count = sub->GetDataCount();
      if (pos < offset + sub_count) {
        sub->Add(pos - offset, item);
        return;
      }
      offset += sub_count;
    }
    Add(item);
  }
  void AddAll(const std::vector<T>& collection) override {
    if (!subs_.empty()) subs_.back()->AddAll(collection);
  }
  void Remove(const T& item) override {
    for (auto& sub : subs_) {
      if (sub) sub->Remove(item);
    }
  }
  void RemoveAtPos(const int position) override {
    int offset = 0;
    for (auto& sub : subs_) {
      const int sub_count = sub->GetDataCount();
      if (position < offset + sub_count) {
        sub->RemoveAtPos(position - offset);
        return;
      }
      offset += sub_count;
    }
  }
  bool ReplaceAtPosIfExist(const int position, const T& item) override {
    int offset = 0;
    for (auto& sub : subs_) {
      const int sub_count = sub->GetDataCount();
      if (position < offset + sub_count) {
        return sub->ReplaceAtPosIfExist(position - offset, item);
      }
      offset += sub_count;
    }
    return false;
  }
  void SetData(const std::vector<T>& collection) override {
    // no-op
  }
  int IndexOf(const T& item) const override {
    int offset = 0;
    for (const auto& sub : subs_) {
      const int idx = sub->IndexOf(item);
      if (idx >= 0) return offset + idx;
      offset += sub->GetDataCount();
    }
    return -1;
  }
  void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) override {
    if (!sub) return;

    // If sub already has a parent, remove it from the original parent first
    if (sub->HasBindToParent()) {
      sub->RemoveFromOriginalParent();
    }

    // Set group index and notify the child about the new parent
    const int group_index = static_cast<int>(subs_.size());
    sub->SetGroupIndex(group_index);

    const int count = GetDataCount();
    sub->SetStartIndex(count);

    // Store raw pointer before moving
    PandoraBoxAdapter<T>* sub_ptr = sub.get();
    subs_.push_back(std::move(sub));

    // Notify child it has been added to parent
    sub_ptr->NotifyHasAddToParent(this);
  }

  void RemoveChild(PandoraBoxAdapter<T>* sub) override {
    auto it = std::remove_if(subs_.begin(), subs_.end(),
      [sub](const std::unique_ptr<PandoraBoxAdapter<T>>& ptr) {
        return ptr.get() == sub;
      });

    if (it != subs_.end()) {
      // Notify child it has been removed from parent
      (*it)->NotifyHasRemoveFromParent();
      subs_.erase(it, subs_.end());
    }
  }

  [[nodiscard]] int GetGroupIndex() const override { return group_index_; }

  void SetGroupIndex(int group_index) { group_index_ = group_index; }

  [[nodiscard]] bool HasBindToParent() const override {
    return parent_ != nullptr;
  }

  void RemoveFromOriginalParent() override {
    if (parent_ != nullptr) {
      parent_->RemoveChild(this);
      parent_ = nullptr;
    }
  }

  void NotifyHasAddToParent(PandoraBoxAdapter<T>* parent) {
    parent_ = parent;
  }

  void NotifyHasRemoveFromParent() {
    parent_ = nullptr;
  }

  [[nodiscard]] int GetStartIndex() const { return start_index_; }

  void SetStartIndex(int start_index) { start_index_ = start_index; }

 private:
  std::vector<std::unique_ptr<PandoraBoxAdapter<T>>> subs_;
  int group_index_ = Node<T>::kNoGroupIndex;
  int start_index_ = 0;
  PandoraBoxAdapter<T>* parent_ = nullptr;
};

}  // namespace pandora

#endif  // PANDORA_WRAPPER_DATA_SET_H_
