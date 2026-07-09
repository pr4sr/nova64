start:
    loadi r0 42
    loadi r1 0x1000
    store r0 [r1]
    load r2 [r1]
    hlt
