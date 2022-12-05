#pragma once

#include <gtest/gtest.h>

#include <parsecpp/all.h>

#include <concepts>

using namespace prs;

template<typename T>
concept printable = requires(T t) {
    { std::cout << t } -> std::same_as<std::ostream&>;
};

template <typename T>
void printError(typename Parser<T>::Result const& result, Stream const& stream) {
    if (result.isError()) {

    }

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


template <typename Parser>
void success_parsing(Parser parser,
        parser_result_t<Parser> const& answer,
        std::string const& str,
        std::string_view remaining = "") {
    Stream s{str};
    auto result = parser.apply(s);

    if (result.isError()) {
        printError<typename Parser::Type>(result, s);
    }
    ASSERT_FALSE(result.isError());
    EXPECT_EQ(result.data(), answer);
    EXPECT_EQ(s.remaining(), remaining);
}

template <typename Parser>
void failed_parsing(Parser parser, size_t pos, std::string_view str) {
    Stream s{str};
    auto result = parser.apply(s);

    if (!result.isError()) {
        printError<typename Parser::Type>(result, s);
    }
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().pos, pos);
}
