#ifndef PANDORA_REAL_DATA_SET_H_
#define PANDORA_REAL_DATA_SET_H_

#include "pandora_box_adapter.h"
#include <vector>
#include <algorithm>

namespace pandora {

template <typename T>
class RealDataSet : public PandoraBoxAdapter<T> {
 public:
  RealDataSet() = default;
  [[nodiscard]] int GetDataCount() const override { return static_cast<int>(data_.size()); }
  T* GetDataByIndex(int index) override {
    if (index < 0 || index >= static_cast<int>(data_.size())) return nullptr;
    return &data_[index];
  }
  void ClearAllData() override { data_.clear(); }
  void Add(const T& item) override { data_.push_back(item); }
  void Add(int pos, const T& item) override {
    if (pos < 0 || pos > static_cast<int>(data_.size())) return;
    data_.insert(data_.begin() + pos, item);
  }
  void AddAll(const std::vector<T>& collection) override {
    data_.insert(data_.end(), collection.begin(), collection.end());
  }
  void Remove(const T& item) override {
    auto it = std::find(data_.begin(), data_.end(), item);
    if (it != data_.end()) data_.erase(it);
  }
  void RemoveAtPos(int position) override {
    if (position < 0 || position >= static_cast<int>(data_.size())) return;
    data_.erase(data_.begin() + position);
  }
  bool ReplaceAtPosIfExist(int position, const T& item) override {
    if (position < 0 || position >= static_cast<int>(data_.size())) return false;
    data_[position] = item;
    return true;
  }
  void SetData(const std::vector<T>& collection) override {
    data_ = collection;
  }
  int IndexOf(const T& item) const override {
    auto it = std::find(data_.begin(), data_.end(), item);
    if (it == data_.end()) return -1;
    return static_cast<int>(std::distance(data_.begin(), it));
  }
  // Node接口实现
  [[nodiscard]] int GetGroupIndex() const override { return group_index_; }
  void SetGroupIndex(int group_index) override { group_index_ = group_index; }

  void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) override {
    throw PandoraException("RealDataSet does not support AddChild");
  }
  [[nodiscard]] bool HasBindToParent() const override { return parent_ != nullptr; }
  void RemoveFromOriginalParent() override {
    if (parent_) {
      parent_->RemoveChild(this);
      parent_ = nullptr;
    }
  }
  void RemoveChild(PandoraBoxAdapter<T>* sub) override {
    throw PandoraException("RealDataSet does not support RemoveChild");
  }

  // Index management
  [[nodiscard]] int GetStartIndex() const override { return start_index_; }
  void SetStartIndex(int start_index) override { start_index_ = start_index; }

  // Parent-child relationship notifications
  void NotifyHasAddToParent(PandoraBoxAdapter<T>* parent) override {
    parent_ = parent;
  }

  void NotifyHasRemoveFromParent() override {
    parent_ = nullptr;
  }

 private:
  std::vector<T> data_;
  int group_index_ = Node<T>::kNoGroupIndex;
  int start_index_ = 0;
  PandoraBoxAdapter<T>* parent_ = nullptr;
};

}  // namespace pandora

#endif  // PANDORA_REAL_DATA_SET_H_
