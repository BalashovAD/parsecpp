# Parser Combinators 

![Build](https://github.com/balashovAD/parsecpp/actions/workflows/BuildLinux.yml/badge.svg)

Based on the paper [Direct Style Monadic     Parser Combinators For The Real World](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/parsec-paper-letter.pdf) ([alt](https://drive.proton.me/urls/7DD4TQHFS8#Ao85HnCqn4OD)).

- [Design goals](#design-goals)
- [Examples](#examples)
- [Installation](#installation)
- [Build-in operators](#build-in-operators)
- [Recursion](#recursion)
- [Advanced](#Advanced)
- [Category](#category)

## Design goals
This C++ library, modeled after Haskell's Parsec, is aimed at providing a robust, type-safe parsing framework with a functional-like syntax.
The key objective is to deliver zero-cost abstractions that maintain the library's performance at a level comparable to hand-optimized code, ensuring it excels in performance-critical applications.
### Highlights
- Concepts and compile time checks
- Header only for straightforward integration
- Zero abstraction code, no virtual calls
- No memory allocation
- No exception style

## Examples
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
Here's an example of parsing a complex object into a structure and verifying the correctness of the result.
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
        liftM(details::MakeClass<Circle>{}, pointParser, spaces() >> number<double>())
        .cond([](Circle const& c) {
            return c._radius >= 0;
        });

Stream example{"Circle (1;-3) 4.5"};
auto result = parser(example);
if (result) {
    result.data(); // Circle, assert(_radius >= .0)
}
```

### Sum
Here's an example where we calculate the sum of the products of paired numbers **in place** and return only the resulting sum.
```cpp
struct CountSumRepeat : public Repeat<CountSumRepeat, double, VoidContext> {
    void add(Container& sum, std::tuple<double, double> priceAndQty) const noexcept {
        sum += std::get<0>(priceAndQty) * std::get<1>(priceAndQty);
    }
};

// Parser<std::tuple<double, double>>
auto pairParser = concat(charFrom<'('>() >> number(), charFrom<';'>() >> number() << charFrom<')'>());
// Parser<double>
auto parser = (pairParser * CountSumRepeat{}).endOfStream();
Stream example("(1.5;3)(2;1)(0;3)");
parser(example).join([](double sum) {
    std::cout << "Sum: " << sum << std::endl;
}, [&example](auto const& error) {
    std::cout << "Error: " << example.generateErrorText(error) << std::endl;
});
```

All quick examples are located in `examples/quickExamples`.

## Installation

![Clang-17](https://github.com/balashovAD/parsecpp/actions/workflows/CompilerCheckClang17.yml/badge.svg)
![Clang-16](https://github.com/balashovAD/parsecpp/actions/workflows/CompilerCheckClang16.yml/badge.svg)
![GCC-13](https://github.com/balashovAD/parsecpp/actions/workflows/CompilerCheckGCC13.yml/badge.svg)
![GCC-12](https://github.com/balashovAD/parsecpp/actions/workflows/CompilerCheckGCC12.yml/badge.svg)

This header only library requires a **c++20** compiler.  
You can include the library as a git submodule in your project and add 
`target_include_directories(prj PRIVATE ${PARSECPP_INCLUDE_DIR})` to your `CMakeLists.txt` file.

Alternatively, you can copy & paste the `single_include` headers to your project's include path.
Use `all.hpp` as the base parser header, while `full.hpp` contains some extra options.

There is also a configurable parameter, `Parsecpp_DisableError`, 
that you can turn on to optimize error string. `-DParsecpp_DisableError=ON` is recommended for release builds.

## TODO List
- [x] Disable error log by flag
- [x] Add call stack for debug purpose
- [x] Add custom context for parsing
- [ ] Support LL(k) grammatical rules
- [ ] Non ascii symbols (partial support for now)
- [ ] Lookahead operators
- [ ] SIMD

## Build-in operators

In the following text, `Parser<A>` refers to a `prs::Parser<A, VoidContext, Fn>` for any `Fn`.   
The third template argument is an implementation trick that doesn't change the category. 
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

## Recursion
`Parsecpp` is a top-down parser and doesn't like left recursion.
Furthermore, building your combinator parser with direct recursion would cause a stack overflow before parsing.  
Use `lazy`, `lazyCached`, `lazyForget`, `lazyCtxBinding` functions that will end the loop while building.
Refer to `benchmark/lazyBenchmark.cpp` for more details.

To remove left recursion use this [general algorithm](https://en.wikipedia.org/wiki/Left_recursion#Removing_left_recursion).
See `examples/calc` for an example.

### Lazy
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

### LazyCached

Make one instance of recursive parser in preparing time. It's a much faster way to create recursive parsers.
However, the resulting parser (`Parser'<T>`) must not have mutable states inside, captured non-unique variables,
and allow to be called recursively for one instance.
Also, because `LazyCached` type dependence on `Fn` which depends on `Parser<T>`,
the generator cannot use `decltype(auto)` for the return type.
So, usually, the generator should use `toCommonType` for type erasing.

### LazyForget

This is an alternate version of `lazyCached` without type erasing.
`LazyForget` only depends on the parser result type and doesn't create a recursive type.
You need to specify return type manually with `decltype(X)`,
where `X` is value in `return` with changed `lazyForget<R>(f)` to `std::declval<Parser<R, Ctx, LazyForget<R>>>()`.
This code may be (need to check it) slightly faster when `lazyCached`, but code looks harder to read and edit.

### LazyCtxBinding

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

### Tag in lazy*
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

### Compare lazy* functions

```c++
auto bracesSelfLazy() noexcept {
    return selfLazy<Unit>([](auto const& self) {
        return concat(charFrom('(', '{', '['), self >> charFrom(')', '}', ']'))
                .cond(checkBraces).template repeat<REPEAT_PRE_ALLOC>() >> success();
    });
}

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

See `examples/calc`, `examples/json`, `benchmark/lazyBenchmark.cpp`, and unit tests `tests/` for more complex examples with recursion.

## Advanced

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

## Category

An instance of the `Parser` class in C++ can be understood as both a Monad and, by extension, an Applicative functor. 
This interpretation aligns closely with C++ implementations, though I use a slightly modified definition for these concepts to better fit the C++ context:
```haskell
class Functor Parser where
	fmap :: (a -> b) -> Parser a -> Parser b

class Functor Parser => Applicative Parser where
	pure :: a -> Parser a
	ap :: Parser (a -> b) -> Parser a -> Parser b
	
class Applicative Parser => Monad Parser where 
    bind :: Parser a -> (a -> Parser b) -> Parser b
```

I propose viewing the Applicative functor from a perspective that emphasizes function application. 
Essentially, Applicative extends the capability of `fmap` to handle functions with multiple arguments.
This can be conceptualized in pseudo Haskell notation as: 
```haskell
fmap_ap :: (t_1 -> ... -> t_n -> d) -> Parser t_1 -> ... -> Parser t_n -> Parser d
```

In C++ code, this concept translates to:
```c++
// fmap 
parser >>= fn; // Parser<A> >>= (A -> B) :: Parser<B> 
// or 
liftM(f, parser); // liftM (A -> B) Parser<A> :: Parser<B>

// pure
pure(t); // Parser<T>

// fmap_ap
liftM(f, parserA, ..., parserZ); // liftM (T_1 -> ... -> T_n -> D) Parser<T_1> ... Parser<T_n> :: Parser<D>

// bind
parser.bind(fn); // Parser<A> `.bind` (A -> Parser<B>) :: Parser<B>
```

See [TypeClassopedia](https://wiki.haskell.org/wikiupload/e/e9/Typeclassopedia.pdf) ([alt](https://drive.proton.me/urls/ARFQ2T5XV0#1rW2TSCi7O4l)) for details.