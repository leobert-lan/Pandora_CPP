#ifndef PANDORA_EXCEPTION_H_
#define PANDORA_EXCEPTION_H_

#include <exception>
#include <string>
#include <utility>

namespace pandora {

class PandoraException : public std::exception {
 public:
  explicit PandoraException(std::string  message) : message_(std::move(message)) {}
  [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }
  static PandoraException AliasConflict(const std::string& alias) {
    return PandoraException("alias want to set is conflicted:" + alias);
  }
 private:
  std::string message_;
};

}  // namespace pandora

#endif  // PANDORA_EXCEPTION_H_

