.org 0x1000
start:
    loadi r1 0x1100
    csrw eha r1

    loadi r2 42
    loadi r3 0
    div r4 r2 r3

    loadi r5 0x7FFFFFFFFFFFFFFF
    loadi r6 1
    add r7 r5 r6

    hlt

.org 0x1100
exception_handler:
    csrr r10 ecause
    csrr r11 epc
    loadi r12 4
    add r11 r11 r12
    csrw epc r11
    eret
