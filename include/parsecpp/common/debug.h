#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/utils/cmp.h>

namespace prs::debug {

class DebugParser;

//template <bool enable = true>
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

    void addLog(size_t pos, std::string desc) const noexcept {
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
    mutable std::vector<CallInfo> m_callStack;
};


class DebugParser {
public:
    explicit DebugParser(DebugEnvironment& env) noexcept
        : m_env(env) {

    }

    void addLog(size_t pos, std::string desc) const noexcept {
        m_env.addLog(pos, std::move(desc));
    }

    using CallInfo = DebugEnvironment::CallInfo;
private:
    DebugEnvironment& m_env;
};

struct LogPoint : private DebugParser {
public:
    using P = Parser<Drop>;

    LogPoint(DebugEnvironment& env, std::string const& name) noexcept
        : DebugParser(env)
        , m_desc("Point{" + name + "}") {

    }

    P::Result operator()(Stream& stream) const noexcept {
        addLog(stream.pos(), m_desc);
        return P::data({});
    }
private:
    std::string m_desc;
};


inline auto logPoint(DebugEnvironment& env, std::string name) noexcept {
    return make_parser(LogPoint{env, std::move(name)});
}

template <bool onlyError, ParserType ParserA>
struct ParserWork : private DebugParser {
public:
    using Result = typename ParserA::Result;
    ParserWork(DebugEnvironment& env, ParserA tParser, std::string parserName) noexcept
        : DebugParser(env)
        , m_parser(std::move(tParser))
        , m_parserName(std::move(parserName)) {

    }

    Result operator()(Stream& stream) const noexcept(ParserA::nothrow) {
        if constexpr (!onlyError) {
            addLog(stream.pos(), "Before{" + m_parserName + "}");
        }

        return m_parser.apply(stream).join([&](auto&& t) {
            if constexpr (!onlyError) {
                addLog(stream.pos(), "After{" + m_parserName + "}");
            }
            return Result{t};
        }, [&](auto &&error) {
            addLog(stream.pos(), "Error{" + m_parserName + "}: " + error.description);
            return Result{error};
        });
    }
private:
    ParserA m_parser;
    std::string m_parserName;
};

template <ParserType ParserA>
auto parserWork(DebugEnvironment& env, ParserA tParser, std::string parserName) noexcept {
    return ParserA::make(ParserWork<false, std::decay_t<ParserA>>{env, std::move(tParser), std::move(parserName)});
}


template <ParserType ParserA>
auto parserError(DebugEnvironment& env, ParserA tParser, std::string parserName) noexcept {
    return ParserA::make(ParserWork<true, std::decay_t<ParserA>>{env, std::move(tParser), std::move(parserName)});
}


template <ParserType P>
class DebugExecutor {
public:
    template <typename Fn>
    DebugExecutor(std::unique_ptr<DebugEnvironment> spEnv, Fn&& fn) noexcept(std::is_nothrow_invocable_v<Fn, DebugEnvironment&>) {

    }
private:
    std::unique_ptr<DebugEnvironment> m_spEnv;
    P m_parser;
};
//
//template <typename Fn>
//auto makeDebug(Fn&& f) noexcept(std::is_nothrow_invocable_v<Fn, DebugEnvironment&>) {
//    return []
//}
}