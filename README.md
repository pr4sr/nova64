# nova64

a custom 64-bit virtual machine with its own instruction set, assembler, executable format, and experimental os.

![nova64](https://i.imgur.com/M0zplVQ.png)

## about

nova64 is a risc-inspired architecture project built from the ground up. it includes a cpu emulator, a custom isa, an assembler and disassembler, memory-mapped i/o, and a small operating environment called novaos.

## what's inside

- **custom isa** — a 64-bit, risc-inspired instruction set.
- **cpu & vm** — instruction execution and virtual machine core.
- **assembler / disassembler** — tools for working with nova64 assembly.
- **executable format** — a custom format for assembled programs.
- **memory subsystem** — memory mapping and mmio support.
- **boot rom** — initialization and boot flow.
- **novaos** — an experimental cli environment running on the vm.
- **documentation** — assembly examples, design notes, and technical details.

for implementation details and examples, see [`docs/`](./docs/).
