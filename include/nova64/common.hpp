#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nova64 {
using byte_t = std::uint8_t;
using word_t = std::uint64_t;
using s_word_t = std::int64_t;

constexpr std::uint64_t kMemorySize = 16ull * 1024 * 1024;
constexpr std::uint64_t kStackBase = 0x0000'0000'00F0'0000ULL;
constexpr std::uint64_t kKernelBase = 0x0000'0000'0010'0000ULL;
constexpr std::uint64_t kConsoleAddr = 0xF000'0000ULL;
constexpr std::uint64_t kTimerAddr = 0xF000'0010ULL;
constexpr std::uint64_t kRngAddr = 0xF000'0020ULL;
constexpr std::uint64_t kRtcAddr = 0xF000'0030ULL;
constexpr std::uint64_t kStorageAddr = 0xF000'0040ULL;

enum class opcode : std::uint8_t {
    nop = 0x00,
    mov = 0x01,
    loadi = 0x02,
    add = 0x03,
    sub = 0x04,
    mul = 0x05,
    div = 0x06,
    and_ = 0x07,
    or_ = 0x08,
    xor_ = 0x09,
    shl = 0x0A,
    shr = 0x0B,
    rol = 0x0C,
    ror = 0x0D,
    load = 0x10,
    store = 0x11,
    push = 0x12,
    pop = 0x13,
    jmp = 0x20,
    jeq = 0x21,
    jne = 0x22,
    jlt = 0x23,
    jgt = 0x24,
    call = 0x25,
    ret = 0x26,
    syscall = 0x30,
    hlt = 0x31,
    dbg = 0x32
};

struct flags_t {
    bool zero = false;
    bool sign = false;
    bool carry = false;
    bool overflow = false;
};

struct instruction_t {
    opcode op{};
    std::uint8_t rd{};
    std::uint8_t rs1{};
    std::uint8_t rs2{};
    std::int16_t imm{};
};

inline std::uint32_t encode_instruction(instruction_t inst) {
    return (static_cast<std::uint32_t>(inst.op) << 24) |
           (static_cast<std::uint32_t>(inst.rd) << 19) |
           (static_cast<std::uint32_t>(inst.rs1) << 14) |
           (static_cast<std::uint32_t>(inst.rs2) << 9) |
           (static_cast<std::uint32_t>(static_cast<std::int16_t>(inst.imm) & 0x1FF));
}

inline instruction_t decode_instruction(std::uint32_t word) {
    instruction_t inst{};
    inst.op = static_cast<opcode>((word >> 24) & 0xFF);
    inst.rd = static_cast<std::uint8_t>((word >> 19) & 0x1F);
    inst.rs1 = static_cast<std::uint8_t>((word >> 14) & 0x1F);
    inst.rs2 = static_cast<std::uint8_t>((word >> 9) & 0x1F);
    inst.imm = static_cast<std::int16_t>(word & 0x1FF);
    return inst;
}

inline std::string opcode_name(opcode op) {
    switch (op) {
        case opcode::nop: return "nop";
        case opcode::mov: return "mov";
        case opcode::loadi: return "loadi";
        case opcode::add: return "add";
        case opcode::sub: return "sub";
        case opcode::mul: return "mul";
        case opcode::div: return "div";
        case opcode::and_: return "and";
        case opcode::or_: return "or";
        case opcode::xor_: return "xor";
        case opcode::shl: return "shl";
        case opcode::shr: return "shr";
        case opcode::rol: return "rol";
        case opcode::ror: return "ror";
        case opcode::load: return "load";
        case opcode::store: return "store";
        case opcode::push: return "push";
        case opcode::pop: return "pop";
        case opcode::jmp: return "jmp";
        case opcode::jeq: return "jeq";
        case opcode::jne: return "jne";
        case opcode::jlt: return "jlt";
        case opcode::jgt: return "jgt";
        case opcode::call: return "call";
        case opcode::ret: return "ret";
        case opcode::syscall: return "syscall";
        case opcode::hlt: return "hlt";
        case opcode::dbg: return "dbg";
    }
    return "unknown";
}
}  // namespace nova64
