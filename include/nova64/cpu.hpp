#pragma once

#include "nova64/common.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace nova64 {
class memory {
public:
    explicit memory(std::size_t size = kMemorySize);
    byte_t read8(word_t address) const;
    std::uint16_t read16(word_t address) const;
    std::uint32_t read32(word_t address) const;
    word_t read64(word_t address) const;
    void write8(word_t address, byte_t value);
    void write16(word_t address, std::uint16_t value);
    void write32(word_t address, std::uint32_t value);
    void write64(word_t address, word_t value);
    void load_bytes(word_t base, const std::vector<byte_t>& data);
    void clear();
    std::size_t size() const;
    std::vector<byte_t> dump(word_t start, std::size_t length) const;

    using mmio_handler = std::function<void(word_t, word_t, bool)>;
    void set_mmio_handler(mmio_handler handler);

private:
    std::vector<byte_t> data_;
    mmio_handler mmio_handler_;
};

class cpu {
public:
    cpu(memory& mem);
    void reset();
    bool step();
    void set_syscall_handler(std::function<void(cpu&, memory&)> handler);
    void set_breakpoint(word_t address, bool enabled);
    bool has_breakpoint(word_t address) const;
    void set_trace(bool enabled);
    bool trace() const;

    word_t pc() const;
    word_t sp() const;
    word_t lr() const;
    word_t flags() const;
    std::array<word_t, 32> registers() const;
    word_t read_register(std::uint8_t index) const;
    void write_register(std::uint8_t index, word_t value);
    word_t cycle_count() const;
    bool running() const;
    std::vector<std::string> trace_log() const;
    void set_max_steps(std::uint64_t max_steps);
    std::uint64_t max_steps() const;

private:
    void update_flags(word_t result, word_t lhs, word_t rhs, bool subtract);
    bool execute_instruction(const instruction_t& inst);

    memory& mem_;
    std::array<word_t, 32> gpr_{};
    word_t pc_{};
    word_t sp_{};
    word_t lr_{};
    word_t flags_{};
    std::uint8_t privilege_{};
    std::uint64_t cycles_{};
    bool running_{};
    bool trace_enabled_{};
    std::uint64_t max_steps_{};
    std::uint64_t executed_steps_{};
    std::unordered_map<word_t, bool> breakpoints_{};
    std::vector<std::string> trace_log_{};
    std::function<void(cpu&, memory&)> syscall_handler_;
};

class virtual_machine {
public:
    explicit virtual_machine(std::size_t memory_size = kMemorySize);
    void load_program(const std::vector<byte_t>& bytes, word_t entry = 0x1000);
    void run(std::size_t max_steps = 1000);
    memory& mem();
    cpu& core();

private:
    memory mem_;
    cpu core_;
};
}  // namespace nova64
