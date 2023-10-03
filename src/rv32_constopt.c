/* RV32I Base Instruction Set */

/* Internal */
CONSTOPT(nop, {})

/* LUI is used to build 32-bit constants and uses the U-type format. LUI
 * places the U-immediate value in the top 20 bits of the destination
 * register rd, filling in the lowest 12 bits with zeros. The 32-bit
 * result is sign-extended to 64 bits.
 */
CONSTOPT(lui, {
    constopt_info->is_constant[ir->rd] = true;
    constopt_info->const_val[ir->rd] = ir->imm;
})

/* AUIPC is used to build pc-relative addresses and uses the U-type format.
 * AUIPC forms a 32-bit offset from the 20-bit U-immediate, filling in the
 * lowest 12 bits with zeros, adds this offset to the address of the AUIPC
 * instruction, then places the result in register rd.
 */
CONSTOPT(auipc, {
    ir->imm += ir->pc;
    constopt_info->is_constant[ir->rd] = true;
    constopt_info->const_val[ir->rd] = ir->imm;
    ir->opcode = rv_insn_lui;
    ir->impl = dispatch_table[ir->opcode];
})

/* JAL: Jump and Link
 * store successor instruction address into rd.
 * add next J imm (offset) to pc.
 */
CONSTOPT(jal, {
    if (ir->rd) {
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->pc + ir->insn_len;
    }
})

/* The indirect jump instruction JALR uses the I-type encoding. The target
 * address is obtained by adding the sign-extended 12-bit I-immediate to the
 * register rs1, then setting the least-significant bit of the result to zero.
 * The address of the instruction following the jump (pc+4) is written to
 * register rd. Register x0 can be used as the destination if the result is
 * not required.
 */
CONSTOPT(jalr, {
    if (ir->rd) {
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->pc + ir->insn_len;
    }
})

/* clang-format off */
#define OPT_BRANCH_FUNC(type, cond)                               \
    if (constopt_info->is_constant[ir->rs1] &&                   \
        constopt_info->is_constant[ir->rs2]) {                   \
        if ((type) constopt_info->const_val[ir->rs1] cond (type) \
                constopt_info->const_val[ir->rs2])               \
            ir->imm = ir->insn_len;                               \
        ir->opcode = rv_insn_jal;                                 \
        ir->impl = dispatch_table[ir->opcode];                    \
    }
/* clang-format on */

/* BEQ: Branch if Equal */
CONSTOPT(beq, { OPT_BRANCH_FUNC(uint32_t, !=); })

/* BNE: Branch if Not Equal */
CONSTOPT(bne, { OPT_BRANCH_FUNC(uint32_t, ==); })

/* BLT: Branch if Less Than */
CONSTOPT(blt, { OPT_BRANCH_FUNC(int32_t, >=); })

/* BGE: Branch if Greater Than */
CONSTOPT(bge, { OPT_BRANCH_FUNC(int32_t, <); })

/* BLTU: Branch if Less Than Unsigned */
CONSTOPT(bltu, { OPT_BRANCH_FUNC(uint32_t, >=); })

/* BGEU: Branch if Greater Than Unsigned */
CONSTOPT(bgeu, { OPT_BRANCH_FUNC(uint32_t, <); })

/* LB: Load Byte */
CONSTOPT(lb, { constopt_info->is_constant[ir->rd] = false; })

/* LH: Load Halfword */
CONSTOPT(lh, { constopt_info->is_constant[ir->rd] = false; })

/* LW: Load Word */
CONSTOPT(lw, { constopt_info->is_constant[ir->rd] = false; })

/* LBU: Load Byte Unsigned */
CONSTOPT(lbu, { constopt_info->is_constant[ir->rd] = false; })

/* LHU: Load Halfword Unsigned */
CONSTOPT(lhu, { constopt_info->is_constant[ir->rd] = false; })

/* SB: Store Byte */
CONSTOPT(sb, {})

/* SH: Store Halfword */
CONSTOPT(sh, {})

/* SW: Store Word */
CONSTOPT(sw, {})

/* ADDI adds the sign-extended 12-bit immediate to register rs1. Arithmetic
 * overflow is ignored and the result is simply the low XLEN bits of the
 * result. ADDI rd, rs1, 0 is used to implement the MV rd, rs1 assembler
 * pseudo-instruction.
 */
