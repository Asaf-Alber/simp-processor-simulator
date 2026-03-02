
.word 0x100 0x6                 # Input: n = 6 (hexadecimal)

lw  $s0, $imm, $zero, 0x100        # Load n from memory address 0x100 into $s0
add  $s1, $imm, $zero, 1            # Initialize result = 1

check_n_zero:
    bne  $imm, $s0, $zero, fact_loop    # If n != 0, jump to factorial loop

fact_loop:
    mul  $s1, $s1, $s0, 0               # result = result * n
    add  $s0, $s0, $imm, -1             # n = n - 1
    bne  $imm, $s0, $zero, fact_loop    # If n != 0, continue loop

done:
    sw   $s1, $imm, $zero, 0x101        # Store result to memory address 0x101
    halt $zero, $zero, $zero, 0         # Halt execution


