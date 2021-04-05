// bytecode definitions
#ifndef __FN_BYTES_HPP
#define __FN_BYTES_HPP

#include "base.hpp"
#include "vm.hpp"

#include <iostream>

namespace fn_bytes {

using namespace fn;

/// instruction opcodes

// note: whenever an instruction uses a value on the stack, that value is popped off.

// nop; do absolutely nothing
constexpr u8 OP_NOP = 0x00;

// pop; pop one element off the top of the stack
constexpr u8 OP_POP = 0x01;
// local b_yt_e; access the b_yt_eth element of the stack, indexed from the bottom
constexpr u8 OP_LOCAL = 0x02;
// set-local b_yt_e; set the b_yt_eth element of the stack to the current top of the stack
constexpr u8 OP_SET_LOCAL = 0x03;
// copy b_yt_e; works like OP_LOCAL but its indices count down from the top of the stack
constexpr u8 OP_COPY = 0x04;

// upvalue b_yt_e; get the b_yt_eth upvalue
constexpr u8 OP_UPVALUE = 0x05;
// set-upvalue b_yt_e; set the b_yt_eth upvalue to the value on top of the stack
constexpr u8 OP_SET_UPVALUE = 0x06;
// closure s_ho_rt; instantiate a closure using s_ho_rt as the function i_d
constexpr u8 OP_CLOSURE = 0x07;
// close b_yt_e; pop the stack b_yt_e times, closing any open upvalues in the process
constexpr u8 OP_CLOSE = 0x08;

// global; get a global variable based on a string on top of the stack
constexpr u8 OP_GLOBAL = 0x10;
// set-global; set a global variable. stack arguments ->[value] symbol ...
// note: leaves the symbol on the stack
constexpr u8 OP_SET_GLOBAL = 0x11;

// const s_ho_rt; load a constant via its 16-bit i_d
constexpr u8 OP_CONST = 0x12;
// null; push a null value on top of the stack
constexpr u8 OP_NULL  = 0x13;
// false; push a false value on top of the stack
constexpr u8 OP_FALSE = 0x14;
// true; push a true value on top of the stack
constexpr u8 OP_TRUE  = 0x15;


// obj-get; get the value of a property. stack arguments ->[key] obj ...
constexpr u8 OP_OBJ_GET = 0x16;
// obj-set; add or update the value of a property. stack arguments ->[new-value] key obj ...
constexpr u8 OP_OBJ_SET = 0x17;

// t_od_o: 
// t_od_o: i_mp_le_me_nt t_he_se c_ha_ng_es i_n v_m
// t_od_o: 
// module; change the current module. stack arguments ->[module-object] ...
constexpr u8 OP_MODULE = 0x18;
// import; given a module id list, put the corresponding module object on top of
// the stack. if no such module exists, a new one will be created. stack
// arguments ->[module-id-list] ...
constexpr u8 OP_IMPORT = 0x19;


// control flow & function calls

// jump s_ho_rt; add signed s_ho_rt to ip
constexpr u8 OP_JUMP = 0x30;
// cjump s_ho_rt; if top of the stack is falsey, add signed s_ho_rt to ip
constexpr u8 OP_CJUMP = 0x31;
// call b_yt_e; perform a function call
constexpr u8 OP_CALL = 0x32;
// return; return from the current function
constexpr u8 OP_RETURN = 0x33;

// apply b_yt_e; like call, but the last argument is actually a list to be expanded as individual
// arugments
constexpr u8 OP_APPLY = 0x34;

// tcall b_yt_e; perform a tail call
//constexpr u8 OP_t_ca_ll = 0x35;


// might want this for implementing apply
// // apply;
// constexpr u8 OP_APPLY = 0x34;

// might want this for implementing t_co
// // long-jump a_dd_r; jump to the given 32-bit address
// constexpr u8 OP_l_on_g_JUMP = 0x35;


// gives the width of an instruction + its operands in bytes
inline u8 instr_width(u8 instr) {
    switch (instr) {
    case OP_NOP:
    case OP_POP:
    case OP_GLOBAL:
    case OP_SET_GLOBAL:
    case OP_NULL:
    case OP_FALSE:
    case OP_TRUE:
    case OP_RETURN:
    case OP_OBJ_GET:
    case OP_OBJ_SET:
    case OP_MODULE:
    case OP_IMPORT:
        return 1;
    case OP_LOCAL:
    case OP_SET_LOCAL:
    case OP_COPY:
    case OP_UPVALUE:
    case OP_SET_UPVALUE:
    case OP_CLOSE:
    case OP_CALL:
    case OP_APPLY:
        return 2;
    case OP_CONST:
    case OP_JUMP:
    case OP_CJUMP:
    case OP_CLOSURE:
        return 3;
    default:
        // t_od_o: shouldn't get here. maybe raise a warning?
        return 1;
    }
}


// disassembly a single instruction, writing output to out
void disassemble_instr(const bytecode& code, bc_addr ip, std::ostream& out);


void disassemble(const bytecode& code, std::ostream& out);


}


#endif
