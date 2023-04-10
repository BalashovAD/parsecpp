# Parser Combinators 

![Build](https://github.com/balashovAD/parsecpp/actions/workflows/BuildLinux.yml/badge.svg)

Based on the paper [Direct Style Monadic     Parser Combinators For The Real World](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/parsec-paper-letter.pdf).

## TODO List
- [x] Add quick examples to readme
- [ ] Add guide how to write the fastest parsers
- [x] Add Drop control class for performance optimizations
- [x] Disable error log by flag
- [x] Add call stack for debug purpose
- [x] Add custom context for parsing

## Examples

All quick examples are located in `examples/quickExamples`.

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
           liftM(details::MakeClass<Point>{}, number<int>(), charFrom(';') >> number<int>()));

// Parser<Circle>
auto parser = literal("Circle") >> spaces() >>
        liftM(details::MakeClass<Circle>{}, pointParser, spaces() >> number<double>());

Stream example{"Circle (1;-3) 4.5"};
parser(example);
```


## Installation

This header only library requires a c++20 compiler.  
You can add git submodule to your project and add `target_include_directories(prj PRIVATE ${PARSECPP_INCLUDE_DIR})`.

Alternatively you can copy & paste the `single_include` headers to your include path.
Use `all.hpp` as base parser header, `full.hpp` contains additional options and extend possibilities.

Configurable parameters: `Parsecpp_DisableError`, turn it on to optimize error string, recommend for release build.


### Recursion 
`Parsecpp` is a top-down parser and doesn't like left recursion. 
Furthermore, building your combinator parser with direct recursion would cause a stack overflow before parsing.  
Use `lazy(makeParser)`, `lazyCached`, `lazyForget` functions that will end the loop while building.
Refer to `benchmark/lazyBenchmark.cpp` for more details.


To remove left recursion use this [general algorithm](https://en.wikipedia.org/wiki/Left_recursion#Removing_left_recursion).
See `examples/calc`.

#### Lazy
The easiest way is to use the `lazy` function.
It builds the combinator, but parser generator will only be called only while parsing the stream.
It's slower than building full parser once before start and avoid type erasing.
However, it's a universal method that works with any parsers.

```c++
Parser<char> makeB();

auto makeA() {
    return charFrom('A') >> makeB();
}

Parser<char> makeB() {
    return (charFrom('B') | lazy(makeA)).toCommonType();
}

auto parser = makeA();
Stream example("AAAAB");
parser(example);
```

#### LazyCached

Make one instance of recursive parser in preparing time. It's a much faster way to make recursive parsers. 
But result parser (`Parser'<T>`) must be pure (doesn't have mutable states inside), 
and allow to be called recursively for one instance. 
Also, because `LazyCached` type dependence on `Fn` that dependence on `Parser<T>`,
the generator cannot use `decltype(auto)` for return type. So, usually, the generator should use `toCommonType` for type erasing.

#### LazyForget

This is an improved version of `lazyCached` without type erasing.
`LazyForget` only depends on the parser result type and doesn't create a recursive type. 
You need to specify return type manually with `decltype(X)`, where `X` is value in `return` with changed `lazyForget<R>(f)` to
`std::declval<Parser<R, Ctx, LazyForget<R>>>()`.
This code is slightly faster when `lazyCached`, but code looks harder to read and edit. 


#### Tag in lazy*
The `Tag` type must be unique for any difference call of `lazyCached` function. 
For cases where you call `lazyCached` only once per line, you can use the default parameter `AutoTag`. 
If you are unsure, specialize `Tag` type manually. Be careful with func helpers that cover the auto tag parameter.
For example the following code won't work correctly:
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
    return (concat(charFrom('(', '{', '['), lazy(bracesLazy) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}

Parser<Unit> bracesCache() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCache) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}

