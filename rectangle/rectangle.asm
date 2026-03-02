# Draw filled rectangle on screen
# Rectangle corners (YYXX format)
.word 0x100 0x1440    # Top-Left corner
.word 0x101 0x1C40    # Bottom-Left corner
.word 0x102 0x1460    # Top-Right corner
.word 0x103 0x1C60    # Bottom-Right corner

    lw   $a0, $zero, $imm, 256          # Load Top-Left
    lw   $a1, $zero, $imm, 259          # Load Bottom-Right

    sra  $t0, $a0, $imm, 8              # start Y
    and  $t1, $a0, $imm, 0xFF           # start X
    sra  $t2, $a1, $imm, 8              # end Y
    and  $s0, $a1, $imm, 0xFF           # end X

    add  $s1, $zero, $t0, 0             # current Y = start Y

Outer_Y_Loop:
    bgt  $imm, $s1, $t2, End            # if current Y > end Y, end program

Inner_X_Init:
    add  $s2, $zero, $t1, 0             # current X = start X

Inner_X_Loop:
    bgt  $imm, $s2, $s0, Next_Row       # if current X > end X, go to next row

    add  $v0, $imm, $zero, 255          # load color 255
    out  $v0, $zero, $imm, 21           # set color port

    mul  $v0, $s1, $imm, 256            # calc Y * 256
    add  $v0, $v0, $s2, 0               # add X
    out  $v0, $zero, $imm, 20           # set address port

    add  $v0, $imm, $zero, 1            # draw pixel command
    out  $v0, $zero, $imm, 22           # send draw command

    add  $s2, $s2, $imm, 1              # current X++
    beq  $imm, $zero, $zero, Inner_X_Loop  # loop inner X again

Next_Row:
    add  $s1, $s1, $imm, 1              # current Y++
    beq  $imm, $zero, $zero, Outer_Y_Loop  # loop outer Y again

End:
    halt $zero, $zero, $zero, 0         # halt program