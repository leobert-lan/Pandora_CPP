#ifndef PANDORA_LIST_UPDATE_CALLBACK_H_
#define PANDORA_LIST_UPDATE_CALLBACK_H_

#include <memory>

namespace pandora {

/**
 * An interface that can receive Update operations that are applied to a list.
 *
 * This class can be used together with DiffUtil to detect changes between two lists.
 */
class ListUpdateCallback {
 public:
  /**
   * Called when count number of items are inserted at the given position.
   *
   * @param position The position of the new item.
   * @param count    The number of items that have been added.
   */
  virtual void OnInserted(int position, int count) = 0;

  /**
   * Called when count number of items are removed from the given position.
   *
   * @param position The position of the item which has been removed.
   * @param count    The number of items which have been removed.
   */
  virtual void OnRemoved(int position, int count) = 0;

  /**
   * Called when an item changes its position in the list.
   *
   * @param from_position The previous position of the item before the move.
   * @param to_position   The new position of the item.
   */
  virtual void OnMoved(int from_position, int to_position) = 0;

  /**
   * Called when count number of items are updated at the given position.
   *
   * @param position The position of the item which has been updated.
   * @param count    The number of items which has changed.
   * @param payload  Optional payload object that represents the change.
   */
  virtual void OnChanged(int position, int count, void* payload = nullptr) = 0;

  virtual ~ListUpdateCallback() = default;
};

}  // namespace pandora

#endif  // PANDORA_LIST_UPDATE_CALLBACK_H_

