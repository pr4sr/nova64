#include "nova64/cpu.hpp"

namespace nova64 {
virtual_machine::virtual_machine(std::size_t memory_size) : mem_(memory_size), core_(mem_) {}

void virtual_machine::load_program(const std::vector<byte_t>& bytes, word_t entry) {
    mem_.load_bytes(entry, bytes);
    core_.write_register(0, entry);
    core_.write_register(1, kStackBase);
}

void virtual_machine::run(std::size_t max_steps) {
    std::size_t steps = 0;
    while (core_.running() && steps < max_steps) {
        core_.step();
        ++steps;
    }
}

memory& virtual_machine::mem() { return mem_; }
cpu& virtual_machine::core() { return core_; }
}  // namespace nova64
