#include "../testHelper.h"

std::string fromUtf8(char8_t const* str) {
    return std::string(reinterpret_cast<char const*>(str));
}

TEST(UTF8, InString) {
    std::string rawString = fromUtf8(u8"hello 世界!");
    auto parser = literal<"hello "_prs>() >> until<'!'>();
    success_parsing(parser, fromUtf8(u8"世界"), rawString, "!");
}

TEST(UTF8, CorruptAny) {
    std::string rawString = fromUtf8(u8"hello 世界!");
    auto parser = literal<"hello 世"_prs>() >> anyChar() << untilEnd();
    success_parsing(parser, fromUtf8(u8"界")[0], rawString);
}

TEST(UTF8, SearchOneSymbol) {
    std::string rawString = fromUtf8(u8"hello 世界!");
    auto parser = searchText<"世"_prs>() >> untilEnd();
    success_parsing(parser, fromUtf8(u8"界!"), rawString);
}

TEST(UTF8, fromCharForUtf8) {
    std::string rawString = fromUtf8(u8"hello 世界!");
    auto parser = search(charFrom<' '>()) >> literal<"世"_prs>() >> untilEnd();
    success_parsing(parser, fromUtf8(u8"界!"), rawString);
}