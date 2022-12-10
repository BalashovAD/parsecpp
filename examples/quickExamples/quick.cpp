#include <parsecpp/all.h>

#include <iostream>


using namespace prs;


void hello() {
    auto parser = literal("Hello") >> spaces() >> letters();

    Stream helloExample{"Hello Username"};
    std::cout << "Name is " << parser(helloExample).join([](std::string_view name) {
        return name;
    }, [](details::ParsingError const& error) {
        return "error";
    }) << std::endl;
}


struct Point {
    int x;
    int y;
};


class Circle {
public:
    Circle(Point center, double r) noexcept {

    }
    /// ...
};

void circle() {
    auto pointParser = between('(', ')',
               liftM(details::MakeClass<Point>{}, number<int>(), charIn(';') >> number<int>()));
    auto parser = literal("Circle") >> spaces() >>
            liftM(details::MakeClass<Circle>{}, pointParser, spaces() >> number<double>());

    Stream example{"Circle (1;-3) 4.5"};
    parser(example).join([](Circle circle) {
        std::cout << "Circle parsed" << std::endl;
    }, [&example](auto const& error) {
        std::cout << "Error: " << example.generateErrorText(error) << std::endl;
    });
}


Parser<char> makeB();

auto makeA() {
    return charIn('A') >> makeB();
}

Parser<char> makeB() {
    return (charIn('B') | lazy(makeA)).toCommonType();
}

void recursion() {
    auto parser = makeA();
    Stream example("AAAAB");
    parser(example).join([](char c) {
        std::cout << "Lazy parsed" << std::endl;
    }, [&example](auto const& error) {
        std::cout << "Error: " << example.generateErrorText(error) << std::endl;
    });
}

int main() {

    hello();
    circle();
    recursion();

    return 0;
}