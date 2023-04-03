#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/utils/modifier.h>
#include <parsecpp/utils/cmp.h>

namespace prs::debug {

class DebugParser;

class DebugEnvironment {
public:
    struct PrintEOSHelper {
    public:
        static constexpr char EOS_SYMBOL = '\0';

        PrintEOSHelper(std::string_view full, size_t pos) noexcept {
            if (full.size() > pos) {
                m_symbol = full[pos];
            } else {
                m_symbol = EOS_SYMBOL;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, PrintEOSHelper helper) noexcept {
            if (helper.m_symbol == EOS_SYMBOL) {
                os << "_EOS_";
            } else {
                os << helper.m_symbol;
            }
            return os;
        }

    private:
        char m_symbol = EOS_SYMBOL;
    };

    DebugEnvironment() noexcept = default;

    std::string print(Stream const& stream) const noexcept {
        std::stringstream out;
        out << "Parse '" << stream.full() << "'\n";
        auto step = 0u;
        for (auto const& callInfo : m_callStack) {
            out << step++ << ". pos: " << callInfo.pos
                << "(" << PrintEOSHelper(stream.full(), callInfo.pos) << ")"
                << " desc: " << callInfo.desc << "\n";
        }

        return out.str();
    }

    void addLog(size_t pos, std::string desc) noexcept {
        m_callStack.emplace_back(pos, std::move(desc));
    }

    void clear() noexcept {
        m_callStack.clear();
    }

    struct CallInfo {
        CallInfo(size_t t_pos, std::string t_desc) noexcept
            : pos(t_pos)
            , desc(std::move(t_desc)) {

        }

        size_t pos = std::string_view::npos;
        std::string desc;
    };

private:
    std::vector<CallInfo> m_callStack;
};

using DebugContext = ContextWrapper<DebugEnvironment>;

template <typename T, typename CtxT = VoidContext>
class ParserCast {
public:
    using Ctx = CtxT;

    auto toParser() const noexcept {
        return make_parser<Ctx>(static_cast<T const&>(*this));
    }
};

struct LogPoint : public ParserCast<LogPoint, DebugContext> {
public:
    using P = Parser<Drop>;

    explicit LogPoint(std::string const& name) noexcept
        : m_desc("Point{" + name + "}") {

    }

    P::Result operator()(Stream& stream, Ctx& ctx) const noexcept {
        ctx.get().addLog(stream.pos(), m_desc);
        return P::data({});
    }
private:
    std::string m_desc;
};


inline auto logPoint(std::string const& name) noexcept {
    return LogPoint{name}.toParser();
}

template <bool onlyError>
struct ParserWork {
public:
    explicit ParserWork(std::string parserName) noexcept
        : m_parserName(std::move(parserName)) {

    }

    auto operator()(auto& parser, Stream& stream, DebugContext& ctx) const {
        if constexpr (!onlyError) {
            ctx.get().addLog(stream.pos(), "Before{" + m_parserName + "}");
        }

        return parser().map([&](auto&& t) {
            if constexpr (!onlyError) {
                ctx.get().addLog(stream.pos(), "After{" + m_parserName + "}");
            }
            return t;
        }, [&](auto &&error) {
            ctx.get().addLog(stream.pos(), "Error{" + m_parserName + "}: " + error.description);
            return error;
        });
    }
private:
    std::string m_parserName;
};

template <ParserType ParserA>
auto parserWork(ParserA parser, std::string parserName) noexcept {
    return parser * ModifyWithContext<ParserWork<false>, DebugContext>{parserName};
}


template <ParserType ParserA>
auto parserError(ParserA parser, std::string parserName) noexcept {
    return parser * ModifyWithContext<ParserWork<true>, DebugContext>{parserName};
}


}