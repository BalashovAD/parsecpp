#include <parsecpp/all.h>

#include <iostream>


using namespace prs;


int main() {
    auto hello = literal("Hello") >> spaces() >> letters();

    Stream helloExample{"Hello username"};
    std::cout << "Name is " << hello(helloExample).join([](std::string_view name) {
        return name;
    }, [](details::ParsingError const& error) {
        return "error";
    }) << std::endl;

    return 0;
}