#include <parsecpp/all.hpp>

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
               liftM(details::MakeClass<Point>{}, number<int>(), charFrom(';') >> number<int>()));
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
    return charFrom('A') >> makeB();
}

Parser<char> makeB() {
    return (charFrom('B') | lazy(makeA)).toCommonType();
}

static constexpr auto parserA = charFrom('A');
static constexpr auto parserB = charFrom('B');

Parser<char> makeRecursive() noexcept {
    return (parserB | (parserA >> lazy(makeRecursive))).toCommonType();
}


void recursion() {
    {
        auto parser = makeRecursive();
        Stream example("AAAAB");
        parser(example).join([](char c) {
            std::cout << "Lazy parsed" << std::endl;
        }, [&example](auto const& error) {
            std::cout << "Error: " << example.generateErrorText(error) << std::endl;
        });
    }
    {
        auto parser = makeA();
        Stream example("AAAAB");
        parser(example).join([](char c) {
            std::cout << "Lazy one fn parsed" << std::endl;
        }, [&example](auto const& error) {
            std::cout << "Error: " << example.generateErrorText(error) << std::endl;
        });
    }
}

void contextSimple() {
    using Ctx = ContextWrapper<char const>;
    const auto parser = make_parser<Ctx>([](Stream& stream, Ctx& ctx) {
        return charFrom(get<char>(ctx)).apply(stream);
    });

    char const c = 'v';
    Ctx ctx{c};
    Stream example("vvv");
    parser(example, ctx).join([](char c) {
        std::cout << "Context parsed" << std::endl;
    }, [&example](auto const& error) {
        std::cout << "Error: " << example.generateErrorText(error) << std::endl;
    });
}

void filter() {
    using Ctx = ContextWrapper<unsigned const>;
    const auto parser = skipToNext(concat(number<unsigned>().condC<Ctx>([](unsigned n, Ctx& ctx) {
        return n >= ctx.get();
    }), literal("-") >> until('\n') << charFrom('\n')), searchText("\n")).repeat().cond([](auto const& vector) {
        return vector.size() == 2;
    });


    unsigned border = 4;
    Ctx ctx{border};
    Stream example{R"(1-a
3-c
11-s
4-dd
)"};
    parser(example, ctx).join([](std::vector<std::tuple<unsigned, std::string_view>> const& result) {
        std::cout << "Filter parsed: "
                << get<0>(result[0]) << get<1>(result[0]) << "|"
                << get<0>(result[1]) << get<1>(result[1]) << std::endl;
    }, [&example](auto const& error) {
        std::cout << "Error: " << example.generateErrorText(error) << std::endl;
    });
}

template <typename T, typename Allocator>
struct MakeWithAllocator {
    using Ctx = ContextWrapper<Allocator&>;

    template <typename ...Args>
    auto operator()(Args &&...args, Ctx& ctx) const {
        return ctx.get().template alloc<T>(std::forward<Args>(args)...);
    }
};

struct CountSumRepeat : public Repeat<CountSumRepeat, double, VoidContext> {
    void add(Container& sum, std::tuple<double, double> priceAndQty) const noexcept {
        sum += get<0>(priceAndQty) * get<1>(priceAndQty);
    }
};

void countSum() noexcept {
    auto parser = (concat(charFrom('(') >> number(), charFrom(';') >> number() << charFrom(')')) * CountSumRepeat{}).endOfStream();
    Stream example("(1.5;3)(2;1)(0;3)");
    parser(example).join([](double sum) {
        std::cout << "Count sum: " << sum << std::endl;
    }, [&example](auto const& error) {
        std::cout << "Error: " << example.generateErrorText(error) << std::endl;
    });
}


int main() {

    hello();
    circle();
    recursion();
    contextSimple();
    filter();
    countSum();

    return 0;
}