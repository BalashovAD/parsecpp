#include <parsecpp/fwd.hpp>

class ForwardTest {
public:
    ForwardTest() noexcept;
    bool tryParse(std::string_view s) const;
private:
    prs::Pimpl<char> m_pimpl;
};