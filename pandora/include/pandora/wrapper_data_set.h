#ifndef PANDORA_WRAPPER_DATA_SET_H_
#define PANDORA_WRAPPER_DATA_SET_H_

#include "pandora_box_adapter.h"
#include "pandora_traits.h"
#include <vector>
#include <algorithm>
#include <memory>
#include <utility>

#include "diff_util.h"

namespace pandora
{
    template <typename T>
    class WrapperDataSet : public PandoraBoxAdapter<T>
    {
    public:
        WrapperDataSet() : WrapperDataSet(Node<PandoraBoxAdapter<T>>::kNoGroupIndex, 0)
        {
        }

        WrapperDataSet(const int group_index, const int start_index)
            : group_index_(group_index), start_index_(start_index)
        {
        }

        [[nodiscard]] int GetDataCount() const override
        {
            int ret = 0;
            for (const auto& sub : subs_)
            {
                if (sub) ret += sub->GetDataCount();
            }
            return ret;
        }

        T* GetDataByIndex(const int index) override
        {
            int real_index = index + start_index_;
            Log(Logger::VERBOSE, "getDataByResolvedIndex " + std::to_string(index) +
                " ; real index: " + std::to_string(real_index));

            if (index < 0 || index >= GetDataCount())
            {
                return nullptr;
            }

            // Binary search to find target sub adapter
            PandoraBoxAdapter<T>* target_sub = nullptr;

            int start = 0;
            int end = static_cast<int>(subs_.size()) - 1;

            while (start <= end)
            {
                int mid = (end - start) / 2 + start;
                PandoraBoxAdapter<T>* adapter = subs_[mid].get();

                if (real_index < adapter->GetStartIndex())
                {
                    end = mid - 1;
                }
                else if (adapter->GetDataCount() == 0 ||
                    real_index >= (adapter->GetStartIndex() + adapter->GetDataCount()))
                {
                    start = mid + 1;
                }
                else
                {
                    target_sub = adapter;
                    break;
                }
            }

            if (target_sub == nullptr)
            {
                Log(Logger::ERROR, "getDataByRealIndex " + std::to_string(real_index) +
                    "; no child find");
                return nullptr;
            }

            Log(Logger::VERBOSE, "getDataByIndex " + std::to_string(real_index) +
                " " + target_sub->GetAlias() + " - " + std::to_string(reinterpret_cast<uintptr_t>(target_sub)));

            int resolved_index = real_index - target_sub->GetStartIndex();
            return target_sub->GetDataByIndex(resolved_index);
        }

        void ClearAllData() override
        {
            StartTransaction();
            for (auto& sub : subs_)
            {
                if (sub) sub->ClearAllData();
            }
            EndTransaction();
        }

        void ClearAllChildren()
        {
            if (!subs_.empty())
            {
                OnBeforeChanged();
                while (!subs_.empty())
                {
                    auto& sub = subs_.front();
                    sub->NotifyHasRemoveFromParent();
                    subs_.erase(subs_.begin());
                }
                OnAfterChanged();
            }
        }

        int GetChildCount() const
        {
            return static_cast<int>(subs_.size());
        }

        PandoraBoxAdapter<T>* GetChild(int index)
        {
            if (index < 0 || index >= static_cast<int>(subs_.size()))
                return nullptr;
            return subs_[index].get();
        }

        void Add(const T& item) override
        {
            StartTransaction();
            if (!subs_.empty())
            {
                subs_.back()->Add(item);
            }
            EndTransaction();
        }

        void Add(const int pos, const T& item) override
        {
            if (pos < 0) return;

            StartTransaction();
            if (pos >= GetDataCount())
            {
                Add(item);
            }
            else
            {
                auto target = RetrieveAdapterByDataIndex2(pos);
                if (target.first == nullptr)
                {
                    Log(Logger::ERROR, "bug, cannot find target adapter");
                }
                else
                {
                    target.first->Add(target.second, item);
                }
            }
            EndTransaction();
        }

