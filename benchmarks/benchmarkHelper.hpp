#pragma once

#include <random>
#include <fstream>


inline unsigned getRandomNumber(unsigned l, unsigned r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned> distrib(l, r);
    return distrib(gen);
}

inline std::string readFile(std::string const& filename) noexcept(false) {
    std::ifstream file{filename};
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file");
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
