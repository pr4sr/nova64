# nova64

a custom 64-bit virtual computer architecture designed from the ground up to explore computer architecture, instruction set design, and low-level systems programming.

## overview

nova64 is a risc-inspired 64-bit virtual machine ecosystem featuring a custom instruction set architecture, cpu emulator, assembler toolchain, and experimental operating environment.

the project was created to understand computers from the hardware abstraction level upward — from instruction encoding and memory models to boot processes and system software.

## features

* custom 64-bit risc-inspired isa
* modular cpu and virtual machine core
* custom instruction encoding and decoding system
* assembler and disassembler toolchain
* custom executable format
* memory subsystem with mmio support
* boot rom initialization flow
* experimental cli operating system environment
* example assembly programs and documentation

## architecture

nova64 currently includes:

assembly source
↓
assembler
↓
executable format
↓
virtual machine
↓
cpu core + memory system
↓
novaos environment

## goals

future development goals include expanding the operating environment, improving hardware abstraction, adding more advanced system features, and exploring compiler support.

documentation and technical design notes are available in the `docs` directory.
