# Data: array to sort 
.word 0x100 0x00000008   # 8
.word 0x101 0xFFFFFFFF   # -1
.word 0x102 0x00000003   # 3
.word 0x103 0xFFFFFFE7   # -25
.word 0x104 0x00000005   # 5
.word 0x105 0x00000019   # 25
.word 0x106 0x00000058   # 88
.word 0x107 0x00000000   # 0
.word 0x108 0x00000000   # 0
.word 0x109 0xFFFFFFFD   # -3
.word 0x10A 0x00000011   # 17
.word 0x10B 0x00000001   # 1
.word 0x10C 0xFFFFFFFE   # -2
.word 0x10D 0xFFFFFFD6   # -42
.word 0x10E 0x00000004   # 4
.word 0x10F 0x00000007   # 7


# Set i = 0
add $s0, $imm, $zero, 0

# Load base address of array into s2
add $s2, $imm, $zero, 0x100

loop_i:
    # t0 ← 15 (last index)
    add $t0, $imm, $zero, 15

    # If i > 15, exit loop
    bgt $imm, $s0, $t0, sort_done

    # Reset j = 0 for inner loop
    add $s1, $imm, $zero, 0

loop_j:
    # s3 ← (15 - i)
    add $t0, $imm, $zero, 16
    sub $t0, $t0, $s0, 0
    sub $s3, $t0, $imm, 1

    # If j ≥ s3, move to next i
    bge $imm, $s1, $s3, inc_i

    # t2 ← &A[j]
    add $t2, $s2, $s1, 0
    lw  $t0, $t2, $zero, 0     # t0 = A[j]

    # t2 ← &A[j+1]
    add $t2, $s1, $imm, 1
    add $t2, $s2, $t2, 0
    lw  $t1, $t2, $zero, 0     # t1 = A[j+1]

    # If A[j] ≤ A[j+1], no swap needed
    ble $imm, $t0, $t1, skip_swap

    # Swap A[j] and A[j+1]
    sw  $t0, $t2, $zero, 0     # A[j+1] = A[j]
    add $t2, $s2, $s1, 0
    sw  $t1, $t2, $zero, 0     # A[j] = A[j+1]

skip_swap:
    # j++
    add $s1, $s1, $imm, 1

    # Jump back to inner loop
    beq $imm, $zero, $zero, loop_j

inc_i:
    # i++
    add $s0, $s0, $imm, 1

    # Jump back to outer loop
    beq $imm, $zero, $zero, loop_i

sort_done:
    # Possibly for marking sorted output
    add $a0, $imm, $zero, 0x10a
    add $t3, $t0, $imm, 0x10a
    sw  $a0, $t3, $imm, 0

    # Terminate program
    halt $zero, $zero, $zero, 0

