#pragma once

#include <any>
#include <optional>
#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <memory>
#include <mutex>

namespace sl
{

namespace detail
{

namespace traits
{

template<typename T>
struct is_shared_ptr : public std::false_type {};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : public std::true_type {};

template<typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

}

using IResolverPtr = std::unique_ptr<class IResolver>;

class IResolver
{
public:
    virtual ~IResolver() = default;
    virtual std::any resolve(std::any* item) = 0;
};

template<typename From, typename To>
class TResolver : public IResolver
{
public:
    std::any resolve(std::any *item) override
    {
        auto itemFrom = std::any_cast<From>(item);
        if (!itemFrom)
        {
            return {};
        }

        if constexpr (traits::is_shared_ptr_v<To>)
        {
            static_assert (traits::is_shared_ptr_v<From>, "From type must be shared_ptr");

            return std::static_pointer_cast<typename To::element_type>(*itemFrom);
        }
        else
        {
            return static_cast<To*>(itemFrom);
        }
    }
};

template<typename From, typename To>
inline IResolverPtr makeResolver()
{
    return std::make_unique<TResolver<From, To>>();
}

class Item
{
public:
    Item(IResolverPtr resolver, std::any value)
        : m_resolver(std::move(resolver))
        , m_value(std::move(value))
    {}

    template<typename T>
    auto get()
    {
        auto resolved = m_resolver->resolve(&m_value);

        if constexpr (traits::is_shared_ptr_v<T>)
        {
            return resolved.has_value() ? std::any_cast<T>(resolved)
                                        : nullptr;
        }
        else
        {
            return resolved.has_value() ? std::optional<std::reference_wrapper<T>> { *std::any_cast<T*>(resolved) }
                                        : std::optional<std::reference_wrapper<T>> {};
        }
    }

private:
    IResolverPtr m_resolver;
    std::any m_value;
};

class ContextImpl
{
public:
    template<typename T>
    auto resolve()
    {
        std::lock_guard lock(m_mtx);

        const auto foundIt = m_items.find(std::type_index(typeid (T)));

        if constexpr (traits::is_shared_ptr_v<T>)
        {
            return foundIt != m_items.end() ? foundIt->second->get<T>()
                                            : nullptr;
        }
        else
        {
            return foundIt != m_items.end() ? foundIt->second->get<T>()
                                            : std::optional<std::reference_wrapper<T>> {};
        }
    }

    template<typename T>
    bool mayBeResolved() const
    {
        return m_items.count(std::type_index(typeid (T))) > 0;
    }

    template<typename T>
    void addItem(T&& initial)
    {
        addItemAs<T>(std::forward<T>(initial));
    }

    template<typename T1, typename T2>
    void addItemAs(T2&& initial)
    {
        using Interface = std::decay_t<T1>;
        using T = std::decay_t<T2>;

        if constexpr (traits::is_shared_ptr_v<T>)
        {
            static_assert (traits::is_shared_ptr_v<Interface>, "Interface type must be shared_ptr");
            static_assert (std::is_base_of_v<typename Interface::element_type, typename T::element_type>, "T must be derived from Interface type");
        }
        else
        {
            static_assert (std::is_base_of_v<Interface, T>, "T must be derived from Interface type");
        }

        auto resolver = detail::makeResolver<T, Interface>();
        auto value = std::make_any<T>(std::forward<T2>(initial));
        auto typeIndex = std::type_index(typeid (Interface));

        auto item = std::make_unique<detail::Item>(std::move(resolver), std::move(value));

        {
            std::lock_guard lock(m_mtx);
            m_items.insert({typeIndex, std::move(item)});
        }
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<detail::Item>> m_items;
    std::mutex m_mtx;
};

}

class Context
{
public:
    Context()
        : m_impl(std::make_unique<detail::ContextImpl>())
    {
    }

    template<typename T>
    [[nodiscard]]
    auto resolve()
    {
        return m_impl->resolve<T>();
    }

    template<typename T>
    [[nodiscard]]
    bool mayBeResolved() const
    {
        return m_impl->mayBeResolved<T>();
    }

    template<typename T>
    void addItem(T&& value)
    {
        m_impl->addItem(std::forward<T>(value));
    }

    template<typename Interface, typename T>
    void addItemAs(T&& value)
    {
        m_impl->addItemAs<Interface>(std::forward<T>(value));
    }

private:
    std::unique_ptr<detail::ContextImpl> m_impl;
};

using ContextPtr = std::shared_ptr<Context>;

}
