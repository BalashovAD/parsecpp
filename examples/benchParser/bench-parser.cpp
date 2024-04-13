#include <parsecpp/full.hpp>

#include <chrono>
#include <set>
#include <map>
#include <iostream>
#include <iomanip>
#include <numeric>

using namespace prs;
using namespace std::string_view_literals;
using namespace std::chrono;
using sv = std::string_view;

// BM_eq/EQ_short_slow_mean                   18.7 ns         18.7 ns          200 bytes_per_second=1.49689G/s items_per_second=53.5758M/s

using BenchDuration = duration<double, nanoseconds::period>;

sv trim(sv str, sv symbols = " \t\n\r\f\v") {
    auto start = str.find_first_not_of(symbols);
    if (start == std::string_view::npos) return {};
    auto end = str.find_last_not_of(symbols);
    return str.substr(start, end - start + 1);
}

struct BenchInfo {
    using CustomParams = std::map<std::string, double>;

    BenchInfo(sv gn, sv sn, BenchDuration t, BenchDuration cpu, uint64_t it, CustomParams cp) noexcept
        : globalName(gn)
        , subName(sn)
        , time(t)
        , cpuTime(cpu)
        , iterations(it)
        , customParams(std::move(cp)) {
        full = globalName + '/' + subName;
    }

    bool match(sv s) const noexcept {
        return full.find(s) != sv::npos;
    }

    std::optional<std::string> getSuffix(sv prefix) const noexcept {
        if (full.starts_with(prefix)) {
            return std::string(trim(full.substr(prefix.size()), "/_-"));
        } else {
            return std::nullopt;
        }
    }

    std::string globalName;
    std::string subName;
    std::string full;
    BenchDuration time;
    BenchDuration cpuTime;
    uint64_t iterations;
    CustomParams customParams;

    struct TimePrinter {
        BenchDuration time;

        // some std implementation doesn't overload operator<< for chrono::duration
        friend std::ostream& operator<<(std::ostream& os, TimePrinter const& printer) noexcept {
            // should use another ostream to print twice to save std::setw value
            std::stringstream strStream;
            if (printer.time < microseconds(25)) {
                strStream << printer.time.count() << "ns";
            } else if (printer.time < milliseconds(10)) {
                strStream << duration_cast<duration<double, microseconds::period>>(printer.time).count() << "us";
            } else {
                strStream << duration_cast<duration<double, milliseconds::period>>(printer.time).count() << "ms";
            }

            return os << strStream.str();
        }
    };

    auto print() const {
        return TimePrinter{cpuTime};
    }

    friend std::ostream& operator<<(std::ostream& os, BenchInfo const& info) noexcept {
        os << info.globalName << "/" << info.subName << "-" << info.cpuTime.count() << " " << info.iterations;
        return os;
    }
};

BenchDuration durationFromString(double t, std::string_view period) noexcept {
    static constexpr auto strToPeriod = details::makeFirstMatch(0,
        std::make_pair("ns"sv, 1),
        std::make_pair("ms"sv, 1000),
        std::make_pair("s"sv, 1000 * 1000),
        std::make_pair("%"sv, 1));

    return BenchDuration(t * strToPeriod.apply(period));
}

struct MultInfo {
    MultInfo() = default;
    explicit MultInfo(double v) : mult({v}) {};

    std::vector<double> mult;

    struct Printer {
        friend std::ostream& operator<<(std::ostream& os, Printer const& printer) noexcept {
            std::stringstream strStream;
            strStream << std::setprecision(3);
            if (printer.vals.empty()) {
                strStream << "-";
            } else {
                double val = std::accumulate(printer.vals.begin(), printer.vals.end(), .0) / printer.vals.size();
                strStream << val << "x";
//                strStream << "(";
//                for (auto const& v : printer.vals) {
//                    strStream << v << " + ";
//                }
//                strStream << ")";
            }
            return os << strStream.str();
        }

        std::vector<double> vals;
    };

    auto print() const {
        return Printer{mult};
    }

