#ifndef PANDORA_DIFF_UTIL_H_
#define PANDORA_DIFF_UTIL_H_

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

#include "diff_callback.h"
#include "list_update_callback.h"

namespace pandora {

/**
 * DiffUtil is a utility class that calculates the difference between two lists and outputs a
 * list of update operations that converts the first list into the second one.
 *
 * DiffUtil uses Eugene W. Myers's difference algorithm to calculate the minimal number of updates
 * to convert one list into another. Myers's algorithm does not handle items that are moved so
 * DiffUtil runs a second pass on the result to detect items that were moved.
 *
 * This algorithm is optimized for space and uses O(N) space to find the minimal
 * number of addition and removal operations between the two lists. It has O(N + D^2) expected time
 * performance where D is the length of the edit script.
 */
class DiffUtil {
 public:
  // Forward declarations
  class DiffResult;

  /**
   * Snakes represent a match between two lists. It is optionally prefixed or postfixed with an
   * add or remove operation. See the Myers' paper for details.
   */
  struct Snake {
    int x = 0;           // Position in the old list
    int y = 0;           // Position in the new list
    int size = 0;        // Number of matches. Might be 0.
    bool removal = false; // If true, removal from old list followed by size matches
    bool reverse = false; // If true, the addition/removal is at the end of the snake
  };

  /**
   * Represents a range in two lists that needs to be solved.
   */
  struct Range {
    int old_list_start = 0;
    int old_list_end = 0;
    int new_list_start = 0;
    int new_list_end = 0;

    Range() = default;
    Range(int ols, int ole, int nls, int nle)
        : old_list_start(ols), old_list_end(ole),
          new_list_start(nls), new_list_end(nle) {}
  };

  /**
   * This class holds the information about the result of a CalculateDiff call.
   *
   * You can consume the updates in a DiffResult via DispatchUpdatesTo.
   */
  class DiffResult {
   public:
    static constexpr int NO_POSITION = -1;

    // Flag constants
    static constexpr int FLAG_NOT_CHANGED = 1;
    static constexpr int FLAG_CHANGED = FLAG_NOT_CHANGED << 1;
    static constexpr int FLAG_MOVED_CHANGED = FLAG_CHANGED << 1;
    static constexpr int FLAG_MOVED_NOT_CHANGED = FLAG_MOVED_CHANGED << 1;
    static constexpr int FLAG_IGNORE = FLAG_MOVED_NOT_CHANGED << 1;
    static constexpr int FLAG_OFFSET = 5;
    static constexpr int FLAG_MASK = (1 << FLAG_OFFSET) - 1;

    DiffResult(const DiffCallback* callback,
               const std::vector<Snake>& snakes,
               std::vector<int> old_item_statuses,
               std::vector<int> new_item_statuses,
               bool detect_moves);

    /**
     * Given a position in the old list, returns the position in the new list,
     * or NO_POSITION if it was removed.
     */
    int ConvertOldPositionToNew(int old_list_position) const;

    /**
     * Given a position in the new list, returns the position in the old list,
     * or NO_POSITION if it was added.
     */
    int ConvertNewPositionToOld(int new_list_position) const;

    /**
     * Dispatches update operations to the given Callback.
     * These updates are atomic such that the first update call affects every update call that
     * comes after it.
     */
    void DispatchUpdatesTo(ListUpdateCallback* update_callback);

    const std::vector<Snake>& GetSnakes() const { return snakes_; }

   private:
    struct PostponedUpdate {
      int pos_in_owner_list;
      int current_pos;
      bool removal;

      PostponedUpdate(int pos, int cur, bool rem)
          : pos_in_owner_list(pos), current_pos(cur), removal(rem) {}
    };

    void AddRootSnake();
    void FindMatchingItems();
    void FindAddition(int x, int y, int snake_index);
    void FindRemoval(int x, int y, int snake_index);
    bool FindMatchingItem(int x, int y, int snake_index, bool removal);

    void DispatchAdditions(std::vector<PostponedUpdate>& postponed_updates,
                          ListUpdateCallback* update_callback,
                          int start, int count, int global_index);

    void DispatchRemovals(std::vector<PostponedUpdate>& postponed_updates,
                         ListUpdateCallback* update_callback,
                         int start, int count, int global_index);

    static PostponedUpdate* RemovePostponedUpdate(
        std::vector<PostponedUpdate>& updates, int pos, bool removal);

