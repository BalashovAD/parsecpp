#include "../testHelper.h"

TEST(Map, KeyValue) {
    auto key = letters<false, false, std::string>() << spaces() << charIn('=') << spaces();
    auto value = number<int>() << spaces() << charIn(';') << spaces();
    auto parser = toMap(key, value).endOfStream();

    std::map<std::string, int> ans{{"test", 1}, {"p", -3}};
    success_parsing(parser, ans, "test=1; p =   -3   ; ");
    success_parsing(parser, ans, "p=-1; test = 1; p = -3   ; ");

    failed_parsing(parser, 5, "test=");
    failed_parsing(parser, 5, "test=;");
    failed_parsing(parser, 10, "test=1; q=");
}

TEST(Map, KeyValueRestoreInTheMiddle) {
    auto key = letters<false, false, std::string>() << charIn('=');
    auto value = number<int>() << charIn(';');

    auto parser = toMap<false>(key, value);
    std::map<std::string, int> ans{{"test", 1}};
    success_parsing(parser, ans, "test=1;p=", "p=");

    auto parser2 = toMap<false>(key, value).endOfStream();
    failed_parsing(parser2, 7, "test=1;p="); // end of stream error
}

TEST(Map, KeyValueDelimRestoreInTheMiddle) {
    auto key = letters<false, false, std::string>() << charIn('=');
    auto value = number<int>();
    auto delimiter = charIn(';');

    auto parser = toMap<false>(key, value, delimiter);
    std::map<std::string, int> ans{{"test", 1}};
    success_parsing(parser, ans, "test=1;p=", ";p=");

    auto parser2 = toMap<false>(key, value).endOfStream();
    failed_parsing(parser2, 6, "test=1;p="); // end of stream error
}

TEST(Map, KeyValueDelimRestoreLastDelimiter) {
    auto key = letters<false, false, std::string>() << charIn('=');
    auto value = number<int>();
    auto delimiter = charIn(';');

    auto parser = toMap<false>(key, value, delimiter);
    std::map<std::string, int> ans{{"test", 1}};
    success_parsing(parser, ans, "test=1;", ";");
}


TEST(Map, KeyValueDelim) {
    auto key = letters<false, false, std::string>() << spaces() << charIn('=') << spaces();
    auto value = spaces() >> number<int>() << spaces();
    auto delimiter = spaces() >> charIn(',') << spaces();
    auto parser = toMap(key, value, delimiter).endOfStream();

    std::map<std::string, int> ans{{"test", 1}, {"p", -3}};
    success_parsing(parser, ans, "test=1, p =   -3    ");
    success_parsing(parser, ans, "p=-1, test = 1, p = -3   ");

    failed_parsing(parser, 5, "test=");
    failed_parsing(parser, 7, "test=5 q=5");
    failed_parsing(parser, 10, "test=1, q=");
}

TEST(Map, KeyValueDelimNotErrorInTheMiddle) {
    auto key = letters<false, false, std::string>() << spaces() << charIn('=') << spaces();
    auto value = spaces() >> number<int>() << spaces();
    auto delimiter = spaces() >> charIn(',') << spaces();
    auto parser = toMap<false>(key, value, delimiter);

    std::map<std::string, int> ans{{"test", 1}, {"p", -3}};
    success_parsing(parser, ans, "test=1, p =   -3  , t=  ", ", t=  ");
}

TEST(Map, KeyValueMaxIter) {
    auto key = (letters() << charIn('=')).maybe();
    auto value = letters().maybe();
    auto parser = toMap(key, value).endOfStream();

    failed_parsing(parser, 5, "test=4");
}