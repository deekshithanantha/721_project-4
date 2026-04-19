// See LICENSE for license details.

#include "alu_op_template.h"


reg_t alu_rv64_fmv_x_s(payload_t &pay_buf, const state_t &state) {
   int xlen = 64;
   reg_t pc = pay_buf.pc;
   reg_t npc = sext_xlen(pc + insn_length( MATCH_FMV_X_S));
   insn_t insn = pay_buf.inst;
   pay_buf.fflags = 0;

#include "insns/fmv_x_s.h"

   pay_buf.c_next_pc = npc;

   return npc;
}
