#include "nova64/cpu.hpp"
#include "nova64/assembler.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    using namespace nova64;

    memory mem;
    cpu core(mem);
    core.write_register(0, 10);
    core.write_register(1, 20);

    instruction_t add_inst{};
    add_inst.op = opcode::add;
    add_inst.rd = 2;
    add_inst.rs1 = 0;
    add_inst.rs2 = 1;

    mem.write32(0x1000, encode_instruction(add_inst));
    core.step();
    assert(core.read_register(2) == 30);

    core.reset();
    core.write_register(0, 0x0000000000000001ULL);
    core.write_register(1, 4);
    instruction_t rol_inst{};
    rol_inst.op = opcode::rol;
    rol_inst.rd = 2;
    rol_inst.rs1 = 0;
    rol_inst.rs2 = 1;
    mem.write32(0x1000, encode_instruction(rol_inst));
    core.step();
    assert(core.read_register(2) == 0x0000000000000010ULL);

    core.reset();
    instruction_t loadi_inst{};
    loadi_inst.op = opcode::loadi;
    loadi_inst.rd = 0;
    loadi_inst.imm = -1;
    const auto decoded = decode_instruction(encode_instruction(loadi_inst));
    assert(decoded.imm == -1);

    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path();
    assembler as;
    const auto hello = as.assemble((root / "examples" / "hello.asm").string());
    assert(!hello.sections.empty());
    mem.load_bytes(0x1000, hello.sections.front().bytes);
    core.reset();
    core.set_max_steps(1000);
    while (core.running() && core.cycle_count() < core.max_steps()) {
        core.step();
    }
    assert(core.read_register(2) == 3);

    const auto memory_test = as.assemble((root / "examples" / "memory_test.asm").string());
    mem.load_bytes(0x1000, memory_test.sections.front().bytes);
    core.reset();
    core.set_max_steps(1000);
    while (core.running() && core.cycle_count() < core.max_steps()) {
        core.step();
    }
    assert(core.read_register(2) == 42);

    const auto shell_utils = as.assemble((root / "examples" / "shell_utils.asm").string());
    mem.load_bytes(0x1000, shell_utils.sections.front().bytes);
    core.reset();
    core.set_max_steps(1000);
    while (core.running() && core.cycle_count() < core.max_steps()) {
        core.step();
    }
    assert(core.read_register(2) == 5);

    const auto bootloader = as.assemble((root / "examples" / "bootloader.asm").string());
    const auto kernel = as.assemble((root / "examples" / "kernel.asm").string());
    assert(!bootloader.sections.empty());
    assert(!kernel.sections.empty());

    const auto diag = as.assemble((root / "examples" / "cpu_diag.asm").string());
    assert(!diag.sections.empty());
    mem.load_bytes(0x1000, diag.sections.front().bytes);
    core.reset();
    core.set_max_steps(1000000);
    while (core.running() && core.cycle_count() < core.max_steps()) {
        core.step();
    }
    assert(core.read_register(1) == 0);

    std::cout << "nova64 tests passed\n";
    return 0;
}
