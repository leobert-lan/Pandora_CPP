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
    Prepare();
    try {
      func(adapter_);
    } catch (const std::exception& e) {
      Logger::Println(Logger::ERROR, "Transaction",
                     std::string("transaction failure: ") + e.what());
      Restore();
    } catch (...) {
      Logger::Println(Logger::ERROR, "Transaction", "transaction failure");
      Restore();
    }
  }

 private:
  void Prepare() {
    adapter_->StartTransaction();
  }

  void Restore() {
    adapter_->Restore();
  }

  PandoraBoxAdapter<T>* adapter_;
};

}  // namespace pandora

#endif  // PANDORA_TRANSACTION_H_

