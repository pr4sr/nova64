#pragma once

#include "nova64/common.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace nova64 {
struct symbol_entry {
    std::string name;
    word_t address{};
    std::string section;
};

struct executable {
    struct section {
        std::string name;
        std::vector<byte_t> bytes;
    };
    std::vector<section> sections;
    std::vector<symbol_entry> symbols;
    std::string entry_point;
    word_t entry_address{};
};

class assembler {
public:
    executable assemble(const std::string& source_path);

private:
    struct parsed_line {
        std::string label;
        std::string mnemonic;
        std::vector<std::string> operands;
        std::string original;
    };

    std::string read_file(const std::string& path) const;
    std::vector<std::string> split(const std::string& line) const;
    std::vector<parsed_line> parse_lines(const std::string& source, const std::string& base_path) const;
    std::vector<byte_t> encode_instruction(const std::string& mnemonic, const std::vector<std::string>& operands) const;
    std::optional<word_t> resolve_immediate(const std::string& token, const std::unordered_map<std::string, word_t>& symbols) const;
    std::optional<std::uint8_t> parse_register(const std::string& token) const;
};

class disassembler {
public:
    std::string disassemble(const std::vector<byte_t>& code, word_t base_address = 0) const;
};
}  // namespace nova64
