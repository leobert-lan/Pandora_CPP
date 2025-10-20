#ifndef PANDORA_DATA_ADAPTER_H_
#define PANDORA_DATA_ADAPTER_H_

#include <vector>

namespace pandora {

template <typename T>
class DataAdapter {
 public:
  virtual int GetDataCount() const = 0;
  virtual T* GetDataByIndex(int index) = 0;
  virtual void ClearAllData() = 0;
  virtual void Add(const T& item) = 0;
  virtual void Add(int pos, const T& item) = 0;
  virtual void AddAll(const std::vector<T>& collection) = 0;
  virtual void Remove(const T& item) = 0;
  virtual void RemoveAtPos(int position) = 0;
  virtual bool ReplaceAtPosIfExist(int position, const T& item) = 0;
  virtual void SetData(const std::vector<T>& collection) = 0;
  virtual int IndexOf(const T& item) const = 0;
  virtual ~DataAdapter() = default;
};

}  // namespace pandora

#endif  // PANDORA_DATA_ADAPTER_H_

