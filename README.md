# Parser Combinators 

![Build](https://github.com/balashovAD/parsecpp/actions/workflows/BuildLinux.yml/badge.svg)

Based on the paper [Direct Style Monadic     Parser Combinators For The Real World](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/parsec-paper-letter.pdf) ([alt](https://drive.proton.me/urls/7DD4TQHFS8#Ao85HnCqn4OD)).

## TODO List
- [x] Disable error log by flag
- [x] Add call stack for debug purpose
- [x] Add custom context for parsing
- [ ] Support LL(k) grammatical rules
- [ ] Non ascii symbols
- [ ] Lookahead operators
- [ ] SIMD

## Examples

All quick examples are located in `examples/quickExamples`.

### Hello username
```c++
// Parser<std::string_view>
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

// Parser<Point>: (%int%;%int%)
auto pointParser = between('(', ')',
           liftM(details::MakeClass<Point>{}, number<int>(), charFrom(';') >> number<int>()));

// Parser<Circle>: Circle\s*%Point%\s*%double%
auto parser = literal("Circle") >> spaces() >>
        liftM(details::MakeClass<Circle>{}, pointParser, spaces() >> number<double>());

Stream example{"Circle (1;-3) 4.5"};
parser(example);
```

## Installation

This header only library requires a c++20 compiler.  
You can include the library as a git submodule in your project and add 
`target_include_directories(prj PRIVATE ${PARSECPP_INCLUDE_DIR})` to your `CMakeLists.txt` file.

Alternatively, you can copy & paste the `single_include` headers to your project's include path.
Use `all.hpp` as the base parser header, while `full.hpp` contains some extra options.

There is also a configurable parameter, `Parsecpp_DisableError`, 
that you can turn on to optimize error string. `-DParsecpp_DisableError=ON` is recommended for release builds.

## Build-in operators

In the following text, `Parser<A>` refers to a `prs::Parser<A, VoidContext, Fn>` for any `Fn`.   
The second template argument is an implementation trick that doesn't change the category. 
`Parser'<A>` is simply shorthand for `prs::Parser<A> := prs::Parser<A, VoidContext, StdFunction>`, 
which is a common type for all `Parser<A>`. `Parser'<A, Ctx>` is shorthand for `prs::Parser<A, Ctx>`, 
which is a common type for all `Parser<A, Ctx>`.  
`Drop` is special class, and we assume that `A != Drop`.  

### Context

The library is designed for high performance and efficiency, 
so parsers should be built once and only called during the active phase. 
To capture dynamically changing variables, you can add context to your parser. 
For example, if you need to process only lines with `ts >= ts0`, 
you can see the `example/quick.cpp` file for an example of how to use context.
```c++
// Created once
using TsContext = ContextWrapper<Timestamp const&>;
auto parser = skipToNext(concat(parserTS.condC<TsContext>([](Timestapm const& ts, TsContext& ctx) {
    return ts >= ctx.get();
}), parserData), searchText("\n")).repeat(); // std::vector<std::tuple<Timestamp, Data>>

// ...
// use
TsContext ctx{ts0};
parser(stream, ctx).map([](std::vector<std::tuple<Timestamp, Data>> const& result) {
    // ...
});
```

In addition, context can be used to store the parser's side effect to a mutable variable. 
Often, this is a cleaner way than capturing a lot of variables by link in lambdas at creation time. 
For example, see the `Debug` section and `debug::DebugContext`.

Context type `CtxA & CtxB` is shorthand for `UnionCtx<CtxA, CtxB>`.
Unlike the common `union`, `UnionCtx` depends on the argument sequence. 
For example, `ContextWrapper<char, unsigned> := UnionCtx<char, unsigned> != UnionCtx<unsigned, char> =: ContextWrapper<unsigned, char>`.
Also `CtxA & CtxA := CtxA`, `CtxA & VoidCtx = CtxA`, `VoidCtx & CtxA = CtxA`.

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
// "AB" -> error {Remaining m_str "B"}
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

### Recursion
`Parsecpp` is a top-down parser and doesn't like left recursion.
Furthermore, building your combinator parser with direct recursion would cause a stack overflow before parsing.  
Use `lazy`, `lazyCached`, `lazyForget`, `lazyCtxBinding` functions that will end the loop while building.
Refer to `benchmark/lazyBenchmark.cpp` for more details.

To remove left recursion use this [general algorithm](https://en.wikipedia.org/wiki/Left_recursion#Removing_left_recursion).
See `examples/calc` for an example.

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

Make one instance of recursive parser in preparing time. It's a much faster way to create recursive parsers.
However, the resulting parser (`Parser'<T>`) must not have mutable states inside, captured non-unique variables,
and allow to be called recursively for one instance.
Also, because `LazyCached` type dependence on `Fn` which depends on `Parser<T>`,
the generator cannot use `decltype(auto)` for the return type.
So, usually, the generator should use `toCommonType` for type erasing.

#### LazyForget

This is an alternate version of `lazyCached` without type erasing.
`LazyForget` only depends on the parser result type and doesn't create a recursive type.
You need to specify return type manually with `decltype(X)`,
where `X` is value in `return` with changed `lazyForget<R>(f)` to `std::declval<Parser<R, Ctx, LazyForget<R>>>()`.
This code may be (need to check it) slightly faster when `lazyCached`, but code looks harder to read and edit.

#### LazyCtxBinding

This is an alternate version of `lazyCached` that using context variable to determinate recursion.
This version of recursion methods also requests type erasing using `toCommonType`,
but don't have restrictions for capturing variables and values in parser.
`LazyCtxBinding` adds new context to parser that contains `LazyCtxBinding<T, ParserCtx, Tag>::LazyContext` by value.
If an original parser don't have another context, you can create recursion storage instance using `makeBindingCtx(parser)`,
and context like `auto ctx = parser.makeCtx(lazyBindingStorage);`

This is an alternative version of `lazyCached` that uses a context variable to determine recursion.
This version of recursion methods also requires type erasing, for example, using `toCommonType`,
but it doesn't have restrictions for capturing variables and values in the parser.
`LazyCtxBinding` adds a new context to the parser that contains `LazyCtxBinding<T, ParserCtx, Tag>::LazyContext` by value.
If the original parser doesn't have another context,
you can create a recursion storage instance using `makeBindingCtx(parser)`,
and a context like `auto ctx = parser.makeCtx(lazyBindingStorage);`.

#### Tag in lazy*
The `Tag` type must be unique for any difference captured parser of `lazyCached`, `lazyForget` function.
For cases where you call `lazyCached` only once per line, you can use the default tag `AutoTag`.
Use macros `AutoTagT` for unique type, `AutoTagV` for tag instance.  
If you are unsure, specialize `Tag` type manually. Be careful with func helpers that cover the tag parameter.
For example the following code won't work correctly:
```c++
// Doesn't work properly
template <typename Fn>
auto f(Fn f) {
// use AutoTag with current line, but this code isn't unique for difference f with the same class Fn
    return lazyCached<AutoTagT>(f) << spaces(); 
}

// Correct version
template <typename Tag, typename Fn>
auto f(Fn f) {
// use AutoTag line of call f
    return lazyCached<Tag>(f) << spaces();
}
```

```c++
// Fn ~ std::function<Parser'<T>(void)>

template <typename Tag, std::invocable Fn>
auto lazyCached(Fn const&) noexcept(Fn);
```

#### Compare lazy* functions

```c++
Parser<Unit> bracesLazy() noexcept {
    return (concat(charFrom('(', '{', '['), lazy(bracesLazy) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat() >> success()).toCommonType();
}

Parser<Unit> bracesCache() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCache, AutoTagM) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat() >> success()).toCommonType();
}
using ForgetTag = AutoTagT;
auto bracesForget() noexcept -> decltype((concat(charFrom('(', '{', '['), std::declval<Parser<Unit, VoidContext, LazyForget<Unit, ForgetTag>>>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat() >> success())) {

    return (concat(charFrom('(', '{', '['), lazyForget<Unit, ForgetTag>(bracesForget) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat() >> success());
}

auto bracesCtx() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCtxBinding<Unit>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success()).toCommonType();
}

void usingBracesCtx() {
    auto parser = bracesCtx();
    auto lazyBindingStorage = makeBindingCtx(baseParser);
    auto ctx = parser.makeCtx(lazyBindingStorage);
    parser(stream, ctx);
}
```

Benchmark results:

| *                          | Success   | Failure   | Speedup   |
|----------------------------|-----------|-----------|-----------|
| bracesLazy                 | 4667ns    | 4451ns    | 0.56x     |
| bracesCached               | 2599ns    | 2447ns    | 1.0x      |
| bracesCacheConstexpr       | 2692ns    | 2535ns    | 0.97x     |
| bracesForget               | 2745ns    | 2591ns    | 0.95x     |
| bracesCtx                  | 2622ns    | 2495ns    | 0.99x     |
| bracesCtxConstexpr         | 2647ns    | 2523ns    | 0.98x     |
| -------------------------- | --------- | --------- | --------- |
| bracesCachedDrop           | 1987ns    | 1904ns    | 1.31x     |
| bracesCacheDropConstexpr   | 1618ns    | 1501ns    | 1.61x     |
| bracesCtxDrop              | 1952ns    | 1856ns    | 1.33x     |
| bracesCtxDropConstexpr     | 1601ns    | 1511ns    | 1.62x     |

```
AMD Ryzen 7 5700U, linux gcc 12.3.0 compiler, clang 15.0.7 linker
100 repetitions, median value, no-drop cv < 1.7%, drop cv < 0.7%
Run on (16 X 4369.92 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 4096 KiB (x2)
```

See `examples/calc`, `examples/json`, `benchmark/lazyBenchmark.cpp`, and unit tests `tests/` for more complex examples with recursion.


### Modifier

The helpful function to modify a parser execution is `Modifier`. It's an overloaded `operator*`, `operator*=` that wrap 
parser to the next function. Have the same functionality as a native method of `Parser`.
Example:
```c++
struct MustConsume {
    template <typename T>
    auto operator()(ModifyCallerI<T>& parser, Stream& s) const {
        auto pos = s.pos();
        return parser().flatMap([pos, &s](T &&t) {
            if (s.pos() > pos) {
                return Parser<T>::data(t);
            } else {
                return Parser<T>::makeError("Didn't consume stream", s.pos());
            }
        });
    }
};


auto parser = spaces() * MustConsume{};

Stream s{"test"};
parser(s); // failed, needs to consume at least on symbol
```

Because `operator*` has more priority than all others operators that you use, the next code is equivalent:
```c++
a << b * mod; // a << (b * mod)
a << b *= mod; // (a << b) * mod
```
Use `operator*=` as lower priority `operator*` with `modify` functionality. Also, `operator*=` is Right-to-left.
```
(a | b) * mod * mod === ((a | b) * mod) * mod // Good
(a | b) *= mod *= mod === ((a | b) *= (mod *= mod)) // Compile error, modifier can be applied only to parser  
```

### Operation precedence
```
a >> b >> c === (a >> b) >> c
a | b | c === (a | b) | c
a >> b | c === (a >> b) | c
a | b >> c | d === (a | (b >> c)) | d
a >> b | c >>= F === ((a >> b) | c) >>= F
```

See [cpp](https://en.cppreference.com/w/cpp/language/operator_precedence) and `tests/core/opPriorityTest.cpp` for details.

### Repeat
The simplest way to customize `Parser::reapeat` functionality is using `Repeat` class.
For example, if you don't need to store all values, but compute some function of it, you can use `processRepeat` function:
```c++
// Just for example, use fmap to transform and context to capture variables 
std::string s;
auto parser = charFrom(FromRange('a', 'z')) * processRepeat([&](char c) {
    s += toupper(c);
});

Stream stream{"zaqA"};
parser(stream); // remaining "A";
s == "ZAQ";
```

### Debug
For debug purpose use namespace `debug::`. See `example/json/json.cpp`, `parsecpp/common/debug.h` and `tests/common/debugTest.cpp`.

```c++
debug::DebugContext debugContext;
auto parser = spaces() >> debug::parserWork(number<int>(), "Int") << spaces();
parser(steam, debugContext);
debugContext.get() // <-- Call stack
```
