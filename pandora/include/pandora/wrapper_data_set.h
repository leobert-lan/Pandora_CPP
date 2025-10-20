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
  int GetDataCount() const override {
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
    if (sub) subs_.push_back(std::move(sub));
  }
  void RemoveChild(PandoraBoxAdapter<T>* sub) override {
    auto it = std::remove_if(subs_.begin(), subs_.end(), [sub](const std::unique_ptr<PandoraBoxAdapter<T>>& ptr) { return ptr.get() == sub; });
    subs_.erase(it, subs_.end());
  }
  int GetGroupIndex() const override { return group_index_; }
  bool HasBindToParent() const override
  {
    return false;
  }
  void RemoveFromOriginalParent() override
  {

  }

 private:
  std::vector<std::unique_ptr<PandoraBoxAdapter<T>>> subs_;
  int group_index_ = Node<T>::kNoGroupIndex;
};

}  // namespace pandora

#endif  // PANDORA_WRAPPER_DATA_SET_H_
