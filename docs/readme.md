# nova64 core documentation

## core features

- 64-bit architecture
- 32 general-purpose registers
- little-endian memory
- 64-bit program counter and stack pointer
- privilege levels
- memory-mapped i/o
- simple boot and exception system
- control and status registers (csrs)
- hardware breakpoints and step execution limits
- instruction tracing and execution logging
- custom system call handlers

## registers

- `r0-r31`: general-purpose registers
- `pc`: program counter
- `sp`: stack pointer (`r31`)
- `lr`: return address
- `flags`: status register
- `epc`: exception program counter (csr)
- `eha`: exception handler address (csr)
- `ecause`: exception cause (csr)

## calling convention

- `r0`: return value
- `r1-r6`: arguments
- `r7-r15`: temporary values
- `sp`: stack pointer (`r31`)
- `lr`: return address

## memory map

- `0x0000000000000000 - 0x00000000000fffff`: low memory
- `0x0000000000100000`: kernel start
- `0x00000000f0000000`: device i/o

## exceptions

- invalid instruction
- memory fault
- divide by zero
- breakpoint
- system call

## instruction format

each instruction is 32 bits wide.

| field  | bits    |
|--------|---------|
| opcode | 31-24   |
| rd     | 23-19   |
| rs1    | 18-14   |
| rs2    | 13-9    |
| imm    | 8-0     |

## core instructions

- `nop`
- `mov rd, rs`
- `loadi rd, imm`
- `add rd, rs1, rs2`
- `sub rd, rs1, rs2`
- `mul rd, rs1, rs2`
- `div rd, rs1, rs2`
- `and rd, rs1, rs2`
- `or rd, rs1, rs2`
- `xor rd, rs1, rs2`
- `shl rd, rs1, rs2`
- `shr rd, rs1, rs2`
- `rol rd, rs1, rs2`
- `ror rd, rs1, rs2`
- `load rd, [rs + imm]`
- `store rs, [rd + imm]`
- `push rs`
- `pop rd`
- `jmp target`
- `jeq target`
- `jne target`
- `jlt target`
- `jgt target`
- `call target`
- `ret`
- `csrr rd, csr`
- `csrw csr, rs`
- `eret`
- `syscall`
- `hlt`
- `dbg`