    friend std::ostream& operator<<(std::ostream& os, MultInfo const& info) noexcept {
        os << "Mult:" << info.mult.size();
        return os;
    }
};

// Parser<BenchDuration>
auto durationParser() noexcept {
    return liftM(durationFromString, number<double>(), spaces() >> until<AnySpace{}>());
}

// Parser<BenchInfo>
auto infoParser(bool agg = false) noexcept {
    Parser<sv> subNameParser = until<AnySpace{}>().toCommonType();
    if (agg) {
        static constexpr sv medianPrefix = "_median";
        subNameParser = (until<AnySpace{}>().cond([](sv const& s) {
            return s.ends_with(medianPrefix);
        }) >>= [](sv const& s) {
            return s.substr(0, s.size() - medianPrefix.size());
        }).toCommonType();
    }
    return liftM(details::MakeClass<BenchInfo>{},
         until<'/'>() << charFrom<'/'>(),
         subNameParser,
         spaces() >> durationParser(), spaces() >> durationParser(),
         spaces() >> number<uint64_t>(),
         pure(BenchInfo::CustomParams{}));
}


bool hasArgumentKey(int argc, char* argv[], sv key) {
    return std::any_of(argv + 1, argv + argc, [&key](auto const& t) {
        return t == key;
    });
}

std::vector<std::string> getArgumentsAfterKey(int argc, char* argv[], sv key) {
    std::vector<std::string> args;
    bool keyFound = false;
    for (int i = 1; i != argc; ++i) {
        if (keyFound) {
            if (argv[i][0] == '-' && argv[i][1] == '-') {
                break;
            }
            args.emplace_back(trim(argv[i]));
        } else {
            keyFound = argv[i] == key;
        }
    }

    return args;
}

class RowStorage {
public:
    RowStorage() noexcept = default;

    static inline std::string SLOWDOWN = "Slowdown";

    void store(std::string const& rowName, BenchInfo const& info) noexcept {
        if (auto maybeSuffix = info.getSuffix(rowName); maybeSuffix) {
            m_rowNames.registerName(rowName);
            m_columnNames.registerName(*maybeSuffix);
            auto key = std::make_pair(rowName, *maybeSuffix);
            auto wasAdded = m_table.emplace(key, info).second;
            if (!wasAdded) {
                std::cerr << "Duplicate: " << info << std::endl;
            }
        }
    }

    void store(std::string const& rowName, std::string const& colName, MultInfo mult) noexcept {
        m_rowNames.registerName(rowName);
        m_columnNames.registerName(colName);
        auto key = std::make_pair(rowName, colName);
        auto wasAdded = m_table.emplace(key, mult).second;
        if (!wasAdded) {
            std::cerr << "Duplicate(Mult): " << rowName << "/" << colName << std::endl;
        }
    }

    void setEtalon(std::string const& etalonName) noexcept {
        if (!m_columnNames.hasName(etalonName)) {
            std::cerr << "No etalon row name: " << etalonName << std::endl;
            return;
        }
        store(SLOWDOWN, etalonName, MultInfo{1.});
        for (auto const& col : m_columnNames.getList()) {
            if (col == etalonName) {
                continue;
            }
            MultInfo info;
            for (auto const& row : m_rowNames.getList()) {
                auto etalonCell = getCell(row, etalonName);
                auto cell = getCell(row, col);
                if (etalonCell && cell
                        && std::holds_alternative<BenchInfo>(*etalonCell)
                        && std::holds_alternative<BenchInfo>(*cell)) {
                    double slowdown = (std::get<BenchInfo>(*cell).cpuTime.count()
                            / std::get<BenchInfo>(*etalonCell).cpuTime.count());
                    if (std::isnan(slowdown)) {
                        std::cerr << "Nan" << std::endl;
                        return;
                    }
                    info.mult.emplace_back(slowdown);
                }
            }

            store(SLOWDOWN, col, info);
        }
    }

    static constexpr unsigned WIDTH = 12;
    static constexpr unsigned WIDTH_FIRST_COLUMN = 3;

