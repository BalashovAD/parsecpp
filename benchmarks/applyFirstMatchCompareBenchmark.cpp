#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

constexpr auto sampleSize = 7;
static inline std::array<std::string, sampleSize> samples = {
        "positions",
        "balance_and_position",
        "liquidation-warning",
        "account-greeks",
        "orders",
        "orders-algo",
        "algo-advance",
};

inline auto generateChallengeSamples(size_t maxN) noexcept {
    std::vector<std::string_view> out{}; out.reserve(maxN);

    for (size_t i = 0; i != maxN; ++i) {

    }

    return out;
}
