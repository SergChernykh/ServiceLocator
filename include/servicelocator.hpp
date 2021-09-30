#pragma once

#include <any>
#include <optional>
#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <memory>
#include <iostream>

//TODO: add asserts
//TODO: remove debug logs
//TODO: add mutex
//TODO: refactore
//TODO: implement like: context->item<Class>().as<Interface>().value(initial).add();
//TODO: что если тип для хранения является shared_ptr, да и вообще указателем. UPD: сделал реализацию, работает, нужно поддержать просто указатели и зарефакторить

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
struct is_pointer_t<T, std::void_t<pointer_t<T>>> : std::true_type {};

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
    std::any resolve(std::any *item) override
    {
        auto itemFrom = std::any_cast<From>(item);
        if (!itemFrom)
        {
//            std::cout << "item from is nullptr" << std::endl;
            return {};
        }

        if constexpr (traits::is_pointer_v<From> && traits::is_pointer_v<To>) // kostyl for smart pointers
        {
            return std::static_pointer_cast<traits::remove_pointer_t<To>>(*itemFrom); //TODO: вынести в отдельный хелпер
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
        std::cout << "resolved has_value " << resolved.has_value() << " type name " << resolved.type().name() << std::endl;

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

class ServiceLocatorImpl
{
public:
    template<typename T>
    auto resolve()
    {
        auto foundIt = m_items.find(std::type_index(typeid (T)));

        if constexpr (traits::is_pointer_v<T>)
        {
            return foundIt == m_items.end() ? nullptr : foundIt->second->get<T>();
        }
        else
        {
            return foundIt == m_items.end() ? std::optional<std::reference_wrapper<T>> {} : foundIt->second->get<T>();
        }
    }

    template<typename T>
    void addItem(T&& value)
    {
        addItemAs<T>(std::forward<T>(value));
    }

    template<typename As, typename T>
    void addItemAs(T&& value)
    {
        static_assert (std::is_base_of_v<traits::remove_pointer_t<As>, traits::remove_pointer_t<T>>, "T must be derived from As type");

        auto resolver = detail::makeResolver<T, As>();
        auto item = std::make_any<T>(std::forward<T>(value));
        auto typeIndex = std::type_index(typeid (As));

        auto newItem = std::make_unique<detail::Item>(std::move(resolver), std::move(item));

        m_items.insert({typeIndex, std::move(newItem)});
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<detail::Item>> m_items;
};

}

class ServiceLocator
{
public:
    ServiceLocator()
        : m_impl(std::make_unique<detail::ServiceLocatorImpl>())
    {
    }

    template<typename T>
    [[nodiscard]] auto resolve()
    {
        return m_impl->resolve<T>();
    }

    template<typename T>
    void addItem(T&& value)
    {
        m_impl->addItem(std::forward<T>(value));
    }

    template<typename As, typename T>
    void addItemAs(T&& value)
    {
        m_impl->addItemAs<As>(std::forward<T>(value));
    }

private:
    std::unique_ptr<detail::ServiceLocatorImpl> m_impl;
};

using ServiceLocatorPtr = std::shared_ptr<ServiceLocator>;

}
