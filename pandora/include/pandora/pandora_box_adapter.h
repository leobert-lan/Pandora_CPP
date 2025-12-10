#ifndef PANDORA_BOX_ADAPTER_H_
#define PANDORA_BOX_ADAPTER_H_

#include "node.h"
#include "data_adapter.h"
#include "pandora_exception.h"
#include "logger.h"
#include <string>
#include <functional>

#include "list_update_callback.h"

namespace pandora
{
    template <typename T>
    class PandoraBoxAdapter : public Node<PandoraBoxAdapter<T>>, public DataAdapter<T>
    {
    public:
        using Consumer = std::function<void(const T&)>;
        PandoraBoxAdapter() = default;
        ~PandoraBoxAdapter() override = default;

        [[nodiscard]] int GetDataCount() const override = 0;
        T* GetDataByIndex(int index) override = 0;
        void ClearAllData() override = 0;
        void Add(const T& item) override = 0;
        void Add(int pos, const T& item) override = 0;
        void AddAll(const std::vector<T>& collection) override = 0;
        void Remove(const T& item) override = 0;
        void RemoveAtPos(int position) override = 0;
        bool ReplaceAtPosIfExist(int position, const T& item) override = 0;
        void SetData(const std::vector<T>& collection) override = 0;
        int IndexOf(const T& item) const override = 0;

        void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) override = 0;
        void RemoveChild(PandoraBoxAdapter<T>* sub) override = 0;

        virtual void RunForeach(const Consumer& action)
        {
            const int count = GetDataCount();
            for (int i = 0; i < count; ++i)
            {
                try
                {
                    action(*GetDataByIndex(i));
                }
                catch (...)
                {
                    Logger::Println(Logger::ERROR, "PandoraBoxAdapter", "Exception in RunForeach");
                }
            }
        }

        /// 设置别名，冲突检测全树检测
        void SetAlias(const std::string& alias)
        {
            // Find root adapter
            PandoraBoxAdapter<T>* check_root = this;
            while (check_root->HasBindToParent())
            {
                PandoraBoxAdapter<T>* parent = check_root->GetParent();
                if (!parent) break;
                check_root = parent;
            }

            // Check alias uniqueness in the tree
            if (check_root->IsAliasConflict(alias))
            {
                throw PandoraException("Alias conflict: " + alias);
            }
            alias_ = alias;
        }

        [[nodiscard]] std::string GetAlias() const { return alias_; }

        // Find adapter by alias in this tree
        virtual PandoraBoxAdapter<T>* FindByAlias(const std::string& target_alias) = 0;

        virtual PandoraBoxAdapter<T>* RetrieveAdapterByDataIndex(int index) =0;
        virtual std::pair<PandoraBoxAdapter<T>*, int> RetrieveAdapterByDataIndex2(int index) =0;

        // Check if alias conflicts with any node in this tree
        virtual bool IsAliasConflict(const std::string& alias) = 0;

        // Index and group management
        [[nodiscard]] virtual int GetStartIndex() const = 0;
        virtual void SetStartIndex(int start_index) = 0;
        virtual void SetGroupIndex(int group_index) = 0;

        // Parent-child relationship notifications
        virtual void NotifyHasAddToParent(PandoraBoxAdapter<T>* parent) = 0;
        virtual void NotifyHasRemoveFromParent() = 0;

        // Get parent adapter
        virtual PandoraBoxAdapter<T>* GetParent() = 0;

        // Transaction support
        virtual void StartTransaction() = 0;
        virtual void EndTransaction() = 0;
        virtual void EndTransactionSilently() = 0;

    public:
        // Hook methods for data changes
        virtual void OnBeforeChanged() = 0;
        virtual void RebuildSubNodes() = 0;
        virtual void OnAfterChanged() = 0;
        [[nodiscard]] virtual bool InTransaction() const = 0;
        virtual void Restore() = 0;

        [[nodiscard]] ListUpdateCallback* GetListUpdateCallback() const
        {
            return listUpdateCallback.get();
        }

        void SetListUpdateCallback(std::unique_ptr<ListUpdateCallback> list_update_callback)
        {
            listUpdateCallback = std::move(list_update_callback);
        }

    private:
        std::string alias_;
        std::unique_ptr<ListUpdateCallback> listUpdateCallback;
    };
} // namespace pandora

#endif  // PANDORA_BOX_ADAPTER_H_
