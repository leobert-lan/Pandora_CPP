#ifndef PANDORA_NODE_H_
#define PANDORA_NODE_H_
#include <memory>

namespace pandora {

template <typename T>
class Node {
 public:
  static constexpr int kNoGroupIndex = -1;
  [[nodiscard]] virtual int GetGroupIndex() const = 0;
  virtual void AddChild(std::unique_ptr<T> sub) = 0;
  [[nodiscard]] virtual bool HasBindToParent() const = 0;
  virtual void RemoveFromOriginalParent() = 0;
  virtual void RemoveChild(T* sub) = 0;
  virtual ~Node() = default;
};

}  // namespace pandora

#endif  // PANDORA_NODE_H_
