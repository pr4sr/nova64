#include "nova64/assembler.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace nova64 {
namespace {
std::string trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return begin < end ? std::string(begin, end) : std::string{};
}

std::string process_escapes(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            switch (value[i + 1]) {
                case 'n': out.push_back('\n'); ++i; break;
                case 'r': out.push_back('\r'); ++i; break;
                case 't': out.push_back('\t'); ++i; break;
                case '\\': out.push_back('\\'); ++i; break;
                case '"': out.push_back('"'); ++i; break;
                case '0': out.push_back('\0'); ++i; break;
                default: out.push_back(value[i]); break;
            }
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

std::string strip_commas(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch != ',') {
            out.push_back(ch);
        }
    }
    return out;
}

constexpr std::uint8_t kBranchTempReg = 30;
constexpr std::uint8_t kImmTempReg = 31;

bool fits_signed9(std::int64_t value) {
    return value >= -256 && value <= 255;
}
}  // namespace

std::string assembler::read_file(const std::string& path) const {
    std::ifstream stream(path);
    if (!stream) {
        throw std::runtime_error("cannot open file: " + path);
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::vector<std::string> assembler::split_operands(const std::string& line) const {
    std::istringstream iss(strip_commas(line));
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> assembler::tokenize_operand_list(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;
    bool in_brackets = false;
    for (char ch : strip_commas(text)) {
        if (ch == '[') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            in_brackets = true;
            current.push_back(ch);
        } else if (ch == ']') {
            current.push_back(ch);
            tokens.push_back(current);
            current.clear();
            in_brackets = false;
        } else if (std::isspace(static_cast<unsigned char>(ch)) != 0 && !in_brackets) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::vector<assembler::parsed_line> assembler::parse_lines(const std::string& source) const {
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

        std::string body = trimmed;
        std::size_t colon = std::string::npos;
        bool in_quotes = false;
        for (std::size_t i = 0; i < trimmed.size(); ++i) {
            if (trimmed[i] == '"') {
                in_quotes = !in_quotes;
                continue;
            }
            if (!in_quotes && trimmed[i] == ':') {
                colon = i;
                break;
            }
        }
        if (colon != std::string::npos) {
            parsed.label = trim(trimmed.substr(0, colon));
            body = trim(trimmed.substr(colon + 1));
        }

        if (body.empty()) {
            if (!parsed.label.empty()) {
                result.push_back(parsed);
            }
            continue;
        }

        if (body.rfind('.', 0) == 0) {
            const auto space = body.find(' ');
            const auto directive = space == std::string::npos ? body : body.substr(0, space);
            parsed.directive = directive;
            if (directive == ".asciz") {
                parsed.operands = {trim(body.substr(directive.size()))};
            } else {
                const auto parts = split_operands(body);
                parsed.operands = std::vector<std::string>(parts.begin() + 1, parts.end());
            }
            result.push_back(parsed);
            continue;
        }

        const auto parts = split_operands(body);
        if (parts.empty()) {
            continue;
        }
        parsed.mnemonic = parts[0];
        parsed.operands = std::vector<std::string>(parts.begin() + 1, parts.end());
        result.push_back(parsed);
    }
    return result;
}

std::optional<std::uint8_t> assembler::parse_register(const std::string& token) const {
    if (token.rfind('r', 0) == 0 && token.size() > 1) {
        const auto index = std::stoi(token.substr(1));
        if (index < 0 || index > 31) {
            throw std::runtime_error("register out of range: " + token);
        }
        return static_cast<std::uint8_t>(index);
    }
    return std::nullopt;
}

std::optional<word_t> assembler::resolve_immediate(const std::string& token, const std::unordered_map<std::string, word_t>& symbols) const {
    if (token.empty()) {
        return std::nullopt;
    }
    if (symbols.count(token)) {
        return symbols.at(token);
    }
    try {
        std::size_t consumed = 0;
        const auto value = std::stoull(token, &consumed, 0);
        if (consumed != token.size()) {
            return std::nullopt;
        }
        return static_cast<word_t>(value);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::pair<std::uint8_t, std::int16_t> assembler::parse_memory_operand(const std::string& token,
                                                                      const std::unordered_map<std::string, word_t>& symbols) const {
    if (token.size() < 3 || token.front() != '[' || token.back() != ']') {
        throw std::runtime_error("invalid memory operand: " + token);
    }
    const auto inner = trim(token.substr(1, token.size() - 2));
    const auto plus = inner.find('+');
    if (plus == std::string::npos) {
        const auto reg = parse_register(inner);
        if (!reg) {
            throw std::runtime_error("invalid memory base: " + token);
        }
        return {*reg, static_cast<std::int16_t>(0)};
    }
    const auto base = trim(inner.substr(0, plus));
    const auto offset = trim(inner.substr(plus + 1));
    const auto reg = parse_register(base);
    if (!reg) {
        throw std::runtime_error("invalid memory base: " + token);
    }
    const auto imm = resolve_immediate(offset, symbols);
    if (!imm || !fits_signed9(static_cast<std::int64_t>(*imm))) {
        throw std::runtime_error("memory offset out of range: " + token);
    }
    return {*reg, static_cast<std::int16_t>(*imm)};
}

std::vector<std::uint32_t> assembler::expand_loadimm(std::uint8_t rd, word_t value) const {
    if (fits_signed9(static_cast<std::int64_t>(value))) {
        instruction_t inst{};
        inst.op = opcode::loadi;
        inst.rd = rd;
        inst.imm = static_cast<std::int16_t>(static_cast<std::int64_t>(value));
        return {encode_instruction(inst)};
    }

    std::vector<std::uint32_t> words;
    const std::uint8_t scratch = rd == kImmTempReg ? kBranchTempReg : kImmTempReg;
    const std::uint8_t mask_reg = (rd == 28) ? 29 : 28;

    instruction_t zero{};
    zero.op = opcode::loadi;
    zero.rd = rd;
    zero.imm = 0;
    words.push_back(encode_instruction(zero));

    {
        instruction_t neg_one{};
        neg_one.op = opcode::loadi;
        neg_one.rd = mask_reg;
        neg_one.imm = -1;
        words.push_back(encode_instruction(neg_one));

        instruction_t fifty_five{};
        fifty_five.op = opcode::loadi;
        fifty_five.rd = scratch;
        fifty_five.imm = 55;
        words.push_back(encode_instruction(fifty_five));

        instruction_t make_mask{};
        make_mask.op = opcode::shr;
        make_mask.rd = mask_reg;
        make_mask.rs1 = mask_reg;
        make_mask.rs2 = scratch;
        words.push_back(encode_instruction(make_mask));
    }

    for (int shift = 54; shift >= 0; shift -= 9) {
        const auto chunk = static_cast<std::int16_t>((value >> shift) & 0x1FF);
        const auto signed_chunk = static_cast<std::int16_t>((chunk << 7) >> 7);
        if (signed_chunk != 0) {
            instruction_t piece{};
            piece.op = opcode::loadi;
            piece.rd = scratch;
            piece.imm = signed_chunk;
            words.push_back(encode_instruction(piece));

            instruction_t zero_ext{};
            zero_ext.op = opcode::and_;
            zero_ext.rd = scratch;
            zero_ext.rs1 = scratch;
            zero_ext.rs2 = mask_reg;
            words.push_back(encode_instruction(zero_ext));

            instruction_t merge{};
            merge.op = opcode::or_;
            merge.rd = rd;
            merge.rs1 = rd;
            merge.rs2 = scratch;
            words.push_back(encode_instruction(merge));
        }
        if (shift > 0) {
            instruction_t nine{};
            nine.op = opcode::loadi;
            nine.rd = scratch;
            nine.imm = 9;
            words.push_back(encode_instruction(nine));

            instruction_t shift_inst{};
            shift_inst.op = opcode::shl;
            shift_inst.rd = rd;
            shift_inst.rs1 = rd;
            shift_inst.rs2 = scratch;
            words.push_back(encode_instruction(shift_inst));
        }
    }
    return words;
}

std::vector<std::uint32_t> assembler::encode_mnemonic(const std::string& mnemonic, const std::vector<std::string>& operands,
                                                     const std::unordered_map<std::string, word_t>& symbols,
                                                     word_t address) const {
    (void)address;
    instruction_t inst{};
    std::vector<std::uint32_t> words;

    auto require = [&](std::size_t count) {
        if (operands.size() != count) {
            throw std::runtime_error("operand count mismatch for " + mnemonic);
        }
    };

    auto reg = [&](std::size_t index) {
        const auto value = parse_register(operands[index]);
        if (!value) {
            throw std::runtime_error("expected register: " + operands[index]);
        }
        return *value;
    };

    auto imm = [&](std::size_t index) {
        const auto value = resolve_immediate(operands[index], symbols);
        if (!value) {
            throw std::runtime_error("invalid immediate: " + operands[index]);
        }
        if (!fits_signed9(static_cast<std::int64_t>(*value))) {
            throw std::runtime_error("immediate out of 9-bit range: " + operands[index]);
        }
        return static_cast<std::int16_t>(*value);
    };

    auto branch_to = [&](opcode branch_op) {
        require(1);
        const auto target = resolve_immediate(operands[0], symbols);
        if (!target) {
            throw std::runtime_error("unknown branch target: " + operands[0]);
        }
        words = expand_loadimm(kBranchTempReg, *target);
        instruction_t branch{};
        branch.op = branch_op;
        branch.rs1 = kBranchTempReg;
        branch.imm = 0;
        words.push_back(encode_instruction(branch));
        return words;
    };

    auto jump_to = [&](opcode jump_op) {
        require(1);
        if (auto reg_op = parse_register(operands[0])) {
            instruction_t jump{};
            jump.op = jump_op;
            jump.rs1 = *reg_op;
            jump.imm = 0;
            return std::vector<std::uint32_t>{encode_instruction(jump)};
        }
        const auto target = resolve_immediate(operands[0], symbols);
        if (!target) {
            throw std::runtime_error("unknown jump target: " + operands[0]);
        }
        words = expand_loadimm(kBranchTempReg, *target);
        instruction_t jump{};
        jump.op = jump_op;
        jump.rs1 = kBranchTempReg;
        jump.imm = 0;
        words.push_back(encode_instruction(jump));
        return words;
    };

    if (mnemonic == "nop") {
        inst.op = opcode::nop;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "mov") {
        require(2);
        inst.op = opcode::mov;
        inst.rd = reg(0);
        inst.rs1 = reg(1);
        return {encode_instruction(inst)};
    }
    if (mnemonic == "loadi") {
        require(2);
        const auto value = resolve_immediate(operands[1], symbols);
        if (!value) {
            throw std::runtime_error("invalid loadi immediate: " + operands[1]);
        }
        return expand_loadimm(reg(0), *value);
    }
    if (mnemonic == "add" || mnemonic == "sub" || mnemonic == "mul" || mnemonic == "div" || mnemonic == "and" ||
        mnemonic == "or" || mnemonic == "xor" || mnemonic == "shl" || mnemonic == "shr" || mnemonic == "rol" ||
        mnemonic == "ror") {
        require(3);
        if (mnemonic == "add") {
            inst.op = opcode::add;
        } else if (mnemonic == "sub") {
            inst.op = opcode::sub;
        } else if (mnemonic == "mul") {
            inst.op = opcode::mul;
        } else if (mnemonic == "div") {
            inst.op = opcode::div;
        } else if (mnemonic == "and") {
            inst.op = opcode::and_;
        } else if (mnemonic == "or") {
            inst.op = opcode::or_;
        } else if (mnemonic == "xor") {
            inst.op = opcode::xor_;
        } else if (mnemonic == "shl") {
            inst.op = opcode::shl;
        } else if (mnemonic == "shr") {
            inst.op = opcode::shr;
        } else if (mnemonic == "rol") {
            inst.op = opcode::rol;
        } else {
            inst.op = opcode::ror;
        }
        inst.rd = reg(0);
        inst.rs1 = reg(1);
        inst.rs2 = reg(2);
        return {encode_instruction(inst)};
    }
    if (mnemonic == "load") {
        require(2);
        inst.op = opcode::load;
        inst.rd = reg(0);
        const auto mem = parse_memory_operand(operands[1], symbols);
        inst.rs1 = mem.first;
        inst.imm = mem.second;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "store") {
        require(2);
        inst.op = opcode::store;
        inst.rd = reg(0);
        const auto mem = parse_memory_operand(operands[1], symbols);
        inst.rs1 = mem.first;
        inst.imm = mem.second;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "push") {
        require(1);
        inst.op = opcode::push;
        inst.rd = reg(0);
        return {encode_instruction(inst)};
    }
    if (mnemonic == "pop") {
        require(1);
        inst.op = opcode::pop;
        inst.rd = reg(0);
        return {encode_instruction(inst)};
    }
    if (mnemonic == "jmp") {
        return jump_to(opcode::jmp);
    }
    if (mnemonic == "jeq") {
        return branch_to(opcode::jeq);
    }
    if (mnemonic == "jne") {
        return branch_to(opcode::jne);
    }
    if (mnemonic == "jlt") {
        return branch_to(opcode::jlt);
    }
    if (mnemonic == "jgt") {
        return branch_to(opcode::jgt);
    }
    if (mnemonic == "call") {
        return jump_to(opcode::call);
    }
    if (mnemonic == "ret") {
        inst.op = opcode::ret;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "syscall") {
        inst.op = opcode::syscall;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "hlt") {
        inst.op = opcode::hlt;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "dbg") {
        inst.op = opcode::dbg;
        return {encode_instruction(inst)};
    }
    if (mnemonic == "csrr") {
        require(2);
        inst.op = opcode::csrr;
        inst.rd = reg(0);
        inst.rs1 = static_cast<std::uint8_t>(imm(1));
        return {encode_instruction(inst)};
    }
    if (mnemonic == "csrw") {
        require(2);
        inst.op = opcode::csrw;
        inst.rd = static_cast<std::uint8_t>(imm(0));
        inst.rs1 = reg(1);
        return {encode_instruction(inst)};
    }
    if (mnemonic == "eret") {
        inst.op = opcode::eret;
        return {encode_instruction(inst)};
    }

    throw std::runtime_error("unsupported mnemonic: " + mnemonic);
}

executable assembler::assemble(const std::string& source_path) {
    const auto source = read_file(source_path);
    executable exe;
    executable::section text{};
    text.name = ".text";

    const auto lines = parse_lines(source);
    std::unordered_map<std::string, word_t> symbols;
    symbols["epc"] = 0;
    symbols["eha"] = 1;
    symbols["ecause"] = 2;
    word_t address = 0x1000;

    auto instruction_size = [&](const parsed_line& line) -> word_t {
        if (!line.directive.empty()) {
            if (line.directive == ".org") {
                return 0;
            }
            if (line.directive == ".byte") {
                return static_cast<word_t>(line.operands.size());
            }
            if (line.directive == ".word") {
                return static_cast<word_t>(line.operands.size() * 4);
            }
            if (line.directive == ".quad") {
                return static_cast<word_t>(line.operands.size() * 8);
            }
            if (line.directive == ".asciz") {
                std::string literal;
                for (std::size_t i = 0; i < line.operands.size(); ++i) {
                    if (i > 0) {
                        literal.push_back(' ');
                    }
                    literal += line.operands[i];
                }
                if (literal.size() >= 2 && literal.front() == '"' && literal.back() == '"') {
                    literal = literal.substr(1, literal.size() - 2);
                }
                literal = process_escapes(literal);
                return static_cast<word_t>(literal.size() + 1);
            }
            if (line.directive == ".include") {
                if (line.operands.size() != 1) {
                    throw std::runtime_error(".include requires one operand");
                }
                const auto included = assemble(line.operands[0]);
                return included.sections.empty() ? 0 : static_cast<word_t>(included.sections.front().bytes.size());
            }
            throw std::runtime_error("unknown directive: " + line.directive);
        }
        if (!line.mnemonic.empty()) {
            if (line.mnemonic == "loadi" && line.operands.size() == 2) {
                const auto value = resolve_immediate(line.operands[1], symbols);
                const word_t guessed = value ? *value : (address + 512);
                const auto rd = parse_register(line.operands[0]).value_or(0);
                return static_cast<word_t>(expand_loadimm(rd, guessed).size() * 4);
            }
            if (line.mnemonic == "jmp" || line.mnemonic == "call") {
                if (line.operands.size() != 1) {
                    throw std::runtime_error("branch instruction requires one operand");
                }
                if (parse_register(line.operands[0])) {
                    return 4;
                }
                const auto target = resolve_immediate(line.operands[0], symbols);
                const word_t guessed = target ? *target : 0x1000;
                return static_cast<word_t>((expand_loadimm(kBranchTempReg, guessed).size() + 1) * 4);
            }
            if (line.mnemonic == "jeq" || line.mnemonic == "jne" || line.mnemonic == "jlt" ||
                line.mnemonic == "jgt") {
                if (line.operands.size() != 1) {
                    throw std::runtime_error("branch instruction requires one operand");
                }
                const auto target = resolve_immediate(line.operands[0], symbols);
                const word_t guessed = target ? *target : 0x1000;
                return static_cast<word_t>((expand_loadimm(kBranchTempReg, guessed).size() + 1) * 4);
            }
            return static_cast<word_t>(encode_mnemonic(line.mnemonic, line.operands, symbols, address).size() * 4);
        }
        return 0;
    };

    for (int iteration = 0; iteration < 8; ++iteration) {
        address = 0x1000;
        std::unordered_map<std::string, word_t> next_symbols;
        next_symbols["epc"] = 0;
        next_symbols["eha"] = 1;
        next_symbols["ecause"] = 2;
        for (const auto& line : lines) {
            if (!line.label.empty()) {
                next_symbols[line.label] = address;
            }

            if (!line.directive.empty() && line.directive == ".org") {
                if (line.operands.size() != 1) {
                    throw std::runtime_error(".org requires one operand");
                }
                const auto value = resolve_immediate(line.operands[0], next_symbols);
                if (!value) {
                    throw std::runtime_error("invalid .org address");
                }
                if (*value < address) {
                    throw std::runtime_error(".org address moves backwards");
                }
                address = *value;
                continue;
            }

            address += instruction_size(line);
        }
        if (next_symbols == symbols) {
            break;
        }
        symbols = std::move(next_symbols);
    }

    address = 0x1000;
    auto emit_words = [&](const std::vector<std::uint32_t>& words) {
        for (const auto word : words) {
            const auto bytes = instruction_to_bytes(word);
            text.bytes.insert(text.bytes.end(), bytes.begin(), bytes.end());
            address += 4;
        }
    };

    auto emit_data64 = [&](word_t value) {
        const auto bytes = instruction_to_bytes(static_cast<std::uint32_t>(value & 0xFFFFFFFF));
        text.bytes.insert(text.bytes.end(), bytes.begin(), bytes.end());
        const auto high = instruction_to_bytes(static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFF));
        text.bytes.insert(text.bytes.end(), high.begin(), high.end());
        address += 8;
    };

    for (const auto& line : lines) {
        if (!line.directive.empty() && line.directive == ".org") {
            const auto value = resolve_immediate(line.operands[0], symbols);
            if (*value < address) {
                throw std::runtime_error(".org address moves backwards");
            }
            address = *value;
            continue;
        }

        if (!line.directive.empty()) {
            if (line.directive == ".byte") {
                for (const auto& operand : line.operands) {
                    const auto value = resolve_immediate(operand, symbols);
                    if (!value || *value > 0xFF) {
                        throw std::runtime_error("invalid .byte value");
                    }
                    text.bytes.push_back(static_cast<byte_t>(*value));
                    ++address;
                }
                continue;
            }
            if (line.directive == ".word") {
                for (const auto& operand : line.operands) {
                    const auto value = resolve_immediate(operand, symbols);
                    if (!value) {
                        throw std::runtime_error("invalid .word value");
                    }
                    const auto bytes = instruction_to_bytes(static_cast<std::uint32_t>(*value));
                    text.bytes.insert(text.bytes.end(), bytes.begin(), bytes.end());
                    address += 4;
                }
                continue;
            }
            if (line.directive == ".quad") {
                for (const auto& operand : line.operands) {
                    const auto value = resolve_immediate(operand, symbols);
                    if (!value) {
                        throw std::runtime_error("invalid .quad value");
                    }
                    emit_data64(*value);
                }
                continue;
            }
            if (line.directive == ".asciz") {
                std::string literal;
                for (std::size_t i = 0; i < line.operands.size(); ++i) {
                    if (i > 0) {
                        literal.push_back(' ');
                    }
                    literal += line.operands[i];
                }
                if (literal.size() >= 2 && literal.front() == '"' && literal.back() == '"') {
                    literal = literal.substr(1, literal.size() - 2);
                }
                literal = process_escapes(literal);
                for (char ch : literal) {
                    text.bytes.push_back(static_cast<byte_t>(ch));
                    ++address;
                }
                text.bytes.push_back(0);
                ++address;
                continue;
            }
            if (line.directive == ".include") {
                const auto included = assemble(line.operands[0]);
                if (!included.sections.empty()) {
                    text.bytes.insert(text.bytes.end(), included.sections.front().bytes.begin(),
                                      included.sections.front().bytes.end());
                    address += included.sections.front().bytes.size();
                    for (const auto& symbol : included.symbols) {
                        symbols[symbol.name] = symbol.address;
                        exe.symbols.push_back(symbol);
                    }
                }
                continue;
            }
            throw std::runtime_error("unknown directive: " + line.directive);
        }

        if (!line.mnemonic.empty()) {
            emit_words(encode_mnemonic(line.mnemonic, line.operands, symbols, address));
        }
    }

    for (const auto& [name, value] : symbols) {
        const bool already = std::any_of(exe.symbols.begin(), exe.symbols.end(),
                                         [&](const symbol_entry& entry) { return entry.name == name; });
        if (!already) {
            exe.symbols.push_back(symbol_entry{name, value, ".text"});
        }
    }

    exe.sections.push_back(text);
    exe.entry_point = symbols.count("_start") ? "_start" : (symbols.count("start") ? "start" : "");
    exe.entry_address = symbols.count("_start") ? symbols.at("_start") : (symbols.count("start") ? symbols.at("start") : 0x1000);
    return exe;
}

std::string disassembler::disassemble(const std::vector<byte_t>& code, word_t base_address) const {
    std::ostringstream out;
    for (std::size_t i = 0; i + 3 < code.size(); i += 4) {
        const auto word = static_cast<std::uint32_t>(code[i]) | (static_cast<std::uint32_t>(code[i + 1]) << 8) |
                          (static_cast<std::uint32_t>(code[i + 2]) << 16) | (static_cast<std::uint32_t>(code[i + 3]) << 24);
        const instruction_t inst = decode_instruction(word);
        out << "0x" << std::hex << (base_address + i) << ": " << opcode_name(inst.op) << " rd=" << std::dec
            << static_cast<int>(inst.rd) << " rs1=" << static_cast<int>(inst.rs1) << " rs2=" << static_cast<int>(inst.rs2)
            << " imm=" << inst.imm << "\n";
    }
    return out.str();
}
}  // namespace nova64
