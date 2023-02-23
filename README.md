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
`Parsecpp` is a top-down parser and doesn't like left recursion. 
Furthermore, building your combinator parser with direct recursion would make a stack overflow before parsing.  
Use `lazy(makeParser)`, `lazyCached`, `lazyForget` functions that will end the loop while building. 
See `benchmark/lazyBenchmark.cpp` for details.


To remove left recursion use this [general algorithm](https://en.wikipedia.org/wiki/Left_recursion#Removing_left_recursion).
See `examples/calc`.

#### Lazy
The easiest way is use `lazy` function.
It builds the combinator, but parser generator will be called only while parsing the stream.
It's slower than building full parser once before start and avoid type erasing.
But it's universal method that works with any parsers.

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

#### LazyCached

Make one instance of recursive parser in preparing time. It's much faster way to make recursive parsers. 
But result parser (`Parser'<T>`) must be pure (doesn't have mutable states inside), 
and allow to be called recursively for one instance. 
Also, because `LazyCached` type dependence on `Fn` that dependence on `Parser<T>`,
generator cannot use `decltype(auto)` for return type. So, usually the generator should use `toCommonType` for type erasing.

#### LazyForget

This improved version with type erasing of `lazyCached`. 
`LazyForget` dependence only on parser result type and doesn't make recursive type. 
You need to specify return type manually with `decltype(X)`, where `X` is value in `return` with changed `lazyForget<R>(f)` to
`std::declval<Parser<R, LazyForget<R>>>()`.
This code is slightly faster when `lazyCached`, but code looks harder to read and edit. 


#### Tag in lazy*
The `Tag` type must be unique for any difference call of `lazyCached` function. 
For the case when you call `lazyCached` only once per line you can use default parameter(`AutoTag`). 
If you isn't sure, specialize `Tag` type manually. Be careful with func helpers that cover the auto tag parameter.
For example the follow code won't work correctly
```c++
// Doesn't work properly
template <typename Fn>
auto f(Fn f) {
// use AutoTag with current line, but this code isn't unique for difference f with the same class Fn
    return lazyCached(f) << spaces(); 
}

// Correct version
template <typename Tag = AutoTag<>, typename Fn>
auto f(Fn f) {
// use AutoTag line of call f
    return lazyCached<Tag>(f) << spaces();
}
```

```c++
// Fn ~ std::function<Parser'<T>(void)>

template <typename Tag = AutoTag<>, std::invocable Fn>
auto lazyCached(Fn const&) noexcept(Fn);
```

#### Compare lazy* functions

```c++
Parser<Unit> bracesLazy() noexcept {
    return (concat(charIn('(', '{', '['), lazy(bracesLazy) >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}

Parser<Unit> bracesCache() noexcept {
    return (concat(charIn('(', '{', '['), lazyCached(bracesCache) >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}

auto bracesForget() noexcept -> decltype((concat(charIn('(', '{', '['), std::declval<Parser<Unit, LazyForget<Unit>>>() >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success())) {

    return (concat(charIn('(', '{', '['), lazyForget<Unit>(bracesForget) >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success());
}
```

Benchmark result:
```
BM_bracesSuccess/bracesLazy_median           756 ns
BM_bracesSuccess/bracesCached_median         439 ns
BM_bracesSuccess/bracesForget_median         418 ns

BM_bracesFailure/bracesLazyF_median          494 ns
BM_bracesFailure/bracesCachedF_median        279 ns
BM_bracesFailure/bracesForgetF_median        270 ns
```

See `examples/calc`, `examples/json`, `benchmark/lazyBenchmark.cpp`, and unit tests `tests/` for complex examples with recursion

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