CONSTOPT(addi, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm += constopt_info->const_val[ir->rs1];
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SLTI place the value 1 in register rd if register rs1 is less than the
 * signextended immediate when both are treated as signed numbers, else 0 is
 * written to rd.
 */
CONSTOPT(slti, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] < ir->imm ? 1 : 0;
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SLTIU places the value 1 in register rd if register rs1 is less than the
 * immediate when both are treated as unsigned numbers, else 0 is written to rd.
 */
CONSTOPT(sltiu, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm =
            constopt_info->const_val[ir->rs1] < (uint32_t) ir->imm ? 1 : 0;
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* XORI: Exclusive OR Immediate */
CONSTOPT(xori, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm ^= constopt_info->const_val[ir->rs1];
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* ORI: OR Immediate */
CONSTOPT(ori, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm |= constopt_info->const_val[ir->rs1];
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* ANDI performs bitwise AND on register rs1 and the sign-extended 12-bit
 * immediate and place the result in rd.
 */
CONSTOPT(andi, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm &= constopt_info->const_val[ir->rs1];
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SLLI performs logical left shift on the value in register rs1 by the shift
 * amount held in the lower 5 bits of the immediate.
 */
CONSTOPT(slli, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm = constopt_info->const_val[ir->rs1] << (ir->imm & 0x1f);
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SRLI performs logical right shift on the value in register rs1 by the shift
 * amount held in the lower 5 bits of the immediate.
 */
CONSTOPT(srli, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm = constopt_info->const_val[ir->rs1] >> (ir->imm & 0x1f);
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SRAI performs arithmetic right shift on the value in register rs1 by the
 * shift amount held in the lower 5 bits of the immediate.
 */
CONSTOPT(srai, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm =
            (int32_t) constopt_info->const_val[ir->rs1] >> (ir->imm & 0x1f);
        constopt_info->is_constant[ir->rd] = true;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* ADD */
CONSTOPT(add, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] +
                  (int32_t) constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SUB: Substract */
CONSTOPT(sub, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] -
                  (int32_t) constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SLL: Shift Left Logical */
CONSTOPT(sll, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1]
                  << (constopt_info->const_val[ir->rs2] & 0x1f);
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SLT: Set on Less Than */
CONSTOPT(slt, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] <
                          (int32_t) constopt_info->const_val[ir->rs2]
                      ? 1
                      : 0;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SLTU: Set on Less Than Unsigned */
CONSTOPT(sltu, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] <
                          constopt_info->const_val[ir->rs2]
                      ? 1
                      : 0;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* XOR: Exclusive OR */
CONSTOPT(xor, {
  if (constopt_info->is_constant[ir->rs1] &&
      constopt_info->is_constant[ir->rs2]) {
      constopt_info->is_constant[ir->rd] = true;
      ir->imm = (int32_t) constopt_info->const_val[ir->rs1] ^
                (int32_t) constopt_info->const_val[ir->rs2];
      constopt_info->const_val[ir->rd] = ir->imm;
      ir->opcode = rv_insn_lui;
      ir->impl = dispatch_table[ir->opcode];
  } else
      constopt_info->is_constant[ir->rd] = false;
})

/* SRL: Shift Right Logical */
CONSTOPT(srl, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] >>
                  (constopt_info->const_val[ir->rs2] & 0x1f);
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* SRA: Shift Right Arithmetic */
CONSTOPT(sra, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] >>
                  (constopt_info->const_val[ir->rs2] & 0x1f);
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* OR */
CONSTOPT(or, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] |
                  (int32_t) constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* AND */
CONSTOPT(and, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] &
                  (int32_t) constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* ECALL: Environment Call */
CONSTOPT(ecall, {})

/* EBREAK: Environment Break */
CONSTOPT(ebreak, {})

/* WFI: Wait for Interrupt */
CONSTOPT(wfi, {})

/* URET: return from traps in U-mode */
CONSTOPT(uret, {})

/* SRET: return from traps in S-mode */
CONSTOPT(sret, {})

/* HRET: return from traps in H-mode */
CONSTOPT(hret, {})

/* MRET: return from traps in U-mode */
CONSTOPT(mret, {})

#if RV32_HAS(Zifencei) /* RV32 Zifencei Standard Extension */
CONSTOPT(fencei, {})
#endif

#if RV32_HAS(Zicsr) /* RV32 Zicsr Standard Extension */
/* CSRRW: Atomic Read/Write CSR */
CONSTOPT(csrrw, { constopt_info->is_constant[ir->rd] = false; })

/* CSRRS: Atomic Read and Set Bits in CSR */
CONSTOPT(csrrs, { constopt_info->is_constant[ir->rd] = false; })

/* CSRRC: Atomic Read and Clear Bits in CSR */
CONSTOPT(csrrc, { constopt_info->is_constant[ir->rd] = false; })

/* CSRRWI */
CONSTOPT(csrrwi, { constopt_info->is_constant[ir->rd] = false; })

/* CSRRSI */
CONSTOPT(csrrsi, { constopt_info->is_constant[ir->rd] = false; })

/* CSRRCI */
CONSTOPT(csrrci, { constopt_info->is_constant[ir->rd] = false; })
#endif

/* RV32M Standard Extension */

#if RV32_HAS(EXT_M)
/* MUL: Multiply */
CONSTOPT(mul, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = (int32_t) constopt_info->const_val[ir->rs1] *
                  (int32_t) constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* MULH: Multiply High Signed Signed */
CONSTOPT(mulh, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        const int64_t a = (int32_t) constopt_info->const_val[ir->rs1];
        const int64_t b = (int32_t) constopt_info->const_val[ir->rs2];
        ir->imm = ((uint64_t) (a * b)) >> 32;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* MULHSU: Multiply High Signed Unsigned */
CONSTOPT(mulhsu, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        const int64_t a = (int32_t) constopt_info->const_val[ir->rs1];
        const int64_t b = constopt_info->const_val[ir->rs2];
        ir->imm = ((uint64_t) (a * b)) >> 32;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* MULHU: Multiply High Unsigned Unsigned */
CONSTOPT(mulhu, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = ((int64_t) constopt_info->const_val[ir->rs1] *
                   (int64_t) constopt_info->const_val[ir->rs2]) >>
                  32;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* DIV: Divide Signed */
/* +------------------------+-----------+----------+-----------+
 * |       Condition        |  Dividend |  Divisor |   DIV[W]  |
 * +------------------------+-----------+----------+-----------+
 * | Division by zero       |  x        |  0       |  −1       |
 * | Overflow (signed only) |  −2^{L−1} |  −1      |  −2^{L−1} |
 * +------------------------+-----------+----------+-----------+
 */
CONSTOPT(div, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        const int32_t dividend = (int32_t) constopt_info->const_val[ir->rs1];
        const int32_t divisor = (int32_t) constopt_info->const_val[ir->rs2];
        ir->imm = !divisor ? ~0U
                  : (divisor == -1 &&
                     constopt_info->const_val[ir->rs1] == 0x80000000U)
                      ? constopt_info->const_val[ir->rs1] /* overflow */
                      : (unsigned int) (dividend / divisor);
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* DIVU: Divide Unsigned */
/* +------------------------+-----------+----------+----------+
 * |       Condition        |  Dividend |  Divisor |  DIVU[W] |
 * +------------------------+-----------+----------+----------+
 * | Division by zero       |  x        |  0       |  2^L − 1 |
 * +------------------------+-----------+----------+----------+
 */
CONSTOPT(divu, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        const uint32_t dividend = constopt_info->const_val[ir->rs1];
        const uint32_t divisor = constopt_info->const_val[ir->rs2];
        ir->imm = !divisor ? ~0U : dividend / divisor;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* REM: Remainder Signed */
/* +------------------------+-----------+----------+---------+
 * |       Condition        |  Dividend |  Divisor |  REM[W] |
 * +------------------------+-----------+----------+---------+
 * | Division by zero       |  x        |  0       |  x      |
 * | Overflow (signed only) |  −2^{L−1} |  −1      |  0      |
 * +------------------------+-----------+----------+---------+
 */
CONSTOPT(rem, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        const int32_t dividend = constopt_info->const_val[ir->rs1];
        const int32_t divisor = constopt_info->const_val[ir->rs2];
        ir->imm = !divisor ? dividend
                  : (divisor == -1 &&
                     constopt_info->const_val[ir->rs1] == 0x80000000U)
                      ? 0 /* overflow */
                      : (dividend % divisor);
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* REMU: Remainder Unsigned */
/* +------------------------+-----------+----------+----------+
 * |       Condition        |  Dividend |  Divisor |  REMU[W] |
 * +------------------------+-----------+----------+----------+
 * | Division by zero       |  x        |  0       |  x       |
 * +------------------------+-----------+----------+----------+
 */
CONSTOPT(remu, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        const uint32_t dividend = constopt_info->const_val[ir->rs1];
        const uint32_t divisor = constopt_info->const_val[ir->rs2];
        ir->imm = !divisor ? dividend : dividend % divisor;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_lui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})
#endif

/* RV32A Standard Extension */
/* TODO: support constant optimization for A and F extension */
#if RV32_HAS(EXT_A)

/* LR.W: Load Reserved */
CONSTOPT(lrw, {})

/* SC.W: Store Conditional */
CONSTOPT(scw, {})

/* AMOSWAP.W: Atomic Swap */
CONSTOPT(amoswapw, {})

/* AMOADD.W: Atomic ADD */
CONSTOPT(amoaddw, {})

/* AMOXOR.W: Atomic XOR */
CONSTOPT(amoxorw, {})

/* AMOAND.W: Atomic AND */
CONSTOPT(amoandw, {})

/* AMOOR.W: Atomic OR */
CONSTOPT(amoorw, {})

/* AMOMIN.W: Atomic MIN */
CONSTOPT(amominw, {})

/* AMOMAX.W: Atomic MAX */
CONSTOPT(amomaxw, {})

/* AMOMINU.W */
CONSTOPT(amominuw, {})

/* AMOMAXU.W */
CONSTOPT(amomaxuw, {})
#endif /* RV32_HAS(EXT_A) */

/* RV32F Standard Extension */

#if RV32_HAS(EXT_F)
/* FLW */
CONSTOPT(flw, {})

/* FSW */
CONSTOPT(fsw, {})

/* FMADD.S */
CONSTOPT(fmadds, {})

/* FMSUB.S */
CONSTOPT(fmsubs, {})

/* FNMSUB.S */
CONSTOPT(fnmsubs, {})

/* FNMADD.S */
CONSTOPT(fnmadds, {})

/* FADD.S */
CONSTOPT(fadds, {})

/* FSUB.S */
CONSTOPT(fsubs, {})

/* FMUL.S */
CONSTOPT(fmuls, {})

/* FDIV.S */
CONSTOPT(fdivs, {})

/* FSQRT.S */
CONSTOPT(fsqrts, {})

/* FSGNJ.S */
CONSTOPT(fsgnjs, {})

/* FSGNJN.S */
CONSTOPT(fsgnjns, {})

/* FSGNJX.S */
CONSTOPT(fsgnjxs, {})

/* FMIN.S
 * In IEEE754-201x, fmin(x, y) return
 * - min(x,y) if both numbers are not NaN
 * - if one is NaN and another is a number, return the number
 * - if both are NaN, return NaN
 * When input is signaling NaN, raise invalid operation
 */
CONSTOPT(fmins, {})

/* FMAX.S */
CONSTOPT(fmaxs, {})

/* FCVT.W.S and FCVT.WU.S convert a floating point number to an integer,
 * the rounding mode is specified in rm field.
 */

/* FCVT.W.S */
CONSTOPT(fcvtws, {})

/* FCVT.WU.S */
CONSTOPT(fcvtwus, {})

/* FMV.X.W */
CONSTOPT(fmvxw, {})

/* FEQ.S performs a quiet comparison: it only sets the invalid operation
 * exception flag if either input is a signaling NaN.
 */
CONSTOPT(feqs, {})

/* FLT.S and FLE.S perform what the IEEE 754-2008 standard refers to as
 * signaling comparisons: that is, they set the invalid operation exception
 * flag if either input is NaN.
 */
CONSTOPT(flts, {})

CONSTOPT(fles, {})

/* FCLASS.S */
CONSTOPT(fclasss, {})

/* FCVT.S.W */
CONSTOPT(fcvtsw, {})

/* FCVT.S.WU */
CONSTOPT(fcvtswu, {})

/* FMV.W.X */
CONSTOPT(fmvwx, {})
#endif

/* RV32C Standard Extension */

#if RV32_HAS(EXT_C)
/* C.ADDI4SPN is a CIW-format instruction that adds a zero-extended non-zero
 * immediate, scaledby 4, to the stack pointer, x2, and writes the result to
 * rd'.
 * This instruction is used to generate pointers to stack-allocated variables,
 * and expands to addi rd', x2, nzuimm[9:2].
 */
CONSTOPT(caddi4spn, {
    if (constopt_info->is_constant[rv_reg_sp]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[rv_reg_sp] + (uint16_t) ir->imm;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* C.LW loads a 32-bit value from memory into register rd'. It computes an
 * effective address by adding the zero-extended offset, scaled by 4, to the
 * base address in register rs1'. It expands to lw rd', offset[6:2](rs1').
 */
CONSTOPT(clw, { constopt_info->is_constant[ir->rd] = false; })

/* C.SW stores a 32-bit value in register rs2' to memory. It computes an
 * effective address by adding the zero-extended offset, scaled by 4, to the
 * base address in register rs1'.
 * It expands to sw rs2', offset[6:2](rs1').
 */
CONSTOPT(csw, {})

/* C.NOP is mapped to NOP */

/* C.ADDI adds the non-zero sign-extended 6-bit immediate to the value in
 * register rd then writes the result to rd. C.ADDI expands into
 * addi rd, rd, nzimm[5:0]. C.ADDI is only valid when rd'=x0. The code point
 * with both rd=x0 and nzimm=0 encodes the C.NOP instruction; the remaining
 * code points with either rd=x0 or nzimm=0 encode HINTs.
 */
CONSTOPT(caddi, {
    if (constopt_info->is_constant[ir->rd]) {
        ir->imm = constopt_info->const_val[ir->rd] + (int16_t) ir->imm;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.JAL */
CONSTOPT(cjal, {
    constopt_info->is_constant[rv_reg_ra] = true;
    constopt_info->const_val[rv_reg_ra] = ir->pc + ir->insn_len;
})

/* C.LI loads the sign-extended 6-bit immediate, imm, into register rd.
 * C.LI expands into addi rd, x0, imm[5:0].
 * C.LI is only valid when rd=x0; the code points with rd=x0 encode HINTs.
 */
CONSTOPT(cli, {
    constopt_info->is_constant[ir->rd] = true;
    constopt_info->const_val[ir->rd] = ir->imm;
})

/* C.ADDI16SP is used to adjust the stack pointer in procedure prologues
 * and epilogues. It expands into addi x2, x2, nzimm[9:4].
 * C.ADDI16SP is only valid when nzimm'=0; the code point with nzimm=0 is
 * reserved.
 */
CONSTOPT(caddi16sp, {
    if (constopt_info->is_constant[ir->rd]) {
        ir->imm = constopt_info->const_val[ir->rd] + ir->imm;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.LUI loads the non-zero 6-bit immediate field into bits 17–12 of the
 * destination register, clears the bottom 12 bits, and sign-extends bit
 * 17 into all higher bits of the destination.
 * C.LUI expands into lui rd, nzimm[17:12].
 * C.LUI is only valid when rd'={x0, x2}, and when the immediate is not equal
 * to zero.
 */
CONSTOPT(clui, {
    constopt_info->is_constant[ir->rd] = true;
    constopt_info->const_val[ir->rd] = ir->imm;
})

/* C.SRLI is a CB-format instruction that performs a logical right shift
 * of the value in register rd' then writes the result to rd'. The shift
 * amount is encoded in the shamt field. C.SRLI expands into srli rd',
 * rd', shamt[5:0].
 */
CONSTOPT(csrli, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm = constopt_info->const_val[ir->rs1] >> ir->shamt;
        constopt_info->const_val[ir->rs1] = ir->imm;
        ir->rd = ir->rs1;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.SRAI is defined analogously to C.SRLI, but instead performs an
 * arithmetic right shift. C.SRAI expands to srai rd', rd', shamt[5:0].
 */
CONSTOPT(csrai, {
    if (constopt_info->is_constant[ir->rs1]) {
        const uint32_t mask = 0x80000000 & constopt_info->const_val[ir->rs1];
        ir->imm = constopt_info->const_val[ir->rs1] >> ir->shamt;
        for (unsigned int i = 0; i < ir->shamt; ++i)
            ir->imm |= mask >> i;
        constopt_info->const_val[ir->rs1] = ir->imm;
        ir->rd = ir->rs1;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.ANDI is a CB-format instruction that computes the bitwise AND of the
 * value in register rd' and the sign-extended 6-bit immediate, then writes
 * the result to rd'. C.ANDI expands to andi rd', rd', imm[5:0].
 */
CONSTOPT(candi, {
    if (constopt_info->is_constant[ir->rs1]) {
        ir->imm = constopt_info->const_val[ir->rs1] & ir->imm;
        constopt_info->const_val[ir->rs1] = ir->imm;
        ir->rd = ir->rs1;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.SUB */
CONSTOPT(csub, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] -
                  constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* C.XOR */
CONSTOPT(cxor, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] ^
                  constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

CONSTOPT(cor, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] |
                  constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

CONSTOPT(cand, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] &
                  constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* C.J performs an unconditional control transfer. The offset is sign-extended
 * and added to the pc to form the jump target address.
 * C.J can therefore target a ±2 KiB range.
 * C.J expands to jal x0, offset[11:1].
 */
CONSTOPT(cj, {})

/* C.BEQZ performs conditional control transfers. The offset is sign-extended
 * and added to the pc to form the branch target address.
 * It can therefore target a ±256 B range. C.BEQZ takes the branch if the
 * value in register rs1' is zero. It expands to beq rs1', x0, offset[8:1].
 */
CONSTOPT(cbeqz, {
    if (constopt_info->is_constant[ir->rs1]) {
        if (constopt_info->const_val[ir->rs1])
            ir->imm = ir->insn_len;
        ir->opcode = rv_insn_cj;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.BEQZ */
CONSTOPT(cbnez, {
    if (constopt_info->is_constant[ir->rs1]) {
        if (!constopt_info->const_val[ir->rs1])
            ir->imm = ir->insn_len;
        ir->opcode = rv_insn_cj;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.SLLI is a CI-format instruction that performs a logical left shift of
 * the value in register rd then writes the result to rd. The shift amount
 * is encoded in the shamt field. C.SLLI expands into slli rd, rd, shamt[5:0].
 */
CONSTOPT(cslli, {
    if (constopt_info->is_constant[ir->rd]) {
        ir->imm = constopt_info->const_val[ir->rd] << (uint8_t) ir->imm;
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.LWSP */
CONSTOPT(clwsp, { constopt_info->is_constant[ir->rd] = false; })

/* C.JR */
CONSTOPT(cjr, {})

/* C.MV */
CONSTOPT(cmv, {
    if (constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    }
})

/* C.EBREAK */
CONSTOPT(cebreak, {})

/* C.JALR */
CONSTOPT(cjalr, {
    constopt_info->is_constant[rv_reg_ra] = true;
    constopt_info->const_val[ir->rd] = ir->pc + ir->insn_len;
})

/* C.ADD adds the values in registers rd and rs2 and writes the result to
 * register rd.
 * C.ADD expands into add rd, rd, rs2.
 * C.ADD is only valid when rs2=x0; the code points with rs2=x0 correspond to
 * the C.JALR and C.EBREAK instructions. The code points with rs2=x0 and rd=x0
 * are HINTs.
 */
CONSTOPT(cadd, {
    if (constopt_info->is_constant[ir->rs1] &&
        constopt_info->is_constant[ir->rs2]) {
        constopt_info->is_constant[ir->rd] = true;
        ir->imm = constopt_info->const_val[ir->rs1] +
                  constopt_info->const_val[ir->rs2];
        constopt_info->const_val[ir->rd] = ir->imm;
        ir->opcode = rv_insn_clui;
        ir->impl = dispatch_table[ir->opcode];
    } else
        constopt_info->is_constant[ir->rd] = false;
})

/* C.SWSP */
CONSTOPT(cswsp, {})
#endif