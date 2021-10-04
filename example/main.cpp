#include <iostream>

#include "servicelocator.hpp"

#include <iostream>

class Foo
{
public:
    Foo()
    {
        std::cout << __FUNCTION__ << std::endl;
    }
    ~Foo()
    {
        std::cout << __FUNCTION__ << std::endl;
    }
    Foo(const Foo& other)
    {
        std::cout << __FUNCTION__ << " copy" << std::endl;
    }
    Foo(Foo&& other)
    {
        std::cout << __FUNCTION__ << " move" << std::endl;
    }

    std::string name() const
    {
        return "foo";
    }
};

class IBar
{
public:
    virtual ~IBar() = default;
    virtual std::string name() const = 0;
    virtual void setName(const std::string& value) = 0;
};

class Bar : public IBar
{
public:
    Bar()
    {
        std::cout << __FUNCTION__ << std::endl;
    }
    ~Bar()
    {
        std::cout << __FUNCTION__ << std::endl;
    }
    Bar(const Bar& other)
    {
        std::cout << __FUNCTION__ << " copy" << std::endl;
    }
    Bar(Bar&& other)
    {
        std::cout << __FUNCTION__ << " move" << std::endl;
    }

    std::string name() const override
    {
        return m_name;
    }
    void setName(const std::string &value) override
    {
        m_name = value;
    }

private:
    std::string m_name {"bar"};
};

template<typename T>
void checkResolved(sl::ContextPtr ctx, std::string_view name)
{
    std::cout << std::boolalpha << "may be resolved by " << name << " : " << ctx->mayBeResolved<T>() << std::endl;
}

#define CHECK_RESOLVED(ctx, type) \
    checkResolved<type>(ctx, #type);

int main()
{
    auto context = std::make_shared<sl::Context>();

    Foo newFooItem{};

    context->addItem(newFooItem);
    auto barItem = std::make_shared<Bar>();
    context->addItemAs<std::shared_ptr<IBar>>(barItem);
    context->addItemAs<IBar>(Bar {});

    CHECK_RESOLVED(context, Bar);
    CHECK_RESOLVED(context, IBar);
    CHECK_RESOLVED(context, Foo);
    CHECK_RESOLVED(context, std::shared_ptr<IBar>);

    if (auto fooItem = context->resolve<Foo>())
    {
        const Foo& foo = fooItem.value();
        std::cout << "[resolve as ref] " << foo.name() << std::endl;
    }

    barItem->setName("bar item");

    if (auto fooItem = context->resolve<std::shared_ptr<IBar>>())
    {
        std::shared_ptr<IBar> foo = fooItem;
        std::cout << "[resolve as shared ptr by interface] " << foo->name() << std::endl;
    }

    if (auto fooItem = context->resolve<IBar>())
    {
        const IBar& foo = fooItem.value();
        std::cout << "[resolve as ref by interface] " << foo.name() << std::endl;
    }

    std::cout << "closing app" << std::endl;

    return 0;
}
