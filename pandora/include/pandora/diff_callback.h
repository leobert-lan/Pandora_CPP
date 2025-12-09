#ifndef PANDORA_DIFF_CALLBACK_H_
#define PANDORA_DIFF_CALLBACK_H_

namespace pandora {

/**
 * A Callback class used by DiffUtil while calculating the diff between two lists.
 */
class DiffCallback {
 public:
  /**
   * Returns the size of the old list.
   *
   * @return The size of the old list.
   */
  virtual int GetOldListSize() const = 0;

  /**
   * Returns the size of the new list.
   *
   * @return The size of the new list.
   */
  virtual int GetNewListSize() const = 0;

  /**
   * Called by the DiffUtil to decide whether two objects represent the same Item.
   *
   * For example, if your items have unique ids, this method should check their id equality.
   *
   * @param old_item_position The position of the item in the old list
   * @param new_item_position The position of the item in the new list
   * @return True if the two items represent the same object or false if they are different.
   */
  virtual bool AreItemsTheSame(int old_item_position, int new_item_position) const = 0;

  /**
   * Called by the DiffUtil when it wants to check whether two items have the same data.
   * DiffUtil uses this information to detect if the contents of an item has changed.
   *
   * This method is called only if AreItemsTheSame returns true for these items.
   *
   * @param old_item_position The position of the item in the old list
   * @param new_item_position The position of the item in the new list
   * @return True if the contents of the items are the same or false if they are different.
   */
  virtual bool AreContentsTheSame(int old_item_position, int new_item_position) const = 0;

  /**
   * When AreItemsTheSame returns true for two items and AreContentsTheSame returns false
   * for them, DiffUtil calls this method to get a payload about the change.
   *
   * Default implementation returns nullptr.
   *
   * @param old_item_position The position of the item in the old list
   * @param new_item_position The position of the item in the new list
   * @return A payload object that represents the change between the two items.
   */
  virtual void* GetChangePayload(int old_item_position, int new_item_position) const {
    return nullptr;
  }

  virtual ~DiffCallback() = default;
};

/**
 * Template-based ItemCallback for comparing items directly.
 *
 * @tparam T Type of items to compare.
 */
template <typename T>
class ItemCallback {
 public:
  /**
   * Called to check whether two objects represent the same item.
   *
   * @param old_item The item in the old list.
   * @param new_item The item in the new list.
   * @return True if the two items represent the same object or false if they are different.
   */
  virtual bool AreItemsTheSame(const T& old_item, const T& new_item) const = 0;

  /**
   * Called to check whether two items have the same data.
   *
   * @param old_item The item in the old list.
   * @param new_item The item in the new list.
   * @return True if the contents of the items are the same or false if they are different.
   */
  virtual bool AreContentsTheSame(const T& old_item, const T& new_item) const = 0;

  /**
   * When AreItemsTheSame returns true for two items and AreContentsTheSame returns false
   * for them, this method is called to get a payload about the change.
   *
   * @param old_item The item in the old list.
   * @param new_item The item in the new list.
   * @return A payload object that represents the change between the two items.
   */
  virtual void* GetChangePayload(const T& old_item, const T& new_item) const {
    return nullptr;
  }

  virtual ~ItemCallback() = default;
};

}  // namespace pandora

#endif  // PANDORA_DIFF_CALLBACK_H_