    std::string print(bool transpose = true) const noexcept {
        auto widths = countWidths(transpose);

        auto const* rows = &m_rowNames.getList();
        auto const* columns = &m_columnNames.getList();
        if (transpose) {
            std::swap(rows, columns);
        }

        std::stringstream os;
        os << "| " << std::left << std::setw(widths[0] - 1) << "*";
        for (auto i = 0; i != columns->size(); ++i) {
            auto const& rn = columns->operator[](i);
            os << "| " << std::setw(widths[i + 1] - 1) << trim(rn, "/_");
        }

        os << "|\n" << std::setfill('-');
        for (auto i = 0; i != columns->size() + 1; ++i) {
            os << "|" << std::setw(widths[i]) << "-";
        }
        os << "|\n" << std::setfill(' ');

        for (auto const& rowName : *rows) {
            os << "| " << std::setw(widths[0] - 1) << trim(rowName, "/_");
            for (auto i = 0; i != columns->size(); ++i) {
                auto const& columnName = columns->operator[](i);
                os << "| " << std::setw(widths[i + 1] - 1);
                if (auto it = getCell(rowName, columnName, transpose); it) {
                    std::visit([&](auto const& t) {
                        os << t.print();
                    }, *it);
                } else {
                    os << "-";
                }
            }
            os << "|\n";
        }
        return os.str();
    }
private:

    std::vector<int> countWidths(bool transpose) const noexcept {
        auto const* rows = &m_rowNames.getList();
        auto const* columns = &m_columnNames.getList();
        if (transpose) {
            std::swap(rows, columns);
        }

        std::vector<int> widths; widths.resize(columns->size() + 1, WIDTH);
        widths[0] = std::accumulate(
                rows->begin(), rows->end(),
                WIDTH_FIRST_COLUMN, [](auto l, auto const& r) {
                    return std::max<int>(l, r.size());
        }) + 2;

        for (auto i = 0; i != columns->size(); ++i) {
            widths[i + 1] = std::max<int>(WIDTH, (*columns)[i].size() + 2);
        }

        return widths;
    }

    using Cell = std::variant<BenchInfo, MultInfo>;

    std::optional<Cell> getCell(std::string const& r, std::string const& c, bool transpose = false) const noexcept {
        auto key = std::make_pair(r, c);
        if (transpose) {
            key = std::make_pair(c, r);
        }
        if (m_table.contains(key)) {
            return m_table.at(key);
        } else {
            return std::nullopt;
        }
    }


    class OrderedNames {
    public:
        void registerName(std::string const& name) noexcept {
            if (m_names.emplace(name).second) {
                m_order.emplace_back(name);
            }
        }

        bool hasName(std::string const& name) {
            return m_names.contains(name);
        }

        std::vector<std::string> const& getList() const noexcept {
            return m_order;
        }
    private:
        std::vector<std::string> m_order;
        std::set<std::string> m_names;
    };

    std::map<std::pair<std::string, std::string>, Cell> m_table;
    OrderedNames m_rowNames;
    OrderedNames m_columnNames;
};

int main(int argc, char* argv[]) {
    bool const agg = hasArgumentKey(argc, argv, "--agg");
    auto rows = getArgumentsAfterKey(argc, argv, "--row");
    auto slowdown = getArgumentsAfterKey(argc, argv, "--slowdown");
    bool const transpose = hasArgumentKey(argc, argv, "--transpose");

    auto parser = infoParser(agg);
    std::vector<BenchInfo> benches;
    for (std::string line; std::getline(std::cin, line);) {
        Stream s{line};
        if (auto parsedInfo = parser(s); !parsedInfo.isError()) {
            benches.emplace_back(parsedInfo.data());
        }
    }

    RowStorage storage;
    for (auto const& row : rows) {
        for (auto const& info : benches) {
            storage.store(row, info);
        }
    }

    if (slowdown.size() == 1) {
        storage.setEtalon(slowdown[0]);
    }

    std::cout << storage.print(transpose) << std::endl;

    return 0;
}