        void AddAll(const std::vector<T>& collection) override
        {
            StartTransaction();
            if (!subs_.empty())
            {
                subs_.back()->AddAll(collection);
            }
            EndTransaction();
        }

        void Remove(const T& item) override
        {
            StartTransaction();
            for (auto& sub : subs_)
            {
                if (sub) sub->Remove(item);
            }
            EndTransaction();
        }

        void RemoveAtPos(const int position) override
        {
            StartTransaction();
            if (position < 0 || position >= GetDataCount())
            {
                Log(Logger::ERROR, "index out of boundary");
            }
            else
            {
                auto target = RetrieveAdapterByDataIndex2(position);
                if (target.first == nullptr)
                {
                    Log(Logger::ERROR, "bug, cannot find target adapter");
                }
                else
                {
                    target.first->RemoveAtPos(target.second);
                }
            }
            EndTransaction();
        }

        bool ReplaceAtPosIfExist(const int position, const T& item) override
        {
            if (position < 0 || position >= GetDataCount()) return false;

            StartTransaction();
            auto target = RetrieveAdapterByDataIndex2(position);
            bool result = false;
            if (target.first == nullptr)
            {
                Log(Logger::ERROR, "bug, cannot find target adapter");
            }
            else
            {
                result = target.first->ReplaceAtPosIfExist(target.second, item);
            }
            EndTransaction();
            return result;
        }

        void SetData(const std::vector<T>& collection) override
        {
            Log(Logger::WARN, "setData: WrapperDataSet does not support this operation");
        }

        int IndexOf(const T& item) const override
        {
            int index = -1;
            for (const auto& sub : subs_)
            {
                if (!sub) continue;
                int i = sub->IndexOf(item);
                if (i >= 0)
                {
                    index = sub->GetStartIndex() + i;
                    break;
                }
            }
            if (index == -1)
                return -1;
            return index - start_index_;
        }

        void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) override
        {
            if (!sub) return;

            if (sub->HasBindToParent())
            {
                sub->RemoveFromOriginalParent();
            }

            OnBeforeChanged();

            int group_index = static_cast<int>(subs_.size());
            sub->SetGroupIndex(group_index);

            int count = GetDataCount();
            sub->SetStartIndex(count);

            // Store raw pointer before moving
            PandoraBoxAdapter<T>* sub_ptr = sub.get();
            sub_ptr->NotifyHasAddToParent(this);

            subs_.push_back(std::move(sub));

            OnAfterChanged();
        }

        void RemoveChild(PandoraBoxAdapter<T>* sub) override
        {
            auto it = std::find_if(subs_.begin(), subs_.end(),
                                   [sub](const std::unique_ptr<PandoraBoxAdapter<T>>& ptr)
                                   {
                                       return ptr.get() == sub;
                                   });

            if (it != subs_.end())
            {
                OnBeforeChanged();
                subs_.erase(it);
                sub->NotifyHasRemoveFromParent(); // make sure
                OnAfterChanged();
            }
        }

        [[nodiscard]] int GetGroupIndex() const override { return group_index_; }

        void SetGroupIndex(const int group_index) override { group_index_ = group_index; }

        [[nodiscard]] bool HasBindToParent() const override
        {
            return parent_ != nullptr;
        }

        void RemoveFromOriginalParent() override
        {
            if (parent_ != nullptr)
            {
                parent_->RemoveChild(this);
                parent_ = nullptr;
            }
        }

        void NotifyHasAddToParent(PandoraBoxAdapter<T>* parent) override
        {
            parent_ = parent;
        }

        void NotifyHasRemoveFromParent() override
        {
            parent_ = nullptr;
        }

        [[nodiscard]] int GetStartIndex() const override { return start_index_; }

        void SetStartIndex(const int start_index) override { start_index_ = start_index; }

