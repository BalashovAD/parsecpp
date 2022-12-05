# Parser Combinators 

Based on the paper [Direct Style Monadic Parser Combinators For The Real World](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/parsec-paper-letter.pdf).

## TODO List
- [ ] Add quick examples to readme
- [ ] Installation guide, requirements
- [ ] Add guide how to write the fastest parsers
- [ ] Disable error log by flag
- [ ] Add call stack for debug purpose

## Requirements

It's header only library, require c++20 standard

## Examples

All quick examples in `examples/quickExamples`
```c++
auto hello = literal("Hello") >> spaces() >> letters();

Stream helloExample{"Hello username"};
std::cout << "Name is " << hello(helloExample).join([](std::string_view name) {
    return name;
}, [](details::ParsingError const& error) {
    return "error";
}) << std::endl;
```


See `examples/calc`, `examples/json`, and unit tests `tests/` for complex examples with recursion
