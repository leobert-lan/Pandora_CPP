#ifndef PANDORA_BOX_ADAPTER_H_
#define PANDORA_BOX_ADAPTER_H_

#include "node.h"
#include "data_adapter.h"
#include "pandora_exception.h"
#include "logger.h"
#include <string>
#include <functional>

namespace pandora {

template <typename T>
class PandoraBoxAdapter : public Node<PandoraBoxAdapter<T>>, public DataAdapter<T> {
 public:
  using Consumer = std::function<void(const T&)>;
  PandoraBoxAdapter() = default;
  ~PandoraBoxAdapter() override = default;

  int GetDataCount() const override = 0;
  T* GetDataByIndex(int index) override = 0;
  void ClearAllData() override = 0;
  void Add(const T& item) override = 0;
  void Add(int pos, const T& item) override = 0;
  void AddAll(const std::vector<T>& collection) override = 0;
  void Remove(const T& item) override = 0;
  void RemoveAtPos(int position) override = 0;
  bool ReplaceAtPosIfExist(int position, const T& item) override = 0;
  void SetData(const std::vector<T>& collection) override = 0;
  int IndexOf(const T& item) const override = 0;

  void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) override = 0;
  void RemoveChild(PandoraBoxAdapter<T>* sub) override = 0;

  virtual void RunForeach(const Consumer& action) {
    int count = GetDataCount();
    for (int i = 0; i < count; ++i) {
      try {
        action(*GetDataByIndex(i));
      } catch (...) {
        Logger::Println(Logger::ERROR, "PandoraBoxAdapter", "Exception in RunForeach");
      }
    }
  }

  void SetAlias(const std::string& alias) {
    // TODO: 检查别名唯一性
    alias_ = alias;
  }
  std::string GetAlias() const { return alias_; }

 protected:
  std::string alias_;
};

}  // namespace pandora

#endif  // PANDORA_BOX_ADAPTER_H_
