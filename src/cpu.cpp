#include "nova64/cpu.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace nova64 {
memory::memory(std::size_t size) : data_(size, 0) {}

byte_t memory::read8(word_t address) const {
    if (address >= data_.size()) {
        throw std::out_of_range("memory read8 out of range");
    }
    if (mmio_handler_) {
        mmio_handler_(address, 0, false);
    }
    return data_[address];
}

std::uint16_t memory::read16(word_t address) const {
    return static_cast<std::uint16_t>((read8(address) | (read8(address + 1) << 8)) & 0xFFFF);
}

std::uint32_t memory::read32(word_t address) const {
    return static_cast<std::uint32_t>((read8(address) | (read8(address + 1) << 8) | (read8(address + 2) << 16) | (read8(address + 3) << 24)) & 0xFFFFFFFF);
}

word_t memory::read64(word_t address) const {
    return static_cast<word_t>(read32(address)) | (static_cast<word_t>(read32(address + 4)) << 32);
}

void memory::write8(word_t address, byte_t value) {
    if (mmio_handler_) {
        mmio_handler_(address, value, true);
    }
    if (address >= data_.size()) {
        throw std::out_of_range("memory write8 out of range");
    }
    data_[address] = value;
}

void memory::write16(word_t address, std::uint16_t value) {
    write8(address, static_cast<byte_t>(value & 0xFF));
    write8(address + 1, static_cast<byte_t>((value >> 8) & 0xFF));
}

void memory::write32(word_t address, std::uint32_t value) {
    write8(address, static_cast<byte_t>(value & 0xFF));
    write8(address + 1, static_cast<byte_t>((value >> 8) & 0xFF));
    write8(address + 2, static_cast<byte_t>((value >> 16) & 0xFF));
    write8(address + 3, static_cast<byte_t>((value >> 24) & 0xFF));
}

void memory::write64(word_t address, word_t value) {
    write32(address, static_cast<std::uint32_t>(value & 0xFFFFFFFF));
    write32(address + 4, static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFF));
}

void memory::load_bytes(word_t base, const std::vector<byte_t>& data) {
    for (std::size_t i = 0; i < data.size(); ++i) {
        write8(base + i, data[i]);
    }
}

void memory::clear() { std::fill(data_.begin(), data_.end(), byte_t{0}); }

std::size_t memory::size() const { return data_.size(); }

std::vector<byte_t> memory::dump(word_t start, std::size_t length) const {
    std::vector<byte_t> result;
    result.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        result.push_back(read8(start + i));
    }
    return result;
}

void memory::set_mmio_handler(mmio_handler handler) { mmio_handler_ = std::move(handler); }

cpu::cpu(memory& mem) : mem_(mem) { reset(); }

void cpu::reset() {
    gpr_.fill(0);
    pc_ = 0x1000;
    executed_steps_ = 0;
    max_steps_ = 100000;
    sp_ = kStackBase;
    lr_ = 0;
    flags_ = 0;
    privilege_ = 0;
    cycles_ = 0;
    running_ = true;
    trace_enabled_ = false;
    trace_log_.clear();
}

bool cpu::step() {
    if (!running_) {
        return false;
    }

    if (breakpoints_.count(pc_) && breakpoints_.at(pc_)) {
        running_ = false;
        return false;
    }

    if (max_steps_ != 0 && executed_steps_ >= max_steps_) {
        running_ = false;
        return false;
    }

    if (pc_ >= kMemorySize || pc_ + 4 > kMemorySize) {
        running_ = false;
        return false;
    }

    const auto inst_word = mem_.read32(pc_);
    const instruction_t inst = decode_instruction(inst_word);
    pc_ += 4;
    ++cycles_;
    ++executed_steps_;
    const bool ok = execute_instruction(inst);
    if (!ok) {
        running_ = false;
    }
    if (trace_enabled_) {
        trace_log_.push_back(opcode_name(inst.op) + " @ " + std::to_string(pc_));
    }
    return ok;
}

void cpu::set_syscall_handler(std::function<void(cpu&, memory&)> handler) { syscall_handler_ = std::move(handler); }
void cpu::set_breakpoint(word_t address, bool enabled) { breakpoints_[address] = enabled; }
bool cpu::has_breakpoint(word_t address) const { return breakpoints_.count(address) && breakpoints_.at(address); }
void cpu::set_trace(bool enabled) { trace_enabled_ = enabled; }
bool cpu::trace() const { return trace_enabled_; }
void cpu::set_max_steps(std::uint64_t max_steps) { max_steps_ = max_steps; }
std::uint64_t cpu::max_steps() const { return max_steps_; }
word_t cpu::pc() const { return pc_; }
word_t cpu::sp() const { return sp_; }
word_t cpu::lr() const { return lr_; }
word_t cpu::flags() const { return flags_; }
std::array<word_t, 32> cpu::registers() const { return gpr_; }
word_t cpu::read_register(std::uint8_t index) const { return gpr_[index]; }
void cpu::write_register(std::uint8_t index, word_t value) { gpr_[index] = value; }
word_t cpu::cycle_count() const { return cycles_; }
bool cpu::running() const { return running_; }
std::vector<std::string> cpu::trace_log() const { return trace_log_; }

