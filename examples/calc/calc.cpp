#include <parsecpp/full.hpp>

#include <iomanip>            // for operator<<, setprecision
#include <iostream>           // for operator<<, basic_ostream, ostringstream
#include <memory>             // for __shared_ptr_access, make_shared, share...
#include <optional>           // for optional
#include <string>             // for operator<<, char_traits, getline, string
#include <string_view>        // for basic_string_view, operator""sv, operat...
#include <tuple>              // for get, tuple
#include <type_traits>        // for __strip_reference_wrapper<>::__type
#include <utility>            // for make_pair


using namespace prs;
using namespace std::string_view_literals;

double sum(double lhs, double rhs) noexcept {
    return lhs + rhs;
}

double minus(double lhs, double rhs) noexcept {
    return lhs - rhs;
}

double mul(double lhs, double rhs) noexcept {
    return lhs * rhs;
}

double divide(double lhs, double rhs) noexcept {
    return lhs / rhs;
}


class Expr {
public:
    using NumberType = double;
    using Op = NumberType(*)(NumberType, NumberType) noexcept;
    using ptr = std::shared_ptr<Expr>;

    struct SubOp {
        SubOp(Op op, NumberType value)
            : m_pOp(op)
            , m_spRhs(make(value)) {

        }

        SubOp(Op op, ptr spRhs)
            : m_pOp(op)
            , m_spRhs(std::move(spRhs)) {

        }

        using subptr = std::shared_ptr<SubOp>;
        Op m_pOp = nullptr;
        ptr m_spRhs;
    };

    static constexpr auto opSelectorLow = details::makeFirstMatch((Expr::Op)nullptr,
            std::make_pair('+', &sum),
            std::make_pair('-', &minus));

    static constexpr auto opSelectorHigh = details::makeFirstMatch((Expr::Op)nullptr,
            std::make_pair('/', &divide),
            std::make_pair('*', &mul));

    static constexpr auto opToString = details::makeFirstMatch("??"sv,
            std::make_pair(&sum, "+"sv),
            std::make_pair(&minus, "-"sv),
            std::make_pair(&mul, "*"sv),
            std::make_pair(&divide, "/"sv));

    explicit Expr(NumberType n) noexcept
        : m_value(n) {
    }

    explicit Expr(Expr::ptr const& lhs, Op pOp, Expr::ptr const& rhs) noexcept
        : m_value(0)
        , m_spLeft(lhs)
        , m_spRight(rhs)
        , m_pOp(pOp) {
    }


    static ptr make(NumberType const& n) noexcept {
        return std::make_shared<Expr>(n);
    }

    static ptr make(Expr::ptr const& lhs, Op pOp, Expr::ptr const& rhs) noexcept {
        return std::make_shared<Expr>(lhs, pOp, rhs);
    }

    NumberType exec() noexcept(false) {
        if (m_pOp) {
            return (*m_pOp)(m_spLeft->exec(), m_spRight->exec());
        } else {
            return m_value;
        }
    }


    std::string toString() const noexcept {
        if (m_pOp) {
            std::ostringstream os;
            os << "(" << m_spLeft->toString();
            os << " " << opToString.apply(m_pOp) << " ";
            os << m_spRight->toString() << ")";
            return os.str();
        } else {
            std::ostringstream os;
            os << std::setprecision(2);
            os << m_value;
            return os.str();
        }
    }
private:
    NumberType m_value{};
    ptr m_spLeft;
    ptr m_spRight;
    Op m_pOp = nullptr;
};


struct MakeExpr {
    Expr::ptr operator()(
            Expr::ptr const& spLhs,
            std::vector<Expr::SubOp::subptr> const& maybeRhs) const noexcept {
        auto result = spLhs;
        for (auto const& sub : maybeRhs) {
            result = Expr::make(result, sub->m_pOp, sub->m_spRhs);
        }
        return result;
    }

    Expr::ptr operator()(double n) const noexcept {
        return Expr::make(n);
    }
};


using ParserExpr = Parser<Expr::ptr>;

template <typename FirstMatch>
auto makeOp(FirstMatch ops) noexcept {
    return ((spaces() >> anyChar() << spaces()) >>= [ops](char c) {
        return ops.apply(c);
    }).cond([](Expr::Op pOp) {
        return pOp != nullptr;
    });
}


//E -> T E'
//E' -> op1 TE'  |  eps
//T -> F T'
//T' -> op2 FT'  |  eps
//F -> NUMBER | ( E )

ParserExpr makeE() noexcept;

Parser<Expr::ptr> makeF() noexcept {
    return ((number<double>() >>= MakeExpr{}) |
           (spaces() >> charFrom('(') >> spaces()) >>
           lazy(makeE) << (spaces() << charFrom(')') << spaces())).toCommonType();
}


Parser<std::vector<Expr::SubOp::subptr>> makeT1() noexcept {
    return liftM(details::MakeShared<Expr::SubOp>{},
            makeOp(Expr::opSelectorHigh),
            makeF()).repeat().toCommonType();
}

auto makeT() noexcept {
    return liftM(MakeExpr{}, makeF(), makeT1());
}

Parser<std::vector<Expr::SubOp::subptr>> makeE1() noexcept {
    return liftM(details::MakeShared<Expr::SubOp>{},
            makeOp(Expr::opSelectorLow),
            lazy(makeT)).repeat().toCommonType();
}

ParserExpr makeE() noexcept {
    return liftM(MakeExpr{}, makeT(), makeE1()).toCommonType();
}


int main() {
    std::string strExpr;

    auto parser = (makeE() << spaces()).endOfStream();

    getline(std::cin, strExpr);
    Stream stream{strExpr};
    return parser(stream).join([&stream](Expr::ptr const& spExpr) {
        std::cout << "Expr: " << spExpr->toString() << " = " << spExpr->exec() << std::endl;
        return 0;
    }, [&](details::ParsingError const& error) {
        std::cout << stream.generateErrorText(error) << std::endl;
        return 1;
    });
}