#include "../testHelper.h"

std::string_view csvExample = R"(1996,Ford,E350,"Super, ""luxurious"" truck"
1997, "Ford" , A350
"1997", Ford ,E350,)";

TEST(String, escapedStringCSV) {
    auto quoted = skipChars(' ', '\t') >> charFrom('"') >> escapedString<'"', '"'>() << skipChars(' ', '\t');
    // parser std::vector<std::vector<std::string>>
    auto parser = (quoted | until<std::string>(',', '\n')).repeat(charFrom(',')).repeat(charFrom('\n'));

    success_parsing(parser, {
            {"1996", "Ford", "E350", R"(Super, "luxurious" truck)"},
            {"1997", "Ford", " A350"},
            {"1997", " Ford ", "E350", ""}}, csvExample);
}


struct MyCsv {
    unsigned year;
    std::string brand;
    std::pair<char, unsigned> model;
    std::optional<std::string> description;

    bool operator==(MyCsv const& rhs) const noexcept {
        return std::tie(year, brand, model, description)
               == std::tie(rhs.year, rhs.brand, rhs.model, rhs.description);
    }
};

TEST(String, CSVStrongTypes) {
    auto quoted = skipChars(' ', '\t') >> charFrom('"') >> escapedString<'"', '"'>() << skipChars(' ', '\t');
    auto field = (quoted | until<std::string>(',', '\n'));
    // parser std::vector<MyCsv>
    auto year = number<unsigned>();
    auto model = liftM(details::MakeClass<std::pair<char, unsigned>>{}, spaces() >> charFrom(FromRange('A', 'F')), number<unsigned>());
    auto parser = liftM(details::MakeClass<MyCsv>{},
            field * convertResult(year),
            charFrom(',') >> field,
            charFrom(',') >> field * convertResult(model),
            (charFrom(',') >> field).maybe()).repeat(charFrom('\n')).endOfStream();

    std::vector<MyCsv> answer = {
            MyCsv{1996, "Ford", std::make_pair<char, unsigned>('E', 350), R"(Super, "luxurious" truck)"},
            MyCsv{1997, "Ford", std::make_pair<char, unsigned>('A', 350), std::nullopt},
            MyCsv{1997, " Ford ", std::make_pair<char, unsigned>('E', 350), ""}};

    success_parsing(parser, answer, csvExample);
}