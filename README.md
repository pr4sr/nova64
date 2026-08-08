# nova64

> a custom 64-bit virtual computer architecture. designed to explore computer architecture, instruction set design, and low-level systems programming.

![image](https://i.imgur.com/M0zplVQ.png)

## overview

nova64 is a risc-inspired 64-bit virtual machine ecosystem. it features a custom isa, cpu emulator, assembler toolchain, and an experimental operating environment.

made to understand computers from the hardware abstraction level upward. from instruction encoding and memory models to boot processes and system software.

## features

* **isa:** custom 64-bit risc-inspired instruction set.
* **cpu:** modular cpu and virtual machine core.
* **encoding:** custom instruction encoding and decoding system.
* **toolchain:** assembler and disassembler tools.
* **executable:** custom executable format.
* **memory:** mapped subsystem with mmio support.
* **boot:** boot rom initialization flow.
* **os:** experimental cli operating system environment.
* **docs:** example assembly programs and technical notes.

## architecture flow

```text
assembly source
       ↓
   assembler
       ↓
executable format
       ↓
 virtual machine
       ↓
cpu core & memory
       ↓
novaos environment
```

## goals

future development goals include expanding the operating environment, improving hardware abstraction, adding more advanced system features, and exploring compiler support.

check the `docs` directory for more info and technical design notes.