        // Get parent
        PandoraBoxAdapter<T>* GetParent() override { return parent_; }

        // Alias support
        PandoraBoxAdapter<T>* FindByAlias(const std::string& target_alias) override
        {
            if (target_alias.empty()) return nullptr;
            if (this->GetAlias() == target_alias) return this;

            // Search in children
            for (auto& sub : subs_)
            {
                if (sub)
                {
                    PandoraBoxAdapter<T>* result = sub->FindByAlias(target_alias);
                    if (result) return result;
                }
            }
            return nullptr;
        }

        bool IsAliasConflict(const std::string& alias) override
        {
            if (this->GetAlias() == alias) return true;

            // Check in children
            for (auto& sub : subs_)
            {
                if (sub && sub->IsAliasConflict(alias))
                {
                    return true;
                }
            }
            return false;
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
            CalcChangeAndNotify();
        }

        void EndTransactionSilently() override
        {
            use_transaction_ = false;
            // Propagate to children without notifying changes
            for (auto& sub : subs_)
            {
                if (sub) sub->EndTransactionSilently();
            }
        }

        // Retrieve adapter by data index (public wrapper)
        PandoraBoxAdapter<T>* RetrieveAdapterByDataIndex(const int index) override
        {
            auto temp = RetrieveAdapterByDataIndex2(index);
            if (temp.first == nullptr)
                return nullptr;
            return temp.first;
        }

        // Retrieve adapter and resolved index by data index
        std::pair<PandoraBoxAdapter<T>*, int> RetrieveAdapterByDataIndex2(const int index) override
        {
            int real_index = GetStartIndex() + index;
            if (start_index_ <= real_index && start_index_ + GetDataCount() > real_index)
            {
                // Find the sub adapter
                PandoraBoxAdapter<T>* target_sub = nullptr;

                if (subs_.empty())
                {
                    return {nullptr, -1};
                }

                int mid = static_cast<int>(subs_.size()) / 2;
                PandoraBoxAdapter<T>* sub = subs_[mid].get();
                if (real_index >= sub->GetStartIndex() && real_index < (sub->GetStartIndex() + sub->GetDataCount()))
                {
                    target_sub = sub;
                }

                int start = 0;
                int end = static_cast<int>(subs_.size()) - 1;
                while (start <= end)
                {
                    mid = (end - start) / 2 + start;
                    PandoraBoxAdapter<T>* adapter = subs_[mid].get();

                    if (real_index < adapter->GetStartIndex())
                    {
                        end = mid - 1;
                    }
                    else if (adapter->GetDataCount() == 0 || real_index >= (adapter->GetStartIndex() + adapter->GetDataCount()))
                    {
                        start = mid + 1;
                    }
                    else
                    {
                        target_sub = adapter;
                        break;
                    }
                }

                if (target_sub == nullptr)
                    return {nullptr, -1};

                int resolved_index = real_index - target_sub->GetStartIndex();
                return target_sub->RetrieveAdapterByDataIndex2(resolved_index);
            }
            return {nullptr, -1};
        }

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

        void OnAfterChanged() override
        {
            RebuildSubNodes();
            if (parent_)
            {
                parent_->OnAfterChanged();
            }
            if (!InTransaction())
            {
                CalcChangeAndNotify();
            }
        }

        void Restore() override
        {
            // Restore all children
            for (auto& sub : subs_)
            {
                if (sub) sub->Restore();
            }
        }

        void RebuildSubNodes() override
        {
            const int sub_counts = static_cast<int>(subs_.size());
            int offset = 0;
            for (int i = 0; i < sub_counts; i++)
            {
                auto& sub = subs_[i];
                if (!sub) continue;

                sub->SetGroupIndex(i);
                sub->SetStartIndex(GetStartIndex() + offset);
                sub->RebuildSubNodes();
                offset += sub->GetDataCount();
            }
        }

        bool InTransaction() const override
        {
            return use_transaction_ || IsParentInTransaction();
        }


