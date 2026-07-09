#include "nova64/cpu.hpp"
#include "nova64/kernel.hpp"

#include <iostream>

namespace nova64 {
void boot_rom(memory& mem, cpu& cpu, kernel& kernel) {
    mem.write64(kStackBase, 0);
    cpu.write_register(1, kKernelBase);
    cpu.write_register(2, kStackBase);
    kernel.log("boot rom complete");
}
}  // namespace nova64
