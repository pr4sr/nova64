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

    const auto word = encode_instruction(add_inst);
    mem.write32(0x1000, word);
    core.set_trace(true);

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
    core.write_register(0, 0x0000000000000010ULL);
    core.write_register(1, 2);
    instruction_t ror_inst{};
    ror_inst.op = opcode::ror;
    ror_inst.rd = 2;
    ror_inst.rs1 = 0;
    ror_inst.rs2 = 1;
    mem.write32(0x1000, encode_instruction(ror_inst));
    core.step();
    assert(core.read_register(2) == 0x0000000000000004ULL);

    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path();
    assembler as;
    const auto bootloader = as.assemble((root / "examples" / "bootloader.asm").string());
    const auto kernel = as.assemble((root / "examples" / "kernel.asm").string());
    assert(!bootloader.sections.empty());
    assert(!kernel.sections.empty());

    std::cout << "nova64 tests passed\n";
    return 0;
}