void cpu::update_flags(word_t result, word_t lhs, word_t rhs, bool subtract) {
    flags_t f{};
    f.zero = (result == 0);
    f.sign = (result & (1ULL << 63)) != 0;
    f.carry = subtract ? (lhs < rhs) : (lhs > (std::numeric_limits<word_t>::max() - rhs));
    f.overflow = false;
    flags_ = (f.zero ? 1ULL : 0) | (f.sign ? 2ULL : 0) | (f.carry ? 4ULL : 0) | (f.overflow ? 8ULL : 0);
}

bool cpu::execute_instruction(const instruction_t& inst) {
    switch (inst.op) {
        case opcode::nop:
            break;
        case opcode::mov:
            gpr_[inst.rd] = gpr_[inst.rs1];
            break;
        case opcode::loadi:
            gpr_[inst.rd] = static_cast<word_t>(inst.imm);
            break;
        case opcode::add:
            gpr_[inst.rd] = gpr_[inst.rs1] + gpr_[inst.rs2];
            update_flags(gpr_[inst.rd], gpr_[inst.rs1], gpr_[inst.rs2], false);
            break;
        case opcode::sub:
            gpr_[inst.rd] = gpr_[inst.rs1] - gpr_[inst.rs2];
            update_flags(gpr_[inst.rd], gpr_[inst.rs1], gpr_[inst.rs2], true);
            break;
        case opcode::mul:
            gpr_[inst.rd] = gpr_[inst.rs1] * gpr_[inst.rs2];
            break;
        case opcode::div:
            if (gpr_[inst.rs2] == 0) {
                return false;
            }
            gpr_[inst.rd] = gpr_[inst.rs1] / gpr_[inst.rs2];
            break;
        case opcode::and_:
            gpr_[inst.rd] = gpr_[inst.rs1] & gpr_[inst.rs2];
            break;
        case opcode::or_:
            gpr_[inst.rd] = gpr_[inst.rs1] | gpr_[inst.rs2];
            break;
        case opcode::xor_:
            gpr_[inst.rd] = gpr_[inst.rs1] ^ gpr_[inst.rs2];
            break;
        case opcode::shl:
            gpr_[inst.rd] = gpr_[inst.rs1] << gpr_[inst.rs2];
            break;
        case opcode::shr:
            gpr_[inst.rd] = gpr_[inst.rs1] >> gpr_[inst.rs2];
            break;
        case opcode::rol: {
            const auto amount = static_cast<unsigned int>(gpr_[inst.rs2] & 63);
            const auto value = gpr_[inst.rs1];
            gpr_[inst.rd] = (value << amount) | (value >> ((64 - amount) & 63));
            break;
        }
        case opcode::ror: {
            const auto amount = static_cast<unsigned int>(gpr_[inst.rs2] & 63);
            const auto value = gpr_[inst.rs1];
            gpr_[inst.rd] = (value >> amount) | (value << ((64 - amount) & 63));
            break;
        }
        case opcode::load:
            gpr_[inst.rd] = mem_.read64(gpr_[inst.rs1] + inst.imm);
            break;
        case opcode::store:
            mem_.write64(gpr_[inst.rs1] + inst.imm, gpr_[inst.rd]);
            break;
        case opcode::push:
            sp_ -= 8;
            mem_.write64(sp_, gpr_[inst.rd]);
            break;
        case opcode::pop:
            gpr_[inst.rd] = mem_.read64(sp_);
            sp_ += 8;
            break;
        case opcode::jmp:
            pc_ = gpr_[inst.rs1] + inst.imm;
            break;
        case opcode::jeq:
            if (flags_ & 1ULL) {
                pc_ = gpr_[inst.rs1] + inst.imm;
            }
            break;
        case opcode::jne:
            if (!(flags_ & 1ULL)) {
                pc_ = gpr_[inst.rs1] + inst.imm;
            }
            break;
        case opcode::jlt:
            if ((flags_ & 2ULL) != 0) {
                pc_ = gpr_[inst.rs1] + inst.imm;
            }
            break;
        case opcode::jgt:
            if ((flags_ & 2ULL) == 0 && !(flags_ & 1ULL)) {
                pc_ = gpr_[inst.rs1] + inst.imm;
            }
            break;
        case opcode::call:
            lr_ = pc_;
            pc_ = gpr_[inst.rs1] + inst.imm;
            break;
        case opcode::ret:
            pc_ = lr_;
            break;
        case opcode::syscall:
            if (syscall_handler_) {
                syscall_handler_(*this, mem_);
            }
            break;
        case opcode::hlt:
            running_ = false;
            break;
        case opcode::dbg:
            trace_enabled_ = true;
            break;
    }
    return true;
}
}  // namespace nova64
