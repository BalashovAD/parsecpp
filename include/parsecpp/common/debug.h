#pragma once

#include <parsecpp/core/parser.h>
#include "parsecpp/core/modifier.h"
#include <parsecpp/utils/cmp.h>

namespace prs::debug {

class DebugParser;

class DebugEnvironment {
public:
    struct PrintEOSHelper {
    public:
        static constexpr std::string_view EOS = "_EOS_";

        PrintEOSHelper(std::string_view full, size_t startPos, size_t endPos) noexcept {
            if (full.size() > startPos) {
                m_str = full.substr(startPos, std::max<unsigned>(endPos - startPos, 1));
            } else {
                m_str = EOS;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, PrintEOSHelper helper) noexcept {
            os << helper.m_str;
            return os;
        }

    private:
        std::string_view m_str;
    };

    DebugEnvironment() noexcept = default;

    std::string print(Stream const& stream) const noexcept {
        std::stringstream out;
        out << "Parse '" << stream.full() << "'\n";
        auto step = 0u;
        for (auto const& callInfo : m_callStack) {
            out << step++ << ". ";
            for (auto i = 0u; i != callInfo.level; ++i) {
                out << ">";
            }
            out << "pos: " << callInfo.end
                << "(" << PrintEOSHelper(stream.full(), callInfo.start, callInfo.end) << ")"
                << ": " << callInfo.desc << "\n";
        }

        return out.str();
    }

    void addLog(size_t pos, std::string desc) noexcept {
        m_callStack.emplace_back(pos, pos, std::move(desc), m_stackLevel);
    }

    void addLog(size_t start, size_t end, std::string desc) noexcept {
        m_callStack.emplace_back(start, end, std::move(desc), m_stackLevel);
    }

    void changeLevel(int v) noexcept {
        m_stackLevel += v;
    }

    void clear() noexcept {
        m_callStack.clear();
    }

    struct CallInfo {
        CallInfo(size_t t_start, size_t t_end, std::string t_desc, unsigned t_level = 0) noexcept
            : start(t_start)
            , end(t_end)
            , desc(std::move(t_desc))
            , level(t_level) {

        }

        size_t start = std::string_view::npos;
        size_t end = std::string_view::npos;
        std::string desc;
        unsigned level = 0;
    };

private:
    std::vector<CallInfo> m_callStack;
    unsigned m_stackLevel = 0;
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


struct AddStackLevel {
    auto operator()(auto& parser, Stream& stream, debug::DebugContext& ctx) const noexcept {
        ctx.get().changeLevel(+1);
        Finally revert([&]() {
            ctx.get().changeLevel(-1);
        });
        return parser();
    }
};


struct SaveParsedSource {
    std::string desc;

    auto operator()(auto& parser, Stream& stream, debug::DebugContext& ctx) const noexcept {
        auto start = stream.pos();
        return parser().map([&](auto&& t) {
            auto end = stream.pos();
            ctx.get().addLog(start, end, desc);
            return t;
        });
    }
};

}

namespace prs {

template<>
struct ModifierTrait<debug::AddStackLevel> {
    using Ctx = debug::DebugContext;
};

template<>
struct ModifierTrait<debug::SaveParsedSource> {
    using Ctx = debug::DebugContext;
};

template <bool b>
struct ModifierTrait<debug::ParserWork<b>> {
    using Ctx = debug::DebugContext;
};

}