    private:
        [[nodiscard]] bool IsParentInTransaction() const
        {
            return parent_ != nullptr && parent_->InTransaction();
        }

        // Calculate changes and notify observers
        void CalcChangeAndNotify()
        {
            if (auto callback = PandoraBoxAdapter<T>::GetListUpdateCallback())
            {
                DiffCallbackImpl diff_callback(this, old_data_, old_data_hashes_);
                const auto result = DiffUtil::CalculateDiff(&diff_callback);
                if (result)
                {
                    if (auto ref = result.get()) ref->DispatchUpdatesTo(callback);
                }
            }
        }

        // Snapshot current state (for transaction support)
        void Snapshot()
        {
            old_data_.clear();
            old_data_hashes_.clear();
            const auto count = GetDataCount();
            for (int i = 0; i < count; ++i)
            {
                auto data = GetDataByIndex(i);
                old_data_.push_back(data);
                if (data) {
                    old_data_hashes_.push_back(Pandora::Hash(*data));
                } else {
                    old_data_hashes_.push_back(0);
                }
            }
        }

        // Dump debug information
        void Dump(std::vector<T>& target) const
        {
            int count = GetDataCount();
            for (int i = 0; i < count; i++)
            {
                T* data = const_cast<WrapperDataSet<T>*>(this)->GetDataByIndex(i);
                if (data)
                {
                    target.push_back(*data);
                }
            }
        }

        // Log helper method
        void Log(const Logger::Level level, const std::string& message) const
        {
            Logger::Println(level, "WrapperDataSet", message);
        }

        std::vector<std::unique_ptr<PandoraBoxAdapter<T>>> subs_;
        std::vector<T*> old_data_; // Snapshot for transaction rollback
        std::vector<size_t> old_data_hashes_; // Snapshot of content hashes
        bool use_transaction_ = false;
        int group_index_ = Node<PandoraBoxAdapter<T>>::kNoGroupIndex;
        int start_index_ = 0;
        PandoraBoxAdapter<T>* parent_ = nullptr;

        // DiffCallback implementation for change detection
        class DiffCallbackImpl : public DiffCallback {
        private:
            WrapperDataSet<T>* dataset_;
            const std::vector<T*>& old_list_;
            const std::vector<size_t>& old_hashes_;

        public:
            DiffCallbackImpl(WrapperDataSet<T>* dataset,
                           const std::vector<T*>& old_list,
                           const std::vector<size_t>& old_hashes)
                : dataset_(dataset), old_list_(old_list), old_hashes_(old_hashes) {}

            int GetOldListSize() const override {
                return static_cast<int>(old_list_.size());
            }

            int GetNewListSize() const override {
                return dataset_->GetDataCount();
            }

            bool AreItemsTheSame(int old_item_position, int new_item_position) const override {
                if (old_item_position >= static_cast<int>(old_list_.size())) return false;
                if (new_item_position >= dataset_->GetDataCount()) return false;

                T* old_item = old_list_[old_item_position];
                T* new_item = dataset_->GetDataByIndex(new_item_position);

                return Pandora::Equals(old_item, new_item);
            }

            bool AreContentsTheSame(int old_item_position, int new_item_position) const override {
                if (old_item_position >= static_cast<int>(old_list_.size())) return false;
                if (new_item_position >= dataset_->GetDataCount()) return false;

                T* old_item = old_list_[old_item_position];
                T* new_item = dataset_->GetDataByIndex(new_item_position);

                // First check if items are the same
                bool items_same = Pandora::Equals(old_item, new_item);
                if (!items_same) return false;

                // Then check if content hash matches
                if (new_item == nullptr) return true;
                
                size_t new_hash = Pandora::Hash(*new_item);
                return old_hashes_[old_item_position] == new_hash;
            }
        };
    };
} // namespace pandora

#endif  // PANDORA_WRAPPER_DATA_SET_H_
