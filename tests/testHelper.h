#pragma once

#include <gtest/gtest.h>

#include <parsecpp/full.hpp>

#include <concepts>

using namespace prs;

template<typename T>
concept printable = requires(T t) {
    { std::cout << t } -> std::same_as<std::ostream&>;
};

template <typename T>
void printError(typename Parser<T>::Result const& result, Stream const& stream) {
    result.join([&](T const& t) {
        if constexpr (printable<T>) {
            std::cout << "Error parse: '" << stream.full() << "', r: '" << stream.remaining() << "'"
                    << ", data: '" << t << "', type: " << typeid(T).name() << std::endl;
        } else {
            std::cout << "Error parse: '" << stream.full() << "', r: '" << stream.remaining() << "'"
                    << ", type: " << typeid(T).name() << std::endl;
        }
    }, [&](details::ParsingError const& error) {
        std::cout << "Cannot parse: " << stream.full()
                << ", error: " << stream.generateErrorText(error) << std::endl;
    });
}


template <typename Parser, typename Ctx = VoidContext>
void success_parsing(Parser parser,
        GetParserResult<Parser> const& answer,
        std::string const& str,
        std::string_view remaining = "",
        Ctx& ctx = VOID_CONTEXT,
        details::SourceLocation sourceLocation = details::SourceLocation::current()) {
    Stream s{str};
    auto result = parser.apply(s, ctx);

    if (result.isError()) {
        printError<typename Parser::Type>(result, s);
    }
    ASSERT_FALSE(result.isError()) << "Test: " << sourceLocation.prettyPrint();
    EXPECT_EQ(result.data(), answer) << "Test: " << sourceLocation.prettyPrint();
    EXPECT_EQ(s.remaining(), remaining) << "Test: " << sourceLocation.prettyPrint();
}

template <typename Parser, typename Ctx = VoidContext>
void expect_throw(Parser parser,
        std::string const& str,
        std::string_view text = "",
        Ctx& ctx = VOID_CONTEXT,
        details::SourceLocation sourceLocation = details::SourceLocation::current()) {
    Stream s{str};

    try {
        auto result = parser.apply(s, ctx);
        printError<typename Parser::Type>(result, s);
        FAIL() << "No exception. Test: " << sourceLocation.prettyPrint();
    } catch (std::exception const& e) {
        if (!text.empty()) {
            EXPECT_EQ(e.what(), text) << "Test: " << sourceLocation.prettyPrint();
        }
    }
}

template <typename Parser, typename Ctx = VoidContext>
void failed_parsing(Parser parser,
                    size_t pos,
                    std::string_view str,
                    Ctx& ctx = VOID_CONTEXT,
                    details::SourceLocation sourceLocation = details::SourceLocation::current()) {
    Stream s{str};
    auto result = parser.apply(s, ctx);

    if (!result.isError()) {
        printError<typename Parser::Type>(result, s);
    }
    ASSERT_TRUE(result.isError()) << "Test: " << sourceLocation.prettyPrint();
    EXPECT_EQ(result.error().pos, pos) << "Test: " << sourceLocation.prettyPrint();
}


template <size_t N>
struct uniqCtxName{};

template <size_t N = 0>
auto addContext() {
    using Ctx = ContextWrapper<details::NamedType<unsigned, uniqCtxName<N>>>;
    auto parser = Parser<Unit, Ctx>::make([](Stream&, auto& ctx) {
        return Parser<Unit, Ctx>::data({});
    });
    if constexpr (N == 0) {
        return parser;
    } else {
        return addContext<N - 1>() >> parser;
    }
}


inline auto exceptionParser(std::string const& text = "t") {
    return Parser<Unit>::make([text](Stream&) -> Parser<Unit>::Result {
        throw std::runtime_error(text);
    });
}