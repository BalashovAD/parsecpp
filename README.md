# Parser Combinators 

![Build](https://github.com/balashovAD/parsecpp/actions/workflows/cmake.yml/badge.svg)

Based on the paper [Direct Style Monadic     Parser Combinators For The Real World](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/parsec-paper-letter.pdf).

## TODO List
- [x] Add quick examples to readme
- [ ] Installation guide, requirements
- [ ] Add guide how to write the fastest parsers
- [ ] Add Drop control class for performance optimizations
- [x] Disable error log by flag
- [x] Add call stack for debug purpose

## Requirements

It's header only library, require support c++20 standard

## Examples

All quick examples in `examples/quickExamples`

### Hello username
```c++
auto hello = literal("Hello") >> spaces() >> letters();

Stream helloExample{"Hello username"};
std::cout << "Name is " << hello(helloExample).join([](std::string_view name) {
    return name;
}, [](details::ParsingError const& error) {
    return "error";
}) << std::endl;
```

### Parse in class

```c++
struct Point {
    int x;
    int y;
};

class Circle {
public:
    Circle(Point center, double r);
};

// Parser<Point>
auto pointParser = between('(', ')',
           liftM(details::MakeClass<Point>{}, number<int>(), charIn(';') >> number<int>()));

// Parser<Circle>
auto parser = literal("Circle") >> spaces() >>
        liftM(details::MakeClass<Circle>{}, pointParser, spaces() >> number<double>());

Stream example{"Circle (1;-3) 4.5"};
parser(example);
```

### Recursion 
Parsecpp is a top-down parser and doesn't like left recursion. 
Furthermore, building your combinator parser with direct recursion would make a stack overflow before parsing.  
Use `lazy(makeParser)` function that will end the loop while building. 
It builds the combinator, but parser generator will be called only while parsing the stream. 
It's slower than building full parser once before start and makes to use forward declaration and type erasing.  

To remove left recursion use this [general algorithm](https://en.wikipedia.org/wiki/Left_recursion#Removing_left_recursion).
See `examples/calc`.

```c++
Parser<char> makeB();

auto makeA() {
    return charIn('A') >> makeB();
}

Parser<char> makeB() {
    return (charIn('B') | lazy(makeA)).toCommonType();
}

auto parser = makeA();
Stream example("AAAAB");
parser(example);
```

TODO `lazyCached` example

See `examples/calc`, `examples/json`, and unit tests `tests/` for complex examples with recursion

## Build-in operators

In the follow text `Parser<A>` is a `prs::Parser<A, Fn>` for any Fn.   
Because the second template argument is implementation trick what doesn't change the category.  
`Parser'<A>` is `prs::Parser<A>`. It's a common type for all `Parser<A>`.  
`Drop` is special class, propose `A != Drop`.

### To common type 
```
toCommonType :: Parser<A> -> Parser'<A>
```
Usually uses for store parser in class or forward declaration for `lazy`.  
Reduce performance because uses `std::function` for type erasing. 
```c++
using A = std::invoke_result_t<Fn, Stream&>;
auto parserA = prs::Parser<A>::make(fn); // prs::Parser<A, decltype(fn)>
prs::Parser<A> p = parserA.toCommonType();
```
### Forget op `>>` `<<`
```
(>>) :: Parser<A> -> Parser<B> -> Parser<B>
(<<) :: Parser<A> -> Parser<B> -> Parser<A>
```
Sequencing operators such as the semicolon, compose two actions, discarding any value produced by the first(second).

```c++
auto parser = literal("P-") >> number<unsigned>(); // Parser<unsigned>
// "P-4" -> 4
auto between = charIn('*') >> letters() << charIn('*'); // Parser<std::string_view> 
// "*test*" -> test
```

### Or `|`
```
(|) :: Parser<A> -> Parser<A> -> Parser<A>
```
Opposite to Haskell Parsec library, the or operator is always `LL(inf)`. Now library doesn't support `LL(1)`.  
Try to parse using the first parser, if error, rollback pos and try with the second one.
```c++
auto parser = charIn('A') | charIn('B'); // Parser<char>
// "B" -> B
// "A" -> A

auto parser = literal("A") | literal('AB'); // Parser<std::string_view>
// "AB" -> A
```

### Repeat
```
repeat :: Parser<A> -> Parser<Vector<A>>
```
Params: `atLeastOnce` - if false the empty answer will be valid.  
Be careful with parsers that can parse successfully without consuming stream.

```c++
auto parser = charIn('A', 'B', 'C').repeat(); // Parser<std::vector<char>>
// "ABABCDAB" -> {'A', 'B', 'A', 'B', 'C'}
// "DDD" -> error {Need at least one element in answer}

auto wrongUseRepeat = spaces().repeat(); // spaces always return Success, even no spaces has been parsed
// "ABC" -> error {Max iteration in repeat}
```

### Maybe
```
maybe :: Parser<A> -> Parser<std::optional<A>>
```
Parser is always return Success. Rollback stream if it cannot parse `A`.

```c++
auto parser = charIn('A').maybe(); // Parser<std::optional<char>>
// "B" -> std::nullopt
```


### endOfStream
```
endOfStream :: Parser<A> -> Parser<A>
```
Full stream must be consumed for Success. 

```c++
auto parser = charIn('A', 'B').endOfStream(); // Parser<char>
// "B" -> B
// "AB" -> error {Remaining str "B"}
```

### Debug
For debug purpose use `parsecpp/common/debug.h`. All debug 

## LogPoint
Log itself and continue
```c++
```