auto bracesForget() noexcept -> decltype((concat(charFrom('(', '{', '['), std::declval<Parser<Unit, VoidContext, LazyForget<Unit>>>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success())) {

    return (concat(charFrom('(', '{', '['), lazyForget<Unit>(bracesForget) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success());
}
```

Benchmark result:
```
BM_bracesSuccess/bracesLazy_median               784 ns
BM_bracesSuccess/bracesCached_median             433 ns
BM_bracesSuccess/bracesCachedDrop_median         312 ns
BM_bracesSuccess/bracesForget_median             429 ns

BM_bracesFailure/bracesLazyF_median              499 ns
BM_bracesFailure/bracesCachedF_median            273 ns
BM_bracesFailure/bracesCachedDropF_median        197 ns
BM_bracesFailure/bracesForgetF_median            268 ns
```

See `examples/calc`, `examples/json`, `benchmark/lazyBenchmark.cpp`, and unit tests `tests/` for more complex examples with recursion.

## Build-in operators

In the following text, `Parser<A>` refers to a `prs::Parser<A, VoidContext, Fn>` for any `Fn`.   
The second template argument is an implementation trick that doesn't change the category. 
`Parser'<A>` is simply shorthand for `prs::Parser<A> := prs::Parser<A, VoidContext, StdFunction>`, 
which is a common type for all `Parser<A>`. `Parser'<A, Ctx>` is shorthand for `prs::Parser<A, Ctx>`, 
which is a common type for all `Parser<A, Ctx>`.  
`Drop` is special class, and we assume that `A != Drop`.  

### Context
// TODO 
Context type `CtxA & CtxB` is shorthand for `UnionCtx<CtxA, CtxB>`. 
In opposite to common `union`, `UnionCtx` depends on the argument sequence. 
For example, `ContextWrapper<char, unsigned> := UnionCtx<char, unsigned> != UnionCtx<unsigned, char> =: ContextWrapper<unsigned, char>` 

### Converting to a Common Type
```
toCommonType :: Parser<A, Ctx> -> Parser'<A, Ctx>
```
This function is typically used to store a parser in a class or for forward declaration of recursion parsers. 
However, it may reduce performance because it uses `std::function` for type erasing.

```c++
using A = std::invoke_result_t<Fn, Stream&>;
auto parserA = prs::Parser<A>::make(fn); // prs::Parser<A, decltype(fn)>
prs::Parser<A> p = parserA.toCommonType();
```
### Forget op `>>` and `<<`
```
(>>) :: Parser<A, CtxA> -> Parser<B, CtxB> -> Parser<B, CtxA & CtxB>
(<<) :: Parser<A, CtxA> -> Parser<B, CtxB> -> Parser<A, CtxA & CtxB>
```
Sequencing operators, such as the semicolon, compose two actions and discard any value produced by the first(second).

```c++
auto parser = literal("P-") >> number<unsigned>(); // Parser<unsigned>
// "P-4" -> 4
auto between = charFrom('*') >> letters() << charFrom('*'); // Parser<std::string_view> 
// "*test*" -> test
```

### Or `|`
```
(|) :: Parser<A, CtxA> -> Parser<A, CtxB> -> Parser<A, CtxA & CtxB>
```
Opposite to Haskell Parsec library, the or operator is always `LL(inf)`. Now library doesn't support `LL(1)`.  
Try to parse using the first parser, if that fails, rollback position and try with the second one.
```c++
auto parser = charFrom('A') | charFrom('B'); // Parser<char>
// "B" -> B
// "A" -> A

auto parser = literal("A") | literal('AB'); // Parser<std::string_view>
// "AB" -> A
```

### Repeat
```
repeat :: Parser<A, Ctx> -> Parser<Vector<A>, Ctx>
```
Be careful with parsers that can parse successfully without consuming the stream.

```c++
auto parser = charFrom('A', 'B', 'C').repeat(); // Parser<std::vector<char>>
// "ABABCDAB" -> {'A', 'B', 'A', 'B', 'C'}
// "DDD" -> error {Need at least one element in answer}

auto wrongUseRepeat = spaces().repeat(); // spaces always return Success, even no spaces has been parsed
// "ABC" -> error {Max iteration in repeat}
```

### Maybe
```
maybe :: Parser<A, Ctx> -> Parser<std::optional<A>, Ctx>
```
This parser always returns Success. Rollback stream if it cannot parse `A`.

```c++
auto parser = charFrom('A').maybe(); // Parser<std::optional<char>>
// "B" -> std::nullopt
```


### endOfStream
```
endOfStream :: Parser<A, Ctx> -> Parser<A, Ctx>
```

This parser requires that the entire stream be consumed for Success.

```c++
auto parser = charFrom('A', 'B').endOfStream(); // Parser<char>
// "B" -> B
// "AB" -> error {Remaining str "B"}
```

### Drop

The `Unit` structure is a special type that has only one value `{}`. 
It serves as a more convenient analog of the `void` c++ return type. 
This parser result type is useful in cases where we only need to indicate the status of the parsing process without value. 
In more complex parsers, `Unit` may be used to represent other information. 

For example:
```c++
auto parser = searchText("test").maybe(); // Parser<std::optional<Unit>>
// "noTEST" -> std::nullopt # indicates that 'test' has not been matched 
```

The `Drop` structure is similar to `Unit` in that it serves the same purpose, 
but it also indicates that we want to optimize the parsing process. 
It's particularly useful for operations like `repeat` and `maybe`. 
Additionally, it's a flag that tells the parser to break type conversion, resulting in faster code.

```c++
auto parser = searchText("test").drop().maybe(); // Parser<Drop>
// "noTEST" -> Drop{} 
// "it's test" -> Drop{} 
```

```c++
auto parser = charFrom('a', 'b', 'c').drop().repeat(); // Parser<Drop>
// "aacbatest" -> Drop{} # "test"
// this code doesn't need to allocate memory for std::vector and works much faster
```

### Debug
For debug purpose use namespace `debug::`, see `parsecpp/common/debug.h` and `tests/common/debugTest.cpp`.

```
debug::DebugContext debugContext;
auto parser = spaces() >> debug::parserWork(number<int>(), "Int") << spaces();
parser(steam, debugContext);
debugContext.get() // <-- Call stack
```

