#ifndef PANDORA_TYPE_VISITOR_H_
#define PANDORA_TYPE_VISITOR_H_

namespace pandora
{
    template <typename T>
    class TypeVisitor
    {
    public:
        TypeVisitor() = default;
        virtual ~TypeVisitor() = default;

        virtual void OnHit(T* element)
        {
        }

        virtual void OnMissed()
        {
        }

        T* Visit(void* element)
        {
            if (!element)
            {
                OnMissed();
                return nullptr;
            }
            if (T* ret = dynamic_cast<T*>(element))
            {
                OnHit(ret);
                return ret;
            }
            else
            {
                OnMissed();
                return nullptr;
            }
        }
    };
} // namespace pandora

#endif  // PANDORA_TYPE_VISITOR_H_
