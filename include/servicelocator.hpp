#pragma once

#include <any>
#include <optional>
#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <memory>
#include <mutex>

//TODO: add asserts
//TODO: refactore
//TODO: implement like: context->item<Class>().as<Interface>().value(initial).add();

namespace sl
{

namespace detail
{

namespace traits
{

template<typename T, typename = std::void_t<>>
struct is_pointer_t : public std::false_type {};

template<typename T>
using pointer_t = decltype (*std::declval<T>());

template<typename T>
struct is_pointer_t<T, std::void_t<pointer_t<T>>> : public std::true_type {};

template<typename T>
inline constexpr bool is_pointer_v = is_pointer_t<T>::value;

template<typename T>
class remove_pointer
{
    template<typename U = T>
    static auto test(int) -> std::remove_reference<pointer_t<U>>;
    static auto test(...) -> std::remove_cv<T>;

public:
    using type = typename decltype(test(0))::type;
};

template<typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

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

        if constexpr (traits::is_pointer_v<From> && traits::is_pointer_v<To>)
        {
            return pointerCast<To>(itemFrom);
        }
        else
        {
            return static_cast<To*>(itemFrom);
        }
    }

private:

    template<typename CastTo, typename CastFrom>
    auto pointerCast(const CastFrom& value)
    {
        using UnderlyingTo = traits::remove_pointer_t<CastTo>;

        if constexpr(std::is_same_v<To, std::shared_ptr<UnderlyingTo>>)
        {
            return std::static_pointer_cast<UnderlyingTo>(*value);
        }
        else
        {
            return static_cast<std::remove_reference_t<CastTo>>(value);
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

        if constexpr (traits::is_pointer_v<T>)
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

        if constexpr (traits::is_pointer_v<T>)
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
    void addItem(T&& initial)
    {
        addItemAs<T>(std::forward<T>(initial));
    }

    template<typename Interface, typename T>
    void addItemAs(T&& initial)
    {
        static_assert (std::is_base_of_v<traits::remove_pointer_t<Interface>, traits::remove_pointer_t<T>>, "T must be derived from Interface type");
        static_assert (std::is_same_v<T, void>, "");

        auto resolver = detail::makeResolver<T, Interface>();
        auto value = std::make_any<T>(std::forward<T>(initial));
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
