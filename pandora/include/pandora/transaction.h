#ifndef PANDORA_TRANSACTION_H_
#define PANDORA_TRANSACTION_H_

#include "pandora_box_adapter.h"
#include "logger.h"
#include <functional>

namespace pandora {

template <typename T>
class Transaction {
 public:
  using Function = std::function<void(PandoraBoxAdapter<T>*)>;
  explicit Transaction(PandoraBoxAdapter<T>* adapter) : adapter_(adapter) {}
  void Apply(const Function& func) {
    // TODO: 快照与恢复机制
    try {
      adapter_->StartTransaction();
      func(adapter_);
      adapter_->EndTransaction();
    } catch (...) {
      Logger::Println(Logger::ERROR, "Transaction", "transaction failure");
      adapter_->Restore();
    }
  }
 private:
  PandoraBoxAdapter<T>* adapter_;
};

}  // namespace pandora

#endif  // PANDORA_TRANSACTION_H_

