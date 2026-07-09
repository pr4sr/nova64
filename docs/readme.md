## core features

- 64-bit architecture
- 32 general-purpose registers
- little-endian memory
- 64-bit program counter and stack pointer
- privilege levels
- memory-mapped i/o
- simple boot and exception system

## registers

- r0-r31: general-purpose registers
- pc: program counter
- sp: stack pointer
- lr: return address
- flags: status

## calling convention

- r0: return value
- r1-r6: arguments
- r7-r15: temporary values
- sp: stack pointer
- lr: return address

## memory map

- 0x0000000000000000 - 0x00000000000fffff: low memory
- 0x0000000000100000: kernel start
- 0x00000000f0000000: device i/o

## exceptions

- invalid instruction
- memory fault
- breakpoint
- system call

## instruction Format

Each instruction is 32 bits wide.

- opcode: bits 31-24
- rd: bits 23-19
- rs1: bits 18-14
- rs2: bits 13-9
- imm: bits 8-0

## Core Instructions

- nop
- mov rd, rs
- loadi rd, imm
- add rd, rs1, rs2
- sub rd, rs1, rs2
- mul rd, rs1, rs2
- div rd, rs1, rs2
- and rd, rs1, rs2
- or rd, rs1, rs2
- xor rd, rs1, rs2
- shl rd, rs1, rs2
- shr rd, rs1, rs2
- rol rd, rs1, rs2
- ror rd, rs1, rs2
- load rd, [rs + imm]
- store rs, [rd + imm]
- push rs
- pop rd
- jmp target
- jeq target
- jne target
- jlt target
- jgt target
- call target
- ret
- syscall
- hlt
- dbg