    std::vector<Snake> snakes_;
    std::vector<int> old_item_statuses_;
    std::vector<int> new_item_statuses_;
    const DiffCallback* callback_;
    int old_list_size_;
    int new_list_size_;
    bool detect_moves_;
  };

  /**
   * Calculates the list of update operations that can convert one list into the other one.
   *
   * @param callback The callback that acts as a gateway to the backing list data
   * @return A DiffResult that contains the information about the edit sequence
   */
  static std::unique_ptr<DiffResult> CalculateDiff(const DiffCallback* callback) {
    return CalculateDiff(callback, true);
  }

  /**
   * Calculates the list of update operations that can convert one list into the other one.
   *
   * @param callback The callback that acts as a gateway to the backing list data
   * @param detect_moves True if DiffUtil should try to detect moved items, false otherwise
   * @return A DiffResult that contains the information about the edit sequence
   */
  static std::unique_ptr<DiffResult> CalculateDiff(const DiffCallback* callback,
                                                    bool detect_moves);

 private:
  DiffUtil() = default;  // Utility class, no instances

  static Snake* DiffPartial(const DiffCallback* cb,
                           int start_old, int end_old,
                           int start_new, int end_new,
                           std::vector<int>& forward,
                           std::vector<int>& backward,
                           int k_offset);
};

// ============================================================================
// Implementation
// ============================================================================

inline std::unique_ptr<DiffUtil::DiffResult> DiffUtil::CalculateDiff(
    const DiffCallback* cb, bool detect_moves) {
  const int old_size = cb->GetOldListSize();
  const int new_size = cb->GetNewListSize();

  std::vector<Snake> snakes;
  std::vector<Range> stack;

  stack.push_back(Range(0, old_size, 0, new_size));

  const int max = old_size + new_size + std::abs(old_size - new_size);
  std::vector<int> forward(max * 2, 0);
  std::vector<int> backward(max * 2, 0);

  std::vector<Range> range_pool;

  while (!stack.empty()) {
    Range range = stack.back();
    stack.pop_back();

    Snake* snake = DiffPartial(cb, range.old_list_start, range.old_list_end,
                               range.new_list_start, range.new_list_end,
                               forward, backward, max);

    if (snake != nullptr) {
      if (snake->size > 0) {
        snakes.push_back(*snake);
      }

      // Offset the snake to convert its coordinates from the Range's area to global
      snake->x += range.old_list_start;
      snake->y += range.new_list_start;

      // Add new ranges for left and right
      Range left;
      left.old_list_start = range.old_list_start;
      left.new_list_start = range.new_list_start;

      if (snake->reverse) {
        left.old_list_end = snake->x;
        left.new_list_end = snake->y;
      } else {
        if (snake->removal) {
          left.old_list_end = snake->x - 1;
          left.new_list_end = snake->y;
        } else {
          left.old_list_end = snake->x;
          left.new_list_end = snake->y - 1;
        }
      }
      stack.push_back(left);

      // Re-use range for right
      Range& right = range;
      if (snake->reverse) {
        if (snake->removal) {
          right.old_list_start = snake->x + snake->size + 1;
          right.new_list_start = snake->y + snake->size;
        } else {
          right.old_list_start = snake->x + snake->size;
          right.new_list_start = snake->y + snake->size + 1;
        }
      } else {
        right.old_list_start = snake->x + snake->size;
        right.new_list_start = snake->y + snake->size;
      }
      stack.push_back(right);

      delete snake;
    }
  }

  // Sort snakes
  std::sort(snakes.begin(), snakes.end(), [](const Snake& o1, const Snake& o2) {
    int cmp_x = o1.x - o2.x;
    return cmp_x == 0 ? o1.y < o2.y : cmp_x < 0;
  });

  std::vector<int> old_item_statuses(old_size, 0);
  std::vector<int> new_item_statuses(new_size, 0);

  return std::make_unique<DiffResult>(cb, snakes, std::move(old_item_statuses),
                                      std::move(new_item_statuses), detect_moves);
}

inline DiffUtil::Snake* DiffUtil::DiffPartial(
    const DiffCallback* cb, int start_old, int end_old,
    int start_new, int end_new, std::vector<int>& forward,
    std::vector<int>& backward, int k_offset) {

  const int old_size = end_old - start_old;
  const int new_size = end_new - start_new;

  if (end_old - start_old < 1 || end_new - start_new < 1) {
    return nullptr;
  }

  const int delta = old_size - new_size;
  const int d_limit = (old_size + new_size + 1) / 2;

  std::fill(forward.begin() + k_offset - d_limit - 1,
            forward.begin() + k_offset + d_limit + 1, 0);
  std::fill(backward.begin() + k_offset - d_limit - 1 + delta,
            backward.begin() + k_offset + d_limit + 1 + delta, old_size);

  const bool check_in_fwd = delta % 2 != 0;

  for (int d = 0; d <= d_limit; d++) {
    // Forward pass
    for (int k = -d; k <= d; k += 2) {
      int x;
      bool removal;

      if (k == -d || (k != d && forward[k_offset + k - 1] < forward[k_offset + k + 1])) {
        x = forward[k_offset + k + 1];
        removal = false;
      } else {
        x = forward[k_offset + k - 1] + 1;
        removal = true;
      }

      int y = x - k;

      // Move diagonal as long as items match
      while (x < old_size && y < new_size &&
             cb->AreItemsTheSame(start_old + x, start_new + y)) {
        x++;
        y++;
      }

      forward[k_offset + k] = x;

      if (check_in_fwd && k >= delta - d + 1 && k <= delta + d - 1) {
        if (forward[k_offset + k] >= backward[k_offset + k]) {
          Snake* out_snake = new Snake();
          out_snake->x = backward[k_offset + k];
          out_snake->y = out_snake->x - k;
          out_snake->size = forward[k_offset + k] - backward[k_offset + k];
          out_snake->removal = removal;
          out_snake->reverse = false;
          return out_snake;
        }
      }
    }

    // Backward pass
    for (int k = -d; k <= d; k += 2) {
      const int backward_k = k + delta;
      int x;
      bool removal;

      if (backward_k == d + delta ||
          (backward_k != -d + delta &&
           backward[k_offset + backward_k - 1] < backward[k_offset + backward_k + 1])) {
        x = backward[k_offset + backward_k - 1];
        removal = false;
      } else {
        x = backward[k_offset + backward_k + 1] - 1;
        removal = true;
      }

      int y = x - backward_k;

      // Move diagonal as long as items match
      while (x > 0 && y > 0 &&
             cb->AreItemsTheSame(start_old + x - 1, start_new + y - 1)) {
        x--;
        y--;
      }

      backward[k_offset + backward_k] = x;

      if (!check_in_fwd && k + delta >= -d && k + delta <= d) {
        if (forward[k_offset + backward_k] >= backward[k_offset + backward_k]) {
          Snake* out_snake = new Snake();
          out_snake->x = backward[k_offset + backward_k];
          out_snake->y = out_snake->x - backward_k;
          out_snake->size = forward[k_offset + backward_k] - backward[k_offset + backward_k];
          out_snake->removal = removal;
          out_snake->reverse = true;
          return out_snake;
        }
      }
    }
  }

  throw std::runtime_error(
      "DiffUtil hit an unexpected case while trying to calculate "
      "the optimal path. Please make sure your data is not changing during the "
      "diff calculation.");
}

// DiffResult implementation
inline DiffUtil::DiffResult::DiffResult(
    const DiffCallback* callback,
    const std::vector<Snake>& snakes,
    std::vector<int> old_item_statuses,
    std::vector<int> new_item_statuses,
    bool detect_moves)
    : snakes_(snakes),
      old_item_statuses_(std::move(old_item_statuses)),
      new_item_statuses_(std::move(new_item_statuses)),
      callback_(callback),
      old_list_size_(callback->GetOldListSize()),
      new_list_size_(callback->GetNewListSize()),
      detect_moves_(detect_moves) {
  AddRootSnake();
  FindMatchingItems();
}

inline void DiffUtil::DiffResult::AddRootSnake() {
  Snake* first_snake = snakes_.empty() ? nullptr : &snakes_[0];
  if (first_snake == nullptr || first_snake->x != 0 || first_snake->y != 0) {
    Snake root;
    root.x = 0;
    root.y = 0;
    root.removal = false;
    root.size = 0;
    root.reverse = false;
    snakes_.insert(snakes_.begin(), root);
  }
}

inline void DiffUtil::DiffResult::FindMatchingItems() {
  int pos_old = old_list_size_;
  int pos_new = new_list_size_;

  for (int i = static_cast<int>(snakes_.size()) - 1; i >= 0; i--) {
    const Snake& snake = snakes_[i];
    const int end_x = snake.x + snake.size;
    const int end_y = snake.y + snake.size;

    if (detect_moves_) {
      while (pos_old > end_x) {
        FindAddition(pos_old, pos_new, i);
        pos_old--;
      }
      while (pos_new > end_y) {
        FindRemoval(pos_old, pos_new, i);
        pos_new--;
      }
    }

    for (int j = 0; j < snake.size; j++) {
      const int old_item_pos = snake.x + j;
      const int new_item_pos = snake.y + j;
      const bool the_same = callback_->AreContentsTheSame(old_item_pos, new_item_pos);
      const int change_flag = the_same ? FLAG_NOT_CHANGED : FLAG_CHANGED;
      old_item_statuses_[old_item_pos] = (new_item_pos << FLAG_OFFSET) | change_flag;
      new_item_statuses_[new_item_pos] = (old_item_pos << FLAG_OFFSET) | change_flag;
    }

    pos_old = snake.x;
    pos_new = snake.y;
  }
}

inline void DiffUtil::DiffResult::FindAddition(int x, int y, int snake_index) {
  if (old_item_statuses_[x - 1] != 0) {
    return;
  }
  FindMatchingItem(x, y, snake_index, false);
}

inline void DiffUtil::DiffResult::FindRemoval(int x, int y, int snake_index) {
  if (new_item_statuses_[y - 1] != 0) {
    return;
  }
  FindMatchingItem(x, y, snake_index, true);
}

inline bool DiffUtil::DiffResult::FindMatchingItem(
    int x, int y, int snake_index, bool removal) {
  const int my_item_pos = removal ? y - 1 : x - 1;
  int cur_x = removal ? x : x - 1;
  int cur_y = removal ? y - 1 : y;

  for (int i = snake_index; i >= 0; i--) {
    const Snake& snake = snakes_[i];
    const int end_x = snake.x + snake.size;
    const int end_y = snake.y + snake.size;

    if (removal) {
      for (int pos = cur_x - 1; pos >= end_x; pos--) {
        if (callback_->AreItemsTheSame(pos, my_item_pos)) {
          const bool the_same = callback_->AreContentsTheSame(pos, my_item_pos);
          const int change_flag = the_same ? FLAG_MOVED_NOT_CHANGED : FLAG_MOVED_CHANGED;
          new_item_statuses_[my_item_pos] = (pos << FLAG_OFFSET) | FLAG_IGNORE;
          old_item_statuses_[pos] = (my_item_pos << FLAG_OFFSET) | change_flag;
          return true;
        }
      }
    } else {
      for (int pos = cur_y - 1; pos >= end_y; pos--) {
        if (callback_->AreItemsTheSame(my_item_pos, pos)) {
          const bool the_same = callback_->AreContentsTheSame(my_item_pos, pos);
          const int change_flag = the_same ? FLAG_MOVED_NOT_CHANGED : FLAG_MOVED_CHANGED;
          old_item_statuses_[x - 1] = (pos << FLAG_OFFSET) | FLAG_IGNORE;
          new_item_statuses_[pos] = ((x - 1) << FLAG_OFFSET) | change_flag;
          return true;
        }
      }
    }

    cur_x = snake.x;
    cur_y = snake.y;
  }

  return false;
}

inline int DiffUtil::DiffResult::ConvertOldPositionToNew(int old_list_position) const {
  if (old_list_position < 0 || old_list_position >= old_list_size_) {
    throw std::out_of_range("Index out of bounds - passed position = " +
                           std::to_string(old_list_position) +
                           ", old list size = " + std::to_string(old_list_size_));
  }
  const int status = old_item_statuses_[old_list_position];
  if ((status & FLAG_MASK) == 0) {
    return NO_POSITION;
  }
  return status >> FLAG_OFFSET;
}

inline int DiffUtil::DiffResult::ConvertNewPositionToOld(int new_list_position) const {
  if (new_list_position < 0 || new_list_position >= new_list_size_) {
    throw std::out_of_range("Index out of bounds - passed position = " +
                           std::to_string(new_list_position) +
                           ", new list size = " + std::to_string(new_list_size_));
  }
  const int status = new_item_statuses_[new_list_position];
  if ((status & FLAG_MASK) == 0) {
    return NO_POSITION;
  }
  return status >> FLAG_OFFSET;
}

inline DiffUtil::DiffResult::PostponedUpdate* DiffUtil::DiffResult::RemovePostponedUpdate(
    std::vector<PostponedUpdate>& updates, int pos, bool removal) {
  for (int i = static_cast<int>(updates.size()) - 1; i >= 0; i--) {
    PostponedUpdate& update = updates[i];
    if (update.pos_in_owner_list == pos && update.removal == removal) {
      PostponedUpdate* result = new PostponedUpdate(update);
      updates.erase(updates.begin() + i);
      for (size_t j = i; j < updates.size(); j++) {
        updates[j].current_pos += removal ? 1 : -1;
      }
      return result;
    }
  }
  return nullptr;
}

inline void DiffUtil::DiffResult::DispatchAdditions(
    std::vector<PostponedUpdate>& postponed_updates,
    ListUpdateCallback* update_callback,
    int start, int count, int global_index) {

  if (!detect_moves_) {
    update_callback->OnInserted(start, count);
    return;
  }

  for (int i = count - 1; i >= 0; i--) {
    int status = new_item_statuses_[global_index + i] & FLAG_MASK;
    switch (status) {
      case 0:  // Real addition
        update_callback->OnInserted(start, 1);
        for (auto& update : postponed_updates) {
          update.current_pos += 1;
        }
        break;

      case FLAG_MOVED_CHANGED:
      case FLAG_MOVED_NOT_CHANGED: {
        const int pos = new_item_statuses_[global_index + i] >> FLAG_OFFSET;
        PostponedUpdate* update = RemovePostponedUpdate(postponed_updates, pos, true);
        update_callback->OnMoved(update->current_pos, start);
        if (status == FLAG_MOVED_CHANGED) {
          update_callback->OnChanged(start, 1,
              callback_->GetChangePayload(pos, global_index + i));
        }
        delete update;
        break;
      }

      case FLAG_IGNORE:
        postponed_updates.emplace_back(global_index + i, start, false);
        break;

      default:
        throw std::runtime_error("unknown flag for pos " +
            std::to_string(global_index + i));
    }
  }
}

inline void DiffUtil::DiffResult::DispatchRemovals(
    std::vector<PostponedUpdate>& postponed_updates,
    ListUpdateCallback* update_callback,
    int start, int count, int global_index) {

  if (!detect_moves_) {
    update_callback->OnRemoved(start, count);
    return;
  }

  for (int i = count - 1; i >= 0; i--) {
    const int status = old_item_statuses_[global_index + i] & FLAG_MASK;
    switch (status) {
      case 0:  // Real removal
        update_callback->OnRemoved(start + i, 1);
        for (auto& update : postponed_updates) {
          update.current_pos -= 1;
        }
        break;

      case FLAG_MOVED_CHANGED:
      case FLAG_MOVED_NOT_CHANGED: {
        const int pos = old_item_statuses_[global_index + i] >> FLAG_OFFSET;
        PostponedUpdate* update = RemovePostponedUpdate(postponed_updates, pos, false);
        update_callback->OnMoved(start + i, update->current_pos - 1);
        if (status == FLAG_MOVED_CHANGED) {
          update_callback->OnChanged(update->current_pos - 1, 1,
              callback_->GetChangePayload(global_index + i, pos));
        }
        delete update;
        break;
      }

      case FLAG_IGNORE:
        postponed_updates.emplace_back(global_index + i, start + i, true);
        break;

      default:
        throw std::runtime_error("unknown flag for pos " +
            std::to_string(global_index + i));
    }
  }
}

inline void DiffUtil::DiffResult::DispatchUpdatesTo(ListUpdateCallback* update_callback) {
  std::vector<PostponedUpdate> postponed_updates;
  int pos_old = old_list_size_;
  int pos_new = new_list_size_;

  for (int snake_index = static_cast<int>(snakes_.size()) - 1; snake_index >= 0; snake_index--) {
    const Snake& snake = snakes_[snake_index];
    const int snake_size = snake.size;
    const int end_x = snake.x + snake_size;
    const int end_y = snake.y + snake_size;

    if (end_x < pos_old) {
      DispatchRemovals(postponed_updates, update_callback, end_x, pos_old - end_x, end_x);
    }

    if (end_y < pos_new) {
      DispatchAdditions(postponed_updates, update_callback, end_x, pos_new - end_y, end_y);
    }

    for (int i = snake_size - 1; i >= 0; i--) {
      if ((old_item_statuses_[snake.x + i] & FLAG_MASK) == FLAG_CHANGED) {
        update_callback->OnChanged(snake.x + i, 1,
            callback_->GetChangePayload(snake.x + i, snake.y + i));
      }
    }

    pos_old = snake.x;
    pos_new = snake.y;
  }
}

}  // namespace pandora

#endif  // PANDORA_DIFF_UTIL_H_

