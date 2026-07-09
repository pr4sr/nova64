#include "nova64/assembler.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>

namespace nova64 {
namespace {
std::string trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return begin < end ? std::string(begin, end) : std::string{};
}
}

std::string assembler::read_file(const std::string& path) const {
    std::ifstream stream(path);
    if (!stream) {
        throw std::runtime_error("cannot open file: " + path);
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::vector<std::string> assembler::split(const std::string& line) const {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<assembler::parsed_line> assembler::parse_lines(const std::string& source, const std::string& base_path) const {
    std::vector<parsed_line> result;
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        const auto comment = line.find(';');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        const auto trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }
        parsed_line parsed;
        parsed.original = trimmed;
        const auto colon = trimmed.find(':');
        if (colon != std::string::npos) {
            parsed.label = trim(trimmed.substr(0, colon));
            parsed.mnemonic = trim(trimmed.substr(colon + 1));
        } else {
            const auto parts = split(trimmed);
            if (parts.empty()) {
                continue;
            }
            parsed.mnemonic = parts[0];
            parsed.operands = std::vector<std::string>(parts.begin() + 1, parts.end());
        }
        if (!parsed.mnemonic.empty()) {
            const auto parts = split(parsed.mnemonic);
            if (parts.size() > 1) {
                parsed.mnemonic = parts[0];
                parsed.operands = std::vector<std::string>(parts.begin() + 1, parts.end());
            }
        }
        result.push_back(parsed);
    }
    return result;
}

std::vector<byte_t> assembler::encode_instruction(const std::string& mnemonic, const std::vector<std::string>& operands) const {
    instruction_t inst{};
    if (mnemonic == "nop") {
        inst.op = opcode::nop;
    } else if (mnemonic == "mov") {
        inst.op = opcode::mov;
        inst.rd = parse_register(operands[0]).value_or(0);
        inst.rs1 = parse_register(operands[1]).value_or(0);
    } else if (mnemonic == "loadi") {
        inst.op = opcode::loadi;
        inst.rd = parse_register(operands[0]).value_or(0);
        inst.imm = static_cast<std::int16_t>(std::stoll(operands[1]));
    } else if (mnemonic == "add") {
        inst.op = opcode::add;
        inst.rd = parse_register(operands[0]).value_or(0);
        inst.rs1 = parse_register(operands[1]).value_or(0);
        inst.rs2 = parse_register(operands[2]).value_or(0);
    } else if (mnemonic == "hlt") {
        inst.op = opcode::hlt;
    } else {
        throw std::runtime_error("unsupported mnemonic: " + mnemonic);
    }
    std::vector<byte_t> bytes(4);
    const auto word = nova64::encode_instruction(inst);
    bytes[0] = static_cast<byte_t>((word >> 24) & 0xFF);
    bytes[1] = static_cast<byte_t>((word >> 16) & 0xFF);
    bytes[2] = static_cast<byte_t>((word >> 8) & 0xFF);
    bytes[3] = static_cast<byte_t>(word & 0xFF);
    return bytes;
}

std::optional<word_t> assembler::resolve_immediate(const std::string& token, const std::unordered_map<std::string, word_t>& symbols) const {
    if (token.empty()) {
        return std::nullopt;
    }
    if (symbols.count(token)) {
        return symbols.at(token);
    }
    try {
        return static_cast<word_t>(std::stoull(token));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<std::uint8_t> assembler::parse_register(const std::string& token) const {
    if (token.rfind('r', 0) == 0) {
        return static_cast<std::uint8_t>(std::stoi(token.substr(1)));
    }
    return std::nullopt;
}

executable assembler::assemble(const std::string& source_path) {
    const auto source = read_file(source_path);
    executable exe;
    executable::section text{};
    text.name = ".text";

    const auto lines = parse_lines(source, source_path);
    std::unordered_map<std::string, word_t> symbols;
    std::size_t offset = 0;
    for (const auto& line : lines) {
        if (!line.label.empty()) {
            symbols[line.label] = static_cast<word_t>(offset);
        }
        if (!line.mnemonic.empty()) {
            auto bytes = encode_instruction(line.mnemonic, line.operands);
            text.bytes.insert(text.bytes.end(), bytes.begin(), bytes.end());
            offset += bytes.size();
        }
    }
    exe.sections.push_back(text);
    exe.entry_point = "_start";
    exe.entry_address = 0x1000;
    return exe;
}

std::string disassembler::disassemble(const std::vector<byte_t>& code, word_t base_address) const {
    std::ostringstream out;
    for (std::size_t i = 0; i < code.size(); i += 4) {
        if (i + 3 >= code.size()) {
            break;
        }
        const auto word = static_cast<std::uint32_t>(code[i]) << 24 |
                          static_cast<std::uint32_t>(code[i + 1]) << 16 |
                          static_cast<std::uint32_t>(code[i + 2]) << 8 |
                          static_cast<std::uint32_t>(code[i + 3]);
        const instruction_t inst = decode_instruction(word);
        out << "0x" << std::hex << (base_address + i) << ": " << opcode_name(inst.op) << "\n";
    }
    return out.str();
}
}  // namespace nova64
