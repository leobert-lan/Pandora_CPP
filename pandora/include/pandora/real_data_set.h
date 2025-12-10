#ifndef PANDORA_REAL_DATA_SET_H_
#define PANDORA_REAL_DATA_SET_H_

#include "pandora_box_adapter.h"
#include <vector>
#include <algorithm>

namespace pandora
{
    template <typename T>
    class RealDataSet final : public PandoraBoxAdapter<T>
    {
    public:
        RealDataSet() = default;
        [[nodiscard]] int GetDataCount() const override { return static_cast<int>(data_.size()); }

        T* GetDataByIndex(int index) override
        {
            if (index < 0 || index >= static_cast<int>(data_.size())) return nullptr;
            return &data_[index];
        }

        void ClearAllData() override
        {
            OnBeforeChanged();
            data_.clear();
            OnAfterChanged();
        }

        void Add(const T& item) override
        {
            OnBeforeChanged();
            data_.push_back(item);
            OnAfterChanged();
        }

        void Add(int pos, const T& item) override
        {
            if (pos < 0 || pos > static_cast<int>(data_.size())) return;
            OnBeforeChanged();
            data_.insert(data_.begin() + pos, item);
            OnAfterChanged();
        }

        void AddAll(const std::vector<T>& collection) override
        {
            OnBeforeChanged();
            data_.insert(data_.end(), collection.begin(), collection.end());
            OnAfterChanged();
        }

        void Remove(const T& item) override
        {
            OnBeforeChanged();
            auto it = std::find(data_.begin(), data_.end(), item);
            if (it != data_.end()) data_.erase(it);
            OnAfterChanged();
        }

        void RemoveAtPos(int position) override
        {
            if (position < 0 || position >= static_cast<int>(data_.size())) return;
            OnBeforeChanged();
            data_.erase(data_.begin() + position);
            OnAfterChanged();
        }

        bool ReplaceAtPosIfExist(int position, const T& item) override
        {
            if (position < 0 || position >= static_cast<int>(data_.size())) return false;
            OnBeforeChanged();
            data_[position] = item;
            OnAfterChanged();
            return true;
        }

        void SetData(const std::vector<T>& collection) override
        {
            OnBeforeChanged();
            data_ = collection;
            OnAfterChanged();
        }

        int IndexOf(const T& item) const override
        {
            auto it = std::find(data_.begin(), data_.end(), item);
            if (it == data_.end()) return -1;
            return static_cast<int>(std::distance(data_.begin(), it));
        }

        // Node interface implementation
        [[nodiscard]] int GetGroupIndex() const override { return group_index_; }
        void SetGroupIndex(int group_index) override { group_index_ = group_index; }

        void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) override
        {
            throw PandoraException("RealDataSet does not support AddChild");
        }

        [[nodiscard]] bool HasBindToParent() const override { return parent_ != nullptr; }

        void RemoveFromOriginalParent() override
        {
            if (parent_)
            {
                parent_->RemoveChild(this);
                parent_ = nullptr;
            }
        }

        void RemoveChild(PandoraBoxAdapter<T>* sub) override
        {
            throw PandoraException("RealDataSet does not support RemoveChild");
        }

        // Index management
        [[nodiscard]] int GetStartIndex() const override { return start_index_; }
        void SetStartIndex(const int start_index) override { start_index_ = start_index; }

        PandoraBoxAdapter<T>* RetrieveAdapterByDataIndex(const int index) override
        {
            if (0 <= index && index < GetDataCount())
                return this;
            return nullptr;
        }

        std::pair<PandoraBoxAdapter<T>*, int> RetrieveAdapterByDataIndex2(int index) override
        {
            auto temp = RetrieveAdapterByDataIndex(index);
            if (temp == nullptr)
                return {nullptr, -1};

            return {temp, index};
        }

        // Parent-child relationship notifications
        void NotifyHasAddToParent(PandoraBoxAdapter<T>* parent) override
        {
            parent_ = parent;
        }

        void NotifyHasRemoveFromParent() override
        {
            parent_ = nullptr;
        }

        // Get parent
        PandoraBoxAdapter<T>* GetParent() override { return parent_; }

        // Alias support
        PandoraBoxAdapter<T>* FindByAlias(const std::string& target_alias) override
        {
            if (target_alias.empty()) return nullptr;
            if (this->GetAlias() == target_alias) return this;
            return nullptr;
        }

        bool IsAliasConflict(const std::string& alias) override
        {
            return this->GetAlias() == alias;
        }

        // Transaction support
        void StartTransaction() override
        {
            use_transaction_ = true;
            Snapshot();
        }

        void EndTransaction() override
        {
            use_transaction_ = false;
            // Notify changes if needed (could add diff calculation here)
        }

        void EndTransactionSilently() override
        {
            use_transaction_ = false;
        }

        [[nodiscard]] bool InTransaction() const override
        {
            return use_transaction_ || IsParentInTransaction();
        }

    protected:
        void OnBeforeChanged() override
        {
            if (!InTransaction())
            {
                Snapshot();
            }
            if (parent_)
            {
                parent_->OnBeforeChanged();
            }
        }

        void RebuildSubNodes() override
        {
        }

        void OnAfterChanged() override
        {
            if (parent_)
            {
                parent_->OnAfterChanged();
            }
            if (!InTransaction())
            {
                // todo calc diff
                // Notify changes (could add diff calculation here)
            }
        }

        void Restore() override
        {
            if (old_data_.size() == 0)
            {
                data_.clear();
            }
            else
            {
                data_.assign(old_data_.begin(), old_data_.end());
            }
        }

    private:
        void Snapshot()
        {
            if (data_.size() == 0)
            {
                old_data_.clear();
            }
            else
            {
                old_data_.assign(data_.begin(), data_.end());
            }
        }

        [[nodiscard]] bool IsParentInTransaction() const
        {
            return parent_ != nullptr && parent_->InTransaction();
        }

        std::vector<T> data_;
        std::vector<T> old_data_; // Snapshot for transaction rollback
        bool use_transaction_ = false;
        int group_index_ = Node<PandoraBoxAdapter<T>>::kNoGroupIndex;
        int start_index_ = 0;
        PandoraBoxAdapter<T>* parent_ = nullptr;
    };
} // namespace pandora

#endif  // PANDORA_REAL_DATA_SET_H_
