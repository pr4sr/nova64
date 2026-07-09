#include "nova64/assembler.hpp"
#include "nova64/cpu.hpp"
#include "nova64/kernel.hpp"

#include <fstream>
#include <iostream>
#include <vector>

namespace nova64 {
void boot_rom(memory& mem, cpu& cpu, kernel& kernel);
}

int main(int argc, char** argv) {
    using namespace nova64;

    std::cout << "[nova] initialising memory subsystem\n";
    memory mem;
    std::cout << "[nova] creating cpu core\n";
    cpu core(mem);
    std::cout << "[nova] booting kernel shell\n";
    kernel os(mem, core);
    os.boot();
    std::cout << "[nova] executing boot rom\n";
    try {
        boot_rom(mem, core, os);
    } catch (const std::exception& ex) {
        std::cerr << "[nova] boot failed: " << ex.what() << "\n";
        return 2;
    }

    const auto run_program = [&](const std::string& path) -> int {
        assembler as;
        try {
            const auto exe = as.assemble(path);
            std::cout << "[nova] assembling input file: " << path << "\n";
            std::cout << "[nova] assembled " << exe.sections.size() << " section(s)\n";
            std::vector<byte_t> code = exe.sections.front().bytes;
            std::cout << "[nova] loading " << code.size() << " bytes into memory at 0x1000\n";
            mem.load_bytes(0x1000, code);
            core.reset();
            core.set_max_steps(10000);
            core.write_register(0, 0x1000);
            core.write_register(1, 0x2000);

            std::cout << "[nova] starting execution loop\n";
            while (core.running()) {
                if (core.cycle_count() % 1000 == 0) {
                    std::cout << "[nova] step " << core.cycle_count() << " -> pc=0x" << std::hex << core.pc() << std::dec << "\n";
                }
                if (!core.step()) {
                    break;
                }
            }
            std::cout << "[nova] cpu halted after " << core.cycle_count() << " cycles\n";
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "error: " << ex.what() << "\n";
            return 2;
        }
    };

    if (argc < 2) {
        std::cout << "[nova] entering integrated kernel shell\n";
        os.shell();
        return 0;
    }

    return run_program(argv[1]);
}
