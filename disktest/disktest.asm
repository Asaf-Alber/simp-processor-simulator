# disktest.asm
# This program loads four disk sectors into memory, sums the first word of each,
# and writes the result to a fifth sector. It repeats this for 128 sets.
# Disk controller I/O ports:
#   14: Command (0=no op, 1=read, 2=write)
#   15: Sector number
#   16: Memory address
#   17: Status (0=ready, 1=busy)

# Initialize registers
add $s2, $zero, $imm, 2000    # $s2 = base memory address for sector data
add $s1, $zero, $imm, 0       # $s1 = sector index (0..3)
add $t1, $zero, $imm, 4       # $t1 = number of sectors to load per set
add $t2, $zero, $imm, 128     # $t2 = number of sets to process

# Load four sectors into memory, one at a time
LOAD_SECTORS:
    out $s1, $zero, $imm, 15      # Output sector number to port 15
    out $s2, $zero, $imm, 16      # Output memory address to port 16
    add $a0, $zero, $imm, 1       # $a0 = 1 (read command)
    out $a0, $zero, $imm, 14      # Output command to port 14 (start read)
CHECK_BUSY:
    in $a1, $zero, $imm, 17       # Read status from port 17
    bne $imm, $zero, $a1, CHECK_BUSY # Wait if busy

    add $s1, $s1, $imm, 1         # Increment sector number
    add $s2, $s2, $t2, 0          # Move memory address to next sector
    blt $imm, $s1, $t1, LOAD_SECTORS # Repeat for all 4 sectors

# Process: sum the first word of each of the four loaded sectors
add $s0, $zero, $imm, 0           # $s0 = set index (0..127)
PROCESS_SUM:
    add $t0, $s0, $imm, 2000      # $t0 = address of first sector's word
    lw $a0, $zero, $t0, 0         # $a0 = first word
    add $t0, $s0, $imm, 2128      # $t0 = address of second sector's word
    lw $a1, $zero, $t0, 0         # $a1 = second word
    add $v0, $a0, $a1, 0          # $v0 = sum of first two words

    add $t0, $s0, $imm, 2256      # $t0 = address of third sector's word
    lw $a0, $zero, $t0, 0         # $a0 = third word
    add $v0, $v0, $a0, 0          # $v0 += third word

    add $t0, $s0, $imm, 2384      # $t0 = address of fourth sector's word
    lw $a0, $zero, $t0, 0         # $a0 = fourth word
    add $v0, $v0, $a0, 0          # $v0 += fourth word

    add $t0, $s0, $imm, 4000      # $t0 = address to store result (fifth sector)
    sw $v0, $zero, $t0, 0         # Store sum in fifth sector

    add $s0, $s0, $imm, 1         # Next set
    bne $imm, $s0, $t2, PROCESS_SUM # Repeat for all sets

# Write the results (fifth sector) back to disk
add $s1, $zero, $imm, 4           # $s1 = sector number to write (sector 4)
out $s1, $zero, $imm, 15          # Output sector number to port 15
add $s2, $zero, $imm, 4000        # $s2 = memory address of results
out $s2, $zero, $imm, 16          # Output memory address to port 16
add $a0, $zero, $imm, 2           # $a0 = 2 (write command)
out $a0, $zero, $imm, 14          # Output command to port 14 (start write)
WAIT_WRITE_DONE:
    in $a1, $zero, $imm, 17       # Read status from port 17
    bne $imm, $zero, $a1, WAIT_WRITE_DONE # Wait if busy

halt $zero, $zero, $zero, 0       # Stop execution
