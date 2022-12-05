#include <parsecpp/all.h>
#include <parsecpp/utils/applyFirstMatch.h>

#include <iostream>
#include <memory>
#include <iomanip>


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
    using Op = NumberType(*)(NumberType, NumberType);
    using ptr = std::shared_ptr<Expr>;

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
            std::optional<std::tuple<Expr::Op, Expr::ptr>> const& maybeRhs) noexcept {
        if (!maybeRhs) {
            return spLhs;
        } else {
            return Expr::make(spLhs, std::get<0>(*maybeRhs), std::get<1>(*maybeRhs));
        }
    }

    Expr::ptr operator()(double n) noexcept {
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
           (spaces() >> charIn('(') >> spaces()) >>
           lazy(makeE) << (spaces() << charIn(')') << spaces())).toCommonType();
}


Parser<std::optional<std::tuple<Expr::Op, Expr::ptr>>> makeT1() noexcept {
    return concat(
            makeOp(Expr::opSelectorHigh),
            liftM(MakeExpr{}, makeF(), lazy(makeT1))
    ).maybe().toCommonType();
}

auto makeT() noexcept {
    return liftM(MakeExpr{}, makeF(), makeT1());
}

Parser<std::optional<std::tuple<Expr::Op, Expr::ptr>>> makeE1() noexcept {
    return concat(
            makeOp(Expr::opSelectorLow),
            liftM(MakeExpr{}, makeT(), lazy(makeE1))
    ).maybe().toCommonType();
}

ParserExpr makeE() noexcept {
    return liftM(MakeExpr{}, makeT(), makeE1()).toCommonType();
}


int main() {
    std::string strExpr;

    auto parser = (makeE() << spaces()).endOfStream();

    getline(std::cin, strExpr);
    Stream stream{strExpr};
    return parser(stream).join([](Expr::ptr const& spExpr) {
        std::cout << "Expr: " << spExpr->toString() << " = " << spExpr->exec() << std::endl;
        return 0;
    }, [&](details::ParsingError const& error) {
        std::cout << stream.generateErrorText(error) << std::endl;
        return 1;
    });
}