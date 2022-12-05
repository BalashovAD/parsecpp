#include "../testHelper.h"

TEST(Map, KeyValue) {
    auto key = letters<false, false, std::string>() << spaces() << charIn('=') << spaces();
    auto value = number<int>() << spaces() << charIn(';') << spaces();
    auto parser = toMap(key, value).endOfStream();

    std::map<std::string, int> ans{{"test", 1}, {"p", -3}};
    success_parsing(parser, ans, "test=1; p =   -3   ; ");
    success_parsing(parser, ans, "p=-1; test = 1; p = -3   ; ");

    failed_parsing(parser, 0, "test=");
    failed_parsing(parser, 0, "test=;");
    failed_parsing(parser, 8, "test=1; q");
    failed_parsing(parser, 8, "test=1; q");
}


TEST(Map, KeyValueDelim) {
    auto key = letters<false, false, std::string>() << spaces() << charIn('=') << spaces();
    auto value = spaces() >> number<int>() << spaces();
    auto delimiter = spaces() >> charIn(',') << spaces();
    auto parser = toMap(key, value, delimiter).endOfStream();

    std::map<std::string, int> ans{{"test", 1}, {"p", -3}};
    success_parsing(parser, ans, "test=1, p =   -3    ");
    success_parsing(parser, ans, "p=-1, test = 1, p = -3   ");

    failed_parsing(parser, 0, "test=");
    failed_parsing(parser, 6, "test=1,");
    failed_parsing(parser, 6, "test=1, q");
    failed_parsing(parser, 14, "test=1, q = -3, ");
}