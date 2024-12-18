/*
CPU class function signatures
*/

#ifndef CPU_H
#define CPU_H

// Include libraries
#include <cinttypes> // To use uint*_t

// CPU class
class CPU {
  private:
    // General purpose registers
    uint8_t A,B,C,D,E,F,H,L;
    // Program counter and stack pointer
    uint16_t PC,SP;
    // ETC.
    uint32_t opcodes_run;
    uint8_t cycles;
    uint32_t total_cycles;
    // PPU ETC. (may or may not keep)
    //uint16_t ppu_cycles;
    //uint8_t sprites_scanned;
    //uint8_t sprites_in_buffer;
    //uint8_t obj_penalty;
    //uint8_t wy_is_ly;
  public:
    // Create CPU object
    CPU();
    // CPU loop
    void cpuLoop(uint8_t* mem_map, uint32_t cycles_to_run);
    // Instruction functions
    // r8/r16 is any 8-bit/16-bit register
    // n8/n16 is a 8-bit/16-bit int constant
    // e8 is a 8-bit offset from -127 to 128
    // u3 is a 3-bit unsigned int constant
    uint8_t adc_a_r8(uint8_t *A, uint8_t *r8, uint8_t *F, uint16_t *PC);
    uint8_t bit_u3_r8(uint8_t u3, uint8_t r8, uint8_t *F, uint16_t *PC);
    uint8_t call_cc_n16(int16_t n16, uint8_t F, uint8_t cc, uint16_t *SP, uint16_t *PC, uint8_t *mem_map);
    uint8_t cp_A_n8_OR_r8(uint8_t A, uint8_t n8_OR_r8, uint8_t *F, uint16_t *PC, uint8_t is_n8);
    uint8_t dec_r8(uint8_t *r8, uint8_t *F, uint16_t *PC);
    uint8_t inc_HL(uint8_t *HL, uint8_t *F, uint16_t *PC, uint8_t mem_map);
    uint8_t inc_r16(uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC);
    uint8_t inc_r8(uint8_t *r8, uint8_t *F, uint16_t *PC);
    uint8_t inc_SP(uint16_t *SP, uint16_t *PC);
    uint8_t ld_A_ff00_C(uint8_t *A, uint8_t C, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_A_ff00_n8(uint8_t *A, uint8_t n8, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_ff00_C_A(uint8_t A, uint8_t C, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_ff00_n8_A(uint8_t A, uint8_t n8, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_HLID_r8(uint8_t r8, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC, uint8_t *mem_map, uint8_t is_increment);
    uint8_t ld_n16_r8(uint8_t r8, uint16_t n16, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_r16_r8(uint8_t r8, uint8_t r8_HIGH, uint8_t r8_LOW, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_r8_HLID(uint8_t *r8, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC, uint8_t *mem_map, uint8_t is_increment);
    uint8_t ld_r8_r16(uint8_t *r8, uint8_t r8_HIGH, uint8_t r8_LOW, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_r8_dest_r8_src(uint8_t r8_src, uint8_t *r8_dest, uint16_t *PC);
    uint8_t ld_r16_n16(uint16_t n16, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC);
    uint8_t ld_r8_n8(uint8_t n8, uint8_t *r8, uint16_t *PC);
    uint8_t ld_r16_A(uint8_t *A, uint16_t r16, uint16_t *PC, uint8_t *mem_map);
    uint8_t ld_SP_n16(uint16_t n16, uint16_t *SP, uint16_t *PC);
    uint8_t pop_r16(uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *SP, uint16_t *PC, uint8_t *mem_map);
    uint8_t push_r16(uint8_t r8_HIGH, uint8_t r8_LOW, uint16_t *SP, uint16_t *PC, uint8_t *mem_map);
    uint8_t jr_cc_i8(int8_t i8, uint8_t F, uint8_t cc, uint16_t *PC, uint8_t *mem_map);
    uint8_t ret(uint16_t *SP, uint16_t *PC, uint8_t *mem_map);
    uint8_t ret_cc(uint8_t F, uint8_t cc, uint16_t *SP, uint16_t *PC, uint8_t *mem_map);
    uint8_t rlca(uint8_t *A, uint8_t *F, uint16_t *PC);
    uint8_t rl_r8(uint8_t *r8, uint8_t *F, uint16_t *PC);
    uint8_t rr_r8(uint8_t *r8, uint8_t *F, uint16_t *PC);
    uint8_t xor_a_r8(uint8_t *A, uint8_t *r8, uint8_t *F, uint16_t *PC);
    // Debug print out
    void debug(uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t F, uint8_t H, uint8_t L, uint16_t PC, uint16_t SP, uint32_t opcodes_run, uint32_t total_cycles, uint8_t *mem_map, uint8_t is_cb_opcode);
    // Delete all CPU related objects
    ~CPU();
};

#endif