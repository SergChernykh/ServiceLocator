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

int main()
{
    auto services = std::make_shared<sl::Context>();

    static_assert (std::is_same_v<IBar, sl::detail::traits::remove_pointer_t<std::shared_ptr<IBar>>>, "");

    std::cout << "is a pointer: " << std::is_pointer_v<std::shared_ptr<IBar>> << std::endl;
    std::cout << "is a pointer: " << sl::detail::traits::is_pointer_v<std::shared_ptr<IBar>> << std::endl;


    services->addItem(Foo {});
    services->addItemAs<std::shared_ptr<IBar>>(std::make_shared<Bar>());
    services->addItemAs<IBar>(Bar {});

    Foo* foo1 = new Foo();
    services->addItem(foo1);

    if (auto fooItem = services->resolve<Foo>())
    {
        const Foo& foo = fooItem.value();
        std::cout << foo.name() << std::endl;
    }

    if (auto fooItem = services->resolve<Foo*>())
    {
        Foo* foo = fooItem;
        std::cout << foo->name() << std::endl;
    }

    if (auto fooItem = services->resolve<std::shared_ptr<IBar>>())
    {
        std::shared_ptr<IBar> foo = fooItem;
        std::cout << foo->name() << std::endl;
        foo->setName("new bar");
    }

    if (auto fooItem = services->resolve<IBar>())
    {
        const IBar& foo = fooItem.value();
        std::cout << foo.name() << std::endl;
    }

    std::cout << "closing app" << std::endl;

    return 0;
}
