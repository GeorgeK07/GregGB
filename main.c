/*
  Game Boy Emulator
*/

// Include libraries
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Instruction functions
// r8/r16 is any 8-bit/16-bit register
// n8/n16 is a 8-bit/16-bit int constant
// e8 is a 8-bit offset from -127 to 128
// u3 is a 3-bit unsigned int constant
uint8_t adc_a_r8(uint8_t *A, uint8_t *r8, uint8_t *F, uint16_t *PC) {
  // Add A, r8, and the carry flag if set
  // (*F >> 4) & 1 returns 1 if 4th bit is 1, and 0 if 0
  *A = *A + *r8 + ((*F >> 4) & 1);
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

uint8_t bit_u3_r8(uint8_t u3, uint8_t r8, uint8_t *F, uint16_t *PC) {
  // Test bit u3 in r8
  uint8_t result = (r8 >> u3) & 1;
  // printf("RESULT: %d\n", result); // DEBUG
  // If said bit is 0, set zero flag in F, if not, clear it
  switch (result) {
    case 0:
      *F |= 1 << 7;
      break;
    case 1: // Might be right, matches with BGB behaviour
      *F &= !(1 << 7);
      break;
  }
  // Set subtraction flag to 0 and half carry flag to 1
  *F &= ~(1 << 6);
  *F |= 1 << 5;
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t call_cc_n16(int16_t n16, uint8_t F, uint8_t cc, uint16_t *SP, uint16_t *PC, uint8_t *mem_map) {
  //  Call address n16, pushes address of next instruction (pointed by stack
  // pointer) to stack if cc is true (0=NZ, 1=NC, 2=Z, 3=C, 4=none), then jumps
  // to n16
  if ((cc == 0 && (((F >> 7) & 1)) == 0) || (cc == 1 && (((F >> 4) & 1)) == 0)
  || (cc == 2 && (((F >> 7) & 1)) == 1) || (cc == 3 && (((F >> 4) & 1)) == 1)
  || cc == 4) {
    *PC = *PC + 3; // 3 byte opcode, add 3 to PC
    // Split program counter into high and low bytes
    uint8_t PC_HIGH = (*PC >> 8) & 0xff;
    uint8_t PC_LOW = *PC & 0xff;
    // Add each byte in correct order to stack
    mem_map[*SP - 1] = PC_HIGH;
    mem_map[*SP - 2] = PC_LOW;
    *PC = n16; // Jump to n16
    *SP = *SP - 2; // Subtract 2 from stack pointer
    return 6; // Return number of cycles (in M-cycles)
  }
  *PC = *PC + 3; // 3 byte opcode, add 3 to PC
  return 3; // Return number of cycles (in M-cycles)
}

void debug(uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t F, uint8_t H, uint8_t L, uint16_t PC, uint16_t SP, uint32_t opcodes_run, uint32_t total_cycles, uint8_t *mem_map, uint8_t is_cb_opcode) {
  // Debugging print statements
  switch (is_cb_opcode) {
    case 1:
      printf("From cb\n");
  }
  printf("Unemulated opcode %02x\n", mem_map[PC]);
  printf("A:%02x B:%02x C:%02x D:%02x E:%02x F:%02x H:%02x L:%02x\n", A, B, C, D, E, F, H, L);
  printf("Z:%x N:%x H:%x C:%x\n", (F >> 7) & 1, (F >> 6) & 1, (F >> 5) & 1, (F >> 4) & 1);
  printf("PC:%04x SP:%04x\n", PC, SP); // Print program counter and stack pointer
  printf("OPCODES RUN: %d\n", opcodes_run); // Print total opcodes run
  printf("TOTAL CYCLES: %d\n\n", total_cycles); // Print total cycles
  // For loop that prints mem_map
  printf("MEM_MAP:\n");
  for (int i = 0; i < 65536; i++) {
    // Check if first byte, if so, print offset
    switch (i & 0x0f) { // Bitmask top and check if bottom byte equals 0x00
      case 0x00:
        printf("0x%04x ", i);
    }
    printf("%02x ", mem_map[i]); // Print current hex value
    // Check if last byte, if so, print line end
    switch (i & 0x0f) { // Bitmask top and check if bottom byte equals 0x0f
      case 0x0f:
        printf("\n");
    }
  }
  exit(1);
}

uint8_t dec_r8(uint8_t *r8, uint8_t *F, uint16_t *PC) {
  // Decrease r8 by 1
  // Set subtraction flag to 1 and half carry flag to 1 if borrow from bit 4
  *F |= 1 << 6;
  // If (r8 & 0x0f) 00001000 & 00001111 = 00001000 (bitmask top values)
  // (1 & 0x0f) 00000001 & 00001111 = 00000001 (bitmask top values)
  // 00001000 - 00000001 = 00000111
  // 00000111 & 00010000 = 00000000 therefore no half borrow took place
  // If (62 & 0x0f) 00111110 & 00001111 = 00001110 (bitmask top values)
  // (34 & 0x0f) 00100010 & 00001111 = 00000010 (bitmask top values)
  // 00001110 - 00000010 = 00001100
  // 00001100 & 00010000 = 00000000 therefore no half borrow took place
  // If (34 & 0x0f) 00100010 & 00001111 = 00000010 (bitmask top values)
  // (62 & 0x0f) 00111110 & 00001111 = 00001110 (bitmask top values)
  // 00000010 - 00001110 = 11110011
  // 11110011 & 00010000 = 00010000 therefore half borrow took place (true?)
  switch (((*r8 & 0x0f) - (1 & 0x0f)) & 0x10) {
    case 0x10:
      *F |= 1 << 5;
      break;
    default:
      *F &= ~(1 << 5);
  }
  *r8 = *r8 - 1;
  // Set zero flag to 1 if result is 0
  switch (*r8) {
    case 0:
      *F |= 1 << 7;
      break;
    default:
      *F &= ~(1 << 7);
  }
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

uint8_t inc_HL(uint8_t *HL, uint8_t *F, uint16_t *PC, uint8_t mem_map) {
  // Increase byte pointed to by HL by 1
  // Set subtraction flag to 0 and half carry flag to 1 if overflow from bit 3
  *F &= ~(1 << 6);
  // If (r8 & 0x0f) 00001000 & 00001111 = 00001000 (bitmask top values)
  // (1 & 0x0f) 00000001 & 00001111 = 000000001 (bitmask top values)
  // 00001000 + 00000001 = 00001001
  // 00001001 & 00010000 = 00000000 therefore no half carry took place
  // If (62 & 0x0f) 00111110 & 00001111 = 00001110 (bitmask top values)
  // (34 & 0x0f) 00100010 & 00001111 = 00000010 (bitmask top values)
  // 00001110 + 00000010 = 00010000
  // 00010000 & 00010000 = 00010000 therefore half carry took place
  switch (((mem_map[HL] & 0x000f) + (1 & 0x000f)) & 0x0010) {
    case 0x0010:
      *F |= 1 << 5;
      break;
    default:
      *F &= ~(1 << 5);
  }
  mem_map[HL] = mem_map[HL] + 1;
  // Set zero flag to 1 if result is 0
  switch (mem_map[HL]) {
    case 0:
      *F |= 1 << 7;
      break;
    default:
      *F &= ~(1 << 7);
  }
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t inc_r16(uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC) {
  // Increase r16 by 1
  uint16_t r16 = (*r8_HIGH << 8) + *r8_LOW; // Join r8_HIGH and r8_LOW
  r16 = r16 + 1;
  // Split r8_HIGH and r8_LOW
  *r8_HIGH = (r16 >> 8) & 0xff;
  *r8_LOW = r16 & 0xff;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t inc_r8(uint8_t *r8, uint8_t *F, uint16_t *PC) {
  // Increase r8 by 1
  // Set subtraction flag to 0 and half carry flag to 1 if overflow from bit 3
  *F &= ~(1 << 6);
  // If (r8 & 0x0f) 00001000 & 00001111 = 00001000 (bitmask top values)
  // (1 & 0x0f) 00000001 & 00001111 = 000000001 (bitmask top values)
  // 00001000 + 00000001 = 00001001
  // 00001001 & 00010000 = 00000000 therefore no half carry took place
  // If (62 & 0x0f) 00111110 & 00001111 = 00001110 (bitmask top values)
  // (34 & 0x0f) 00100010 & 00001111 = 00000010 (bitmask top values)
  // 00001110 + 00000010 = 00010000
  // 00010000 & 00010000 = 00010000 therefore half carry took place
  switch (((*r8 & 0x0f) + (1 & 0x0f)) & 0x10) {
    case 0x10:
      *F |= 1 << 5;
      break;
    default:
      *F &= ~(1 << 5);
  }
  *r8 = *r8 + 1;
  // Set zero flag to 1 if result is 0
  switch (*r8) {
    case 0:
      *F |= 1 << 7;
      break;
    default:
      *F &= ~(1 << 7);
  }
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

uint8_t inc_SP(uint16_t *SP, uint16_t *PC) {
  // Increase SP by 1
  *SP = *SP + 1;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_A_ff00_C(uint8_t *A, uint8_t C, uint16_t *PC, uint8_t *mem_map) {
  // Store 0xff00 + C in mem_map at A
  *A = mem_map[0xff00 + C];
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_A_ff00_n8(uint8_t *A, uint8_t n8, uint16_t *PC, uint8_t *mem_map) {
  // Store 0xff00 + n8 in mem_map at A
  *A = mem_map[0xff00 + n8];
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t ld_ff00_C_A(uint8_t A, uint8_t C, uint16_t *PC, uint8_t *mem_map) {
  // Store A at 0xff00 + C in mem_map
  mem_map[0xff00 + C] = A;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_ff00_n8_A(uint8_t A, uint8_t n8, uint16_t *PC, uint8_t *mem_map) {
  // Store A at 0xff00 + n8 in mem_map
  mem_map[0xff00 + n8] = A;
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t ld_HLID_r8(uint8_t r8, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC, uint8_t *mem_map, uint8_t is_increment) {
  // Store r8 at memory r16 (HL) points to, then increment or decrement HL
  // printf("HIGH (BDH): %02x LOW (CEL): %02x\n", *r8_HIGH, *r8_LOW); // DEBUG
  // printf("r8 VAL: %02x", r8); // DEBUG
  uint16_t r16 = (*r8_HIGH << 8) + *r8_LOW; // Join r8_HIGH and r8_LOW
  mem_map[r16] = r8;
  // printf("r16: %04x\n", r16); // DEBUG
  // Check if HL should be incremented or decremented
  switch (is_increment) {
    case 0:
      r16 = r16 - 1;
      break;
    case 1:
      r16 = r16 + 1;
      break;
  }
  // Split r8_HIGH and r8_LOW
  *r8_HIGH = (r16 >> 8) & 0xff;
  *r8_LOW = r16 & 0xff;
  // printf("HIGH (BDH): %02x LOW (CEL): %02x\n", *r8_HIGH, *r8_LOW); // DEBUG
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_r16_r8(uint8_t r8, uint16_t r16, uint16_t *PC, uint8_t *mem_map) {
  // Store r8 at memory r16 points to
  mem_map[r16] = r8;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_r8_HLID(uint8_t *r8, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC, uint8_t *mem_map, uint8_t is_increment) {
  // Store memory r16 (HL) points to in r8, then increment or decrement HL
  uint16_t r16 = (*r8_HIGH << 8) + *r8_LOW; // Join r8_HIGH and r8_LOW
  *r8 = mem_map[r16];
  // Check if HL should be incremented or decremented
  switch (is_increment) {
    case 0:
      r16 = r16 - 1;
      break;
    case 1:
      r16 = r16 + 1;
      break;
  }
  // Split r8_HIGH and r8_LOW
  *r8_HIGH = (r16 >> 8) & 0xff;
  *r8_LOW = r16 & 0xff;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_r8_r16(uint8_t *r8, uint8_t r8_HIGH, uint8_t r8_LOW, uint16_t *PC, uint8_t *mem_map) {
  // Store memory r16 points to in r8
  uint16_t r16 = (r8_HIGH << 8) + r8_LOW; // Join r8_HIGH and r8_LOW
  // printf("r16 VAL:%04x\n", r16); // DEBUG
  // printf("VAL r16 POINTS TO:%02x\n", mem_map[r16]); // DEBUG
  *r8 = mem_map[r16];
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_r8_dest_r8_src(uint8_t r8_src, uint8_t *r8_dest, uint16_t *PC) {
  // Set r8_dest to r8_src
  *r8_dest = r8_src;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

uint8_t ld_r16_n16(uint16_t n16, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC) {
  // Set r16 to n16
  // printf("HIGH (BDH): %02x LOW (CEL): %02x\n", *r8_HIGH, *r8_LOW); // DEBUG
  uint16_t r16 = (*r8_HIGH << 8) + *r8_LOW; // Join r8_HIGH and r8_LOW
  r16 = n16;
  // printf("r16: %04x\n", r16); // DEBUG
  // Split r8_HIGH and r8_LOW
  *r8_HIGH = (r16 >> 8) & 0xff;
  *r8_LOW = r16 & 0xff;
  // printf("HIGH (BDH): %02x LOW (CEL): %02x\n", *r8_HIGH, *r8_LOW); // DEBUG
  *PC = *PC + 3; // 3 byte opcode, add 3 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t ld_r8_n8(uint8_t n8, uint8_t *r8, uint16_t *PC) {
  // Set r8 to n8
  // printf("n8: %02x\n", n8); // DEBUG
  *r8 = n8;
  // printf("r8: %02x\n", *r8); // DEBUG
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_r16_A(uint8_t *A, uint16_t r16, uint16_t *PC, uint8_t *mem_map) {
  // Set A to value r16 points to
  *A = mem_map[r16];
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_SP_n16(uint16_t n16, uint16_t *SP, uint16_t *PC) {
  // Set SP to n16
  *SP = n16;
  *PC = *PC + 3; // 3 byte opcode, add 3 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t pop_r16(uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *SP, uint16_t *PC, uint8_t *mem_map) {
  //  Pop 2 bytes from stack into registers and have stack pointer be moved
  // back to how it was before the push
  *r8_LOW = mem_map[*SP];
  *r8_HIGH = mem_map[*SP + 1];
  *SP = *SP + 2; // Add 2 to stack pointer
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t push_r16(uint8_t r8_HIGH, uint8_t r8_LOW, uint16_t *SP, uint16_t *PC, uint8_t *mem_map) {
  // Push r16 into stack
  mem_map[*SP - 1] = r8_HIGH;
  mem_map[*SP - 2] = r8_LOW;
  *SP = *SP - 2; // Subtract 2 from stack pointer
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 4; // Return number of cycles (in M-cycles)
}

uint8_t jr_cc_i8(int8_t i8, uint8_t F, uint8_t cc, uint16_t *PC, uint8_t *mem_map) {
  // Jump to address i8 if cc is true (0=NZ, 1=NC, 2=Z, 3=C, 4=none)
  if ((cc == 0 && (((F >> 7) & 1)) == 0) || (cc == 1 && (((F >> 4) & 1)) == 0)
  || (cc == 2 && (((F >> 7) & 1)) == 1) || (cc == 3 && (((F >> 4) & 1)) == 1)
  || cc == 4) {
    // printf("i8: %d\n", i8); // DEBUG
    *PC = *PC + 2; // 2 byte opcode, add 2 to PC
    *PC = *PC + i8;
    // printf("PC: %d\n", *PC); // DEBUG
    // exit(1); // DEBUG
    return 3; // Return number of cycles (in M-cycles)
  }
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ret(uint16_t *SP, uint16_t *PC, uint8_t *mem_map) {
  //  Pop 2 bytes from stack into PC and have stack pointer be moved
  // back to how it was before the push
  // Separate PC into PC_HIGH and PC_LOW
  uint8_t PC_HIGH = (*PC >> 8) & 0xff;
  uint8_t PC_LOW = *PC & 0xff;
  PC_LOW = mem_map[*SP];
  PC_HIGH = mem_map[*SP + 1];
  *PC = (PC_HIGH << 8) + PC_LOW; // Join PC_HIGH and PC_LOW
  *SP = *SP + 2; // Add 2 to stack pointer
  // *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 4; // Return number of cycles (in M-cycles)
}

uint8_t ret_cc(uint8_t F, uint8_t cc, uint16_t *SP, uint16_t *PC, uint8_t *mem_map) {
  //  Pop 2 bytes from stack into PC and have stack pointer be moved
  // back to how it was before the push
  // Do this if cc is true (0=NZ, 1=NC, 2=Z, 3=C, 4=none)
  if ((cc == 0 && (((F >> 7) & 1)) == 0) || (cc == 1 && (((F >> 4) & 1)) == 0)
  || (cc == 2 && (((F >> 7) & 1)) == 1) || (cc == 3 && (((F >> 4) & 1)) == 1)) {
    // Separate PC into PC_HIGH and PC_LOW
    uint8_t PC_HIGH = (*PC >> 8) & 0xff;
    uint8_t PC_LOW = *PC & 0xff;
    PC_LOW = mem_map[*SP];
    PC_HIGH = mem_map[*SP + 1];
    *PC = (PC_HIGH << 8) + PC_LOW; // Join PC_HIGH and PC_LOW
    *SP = *SP + 2; // Add 2 to stack pointer
    // *PC = *PC + 1; // 1 byte opcode, add 1 to PC
    return 5; // Return number of cycles (in M-cycles)
  }
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t rlca(uint8_t *A, uint8_t *F, uint16_t *PC) {
  // Rotate bits in A left through carry
  // printf("A bitmasked: %02x\n", *r8 & 0x80); // DEBUG
  uint8_t A_copy = *A; // Copy A into another variable to compare later
  *A = *A << 1;
  // If carry flag 1, add 1 to A
  switch (*F & 0x10) {
    case 0x10:
      *A = *A + 1;
      break;
  }
  //  Bitmask all of A_copy except for 7th bit, then make carry flag equal 1 or 0
  // depending on what is in it (carry flag stores old 7th bit value, 0 or 1)
  switch (A_copy & 0x80) {
    case 0x80:
      *F |= 1 << 4;
      break;
    default:
      *F &= ~(1 << 4);
  }
  // Clear zero, subtraction, and half carry flags
  *F &= ~(1 << 7);
  *F &= ~(1 << 6);
  *F &= ~(1 << 5);
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

uint8_t rl_r8(uint8_t *r8, uint8_t *F, uint16_t *PC) {
  // Rotate bits in r8 left through carry
  // printf("r8 bitmasked: %02x\n", *r8 & 0x80); // DEBUG
  uint8_t r8_copy = *r8; // Copy r8 into another variable to compare later
  *r8 = *r8 << 1;
  // If carry flag 1, add 1 to r8
  switch (*F & 0x10) {
    case 0x10:
      *r8 = *r8 + 1;
      break;
  }
  //  Bitmask all of r8_copy except for 7th bit, then make carry flag equal 1 or 0
  // depending on what is in it (carry flag stores old 7th bit value, 0 or 1)
  switch (r8_copy & 0x80) {
    case 0x80:
      *F |= 1 << 4;
      break;
    default:
      *F &= ~(1 << 4);
  }
  // If r8 is 0, set zero flag to 1
  switch (*r8) {
    case 0:
      *F |= 1 << 7;
  }
  // Clear subtraction and half carry flag
  *F &= ~(1 << 6);
  *F &= ~(1 << 5);
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t rr_r8(uint8_t *r8, uint8_t *F, uint16_t *PC) {
  // Rotate bits in r8 right through carry
  // printf("r8 bitmasked: %02x\n", *r8 & 0x01); // DEBUG
  uint8_t r8_copy = *r8; // Copy r8 into another variable to compare later
  *r8 = *r8 >> 1;
  // If carry flag 1, add 0x80 to r8
  switch (*F & 0x10) {
    case 0x10:
      *r8 = *r8 + 0x80;
      break;
  }
  //  Bitmask all of r8_copy except for 7th bit, then make carry flag equal 1 or 0
  // depending on what is in it (carry flag stores old 7th bit value, 0 or 1)
  switch (r8_copy & 0x01) {
    case 0x01:
      *F |= 1 << 4;
      break;
    default:
      *F &= ~(1 << 4);
  }
  // If r8 is 0, set zero flag to 1
  switch (*r8) {
    case 0:
      *F |= 1 << 7;
  }
  // Clear subtraction and half carry flag
  *F &= ~(1 << 6);
  *F &= ~(1 << 5);
  *PC = *PC + 2; // 2 byte opcode, add 2 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t xor_a_r8(uint8_t *A, uint8_t *r8, uint8_t *F, uint16_t *PC) {
  // Bitwise XOR r8 and A
  uint8_t result = *r8 ^ *A;
  switch (result) {
    case 0:
      *F |= 1 << 7;
  }
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

// Main function
int main(void) {
  // Initialize registers (char or uint8_t, portable and exact)
  // 0x for hex, 0b for bytes
  uint8_t A = 0;
  uint8_t B = 0;
  uint8_t C = 0;
  uint8_t D = 0;
  uint8_t E = 0;
  uint8_t F = 0b00000000; // Flags register (only top 4 bits used, rest is blank)
  uint8_t H = 0;
  uint8_t L = 0;

  // Initialize program counter and stack pointer
  uint16_t PC = 0;
  uint16_t SP = 0;

  // Initialize memory map
  uint8_t *mem_map = malloc(sizeof(uint8_t) * 65536);
  mem_map[0] = 0;

  // Reading Boot ROM into memory
  FILE *rom_ptr = 0;
  rom_ptr = fopen("C:/Users/BellwetherWealth-Enq/Games/ROMs/GB/[BIOS] Nintendo Game Boy Boot ROM (World) (Rev 1).gb", "r");
  // File ptr, offset, where offset added (SEEK_SET, SEEK_CUR, SEEK_END)
  fseek(rom_ptr, 0, SEEK_SET); // Set place in file to read from
  // Memory ptr, size of each element (in bytes), number of elements, file ptr
  fread(mem_map, 1, 256, rom_ptr); // Reads Boot ROM into start of mem_map
  fclose(rom_ptr); // Close to prevent issues

  //  Reading cartridge into memory (first 32kb minus first 256b that Boot ROM
  // is in, will overwrite after boot process)
  rom_ptr = 0;
  rom_ptr = fopen("C:/Users/BellwetherWealth-Enq/Games/ROMs/GB/Dr. Mario (World).gb", "r");
  // File ptr, offset, where offset added (SEEK_SET, SEEK_CUR, SEEK_END)
  fseek(rom_ptr, 0x100, SEEK_SET); // Set place in file to read from
  // Memory ptr, size of each element (in bytes), number of elements, file ptr
  fread(mem_map + 0x100, 1, 32512, rom_ptr); // Reads ROM into after Boot ROM
  fclose(rom_ptr); // Close to prevent issues

  // ETC.
  uint8_t is_running = 1;
  uint32_t opcodes_run = 0;
  uint8_t cycles = 0;
  uint32_t total_cycles = 0;

  // While loop for CPU fetch, decode, execute process
  while (is_running == 1) {
    // printf("Opcode: %02x\n", mem_map[PC]); // DEBUG: Print current byte in hex
    //  Switch statement that checks hex value of current byte, compares with
    // opcode values, and then executes said opcode
    switch (mem_map[PC]) {
      case 0x00: PC++; cycles = 1; break;
      case 0x01:
        cycles = ld_r16_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &B, &C, &PC);
        break;
      case 0x02:
        cycles = ld_r16_A(&A, (C << 8) + B, &PC, mem_map);
        break;
      case 0x03:
        cycles = inc_r16(&B, &C, &PC);
        break;
      case 0x04:
        cycles = inc_r8(&B, &F, &PC);
        break;
      case 0x05:
        cycles = dec_r8(&B, &F, &PC);
        break;
      case 0x06:
        cycles = ld_r8_n8(mem_map[PC+1], &B, &PC);
        break;
      case 0x0a:
        cycles = ld_r8_r16(&A, B, C, &PC, mem_map);
        break;
      case 0x0c:
        cycles = inc_r8(&C, &F, &PC);
        break;
      case 0x0d:
        cycles = dec_r8(&C, &F, &PC);
        break;
      case 0x0e:
        cycles = ld_r8_n8(mem_map[PC+1], &C, &PC);
        break;
      case 0x11:
        cycles = ld_r16_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &D, &E, &PC);
        break;
      case 0x12:
        cycles = ld_r16_A(&A, (E << 8) + D, &PC, mem_map);
        break;
      case 0x13:
        cycles = inc_r16(&D, &E, &PC);
        break;
      case 0x14:
        cycles = inc_r8(&D, &F, &PC);
        break;
      case 0x15:
        cycles = dec_r8(&D, &F, &PC);
        break;
      case 0x16:
        cycles = ld_r8_n8(mem_map[PC+1], &D, &PC);
        break;
      case 0x17:
        cycles = rlca(&A, &F, &PC);
        break;
      case 0x18:
        cycles = jr_cc_i8(mem_map[PC+1], F, 4, &PC, mem_map);
        break;
      case 0x1a:
        cycles = ld_r8_r16(&A, D, E, &PC, mem_map);
        break;
      case 0x1c:
        cycles = inc_r8(&E, &F, &PC);
        break;
      case 0x1d:
        cycles = dec_r8(&E, &F, &PC);
        break;
      case 0x1e:
        cycles = ld_r8_n8(mem_map[PC+1], &E, &PC);
        break;
      case 0x20:
        cycles = jr_cc_i8(mem_map[PC+1], F, 0, &PC, mem_map);
        break;
      case 0x21:
        cycles = ld_r16_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &H, &L, &PC);
        break;
      case 0x22:
        cycles = ld_HLID_r8(A, &H, &L, &PC, mem_map, 1);
        break;
      case 0x23:
        cycles = inc_r16(&H, &L, &PC);
        break;
      case 0x24:
        cycles = inc_r8(&H, &F, &PC);
        break;
      case 0x25:
        cycles = dec_r8(&H, &F, &PC);
        break;
      case 0x26:
        cycles = ld_r8_n8(mem_map[PC+1], &H, &PC);
        break;
      case 0x28:
        cycles = jr_cc_i8(mem_map[PC+1], F, 2, &PC, mem_map);
        break;
      case 0x2a:
        cycles = ld_r8_HLID(&A, &H, &L, &PC, mem_map, 1);
        break;
      case 0x2c:
        cycles = inc_r8(&L, &F, &PC);
        break;
      case 0x2d:
        cycles = dec_r8(&L, &F, &PC);
        break;
      case 0x2e:
        cycles = ld_r8_n8(mem_map[PC+1], &L, &PC);
        break;
      case 0x30:
        cycles = jr_cc_i8(mem_map[PC+1], F, 1, &PC, mem_map);
        break;
      case 0x31:
        cycles = ld_SP_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &SP, &PC);
        break;
      case 0x32:
        cycles = ld_HLID_r8(A, &H, &L, &PC, mem_map, 0);
        break;
      case 0x33:
        cycles = inc_SP(&SP, &PC);
        break;
      case 0x38:
        cycles = jr_cc_i8(mem_map[PC+1], F, 3, &PC, mem_map);
        break;
      case 0x3a:
        cycles = ld_r8_HLID(&A, &H, &L, &PC, mem_map, 0);
        break;
      case 0x3e:
        cycles = ld_r8_n8(mem_map[PC+1], &A, &PC);
        break;
      case 0x40:
        cycles = ld_r8_dest_r8_src(B, &B, &PC);
        break;
      case 0x41:
        cycles = ld_r8_dest_r8_src(C, &B, &PC);
        break;
      case 0x42:
        cycles = ld_r8_dest_r8_src(D, &B, &PC);
        break;
      case 0x43:
        cycles = ld_r8_dest_r8_src(E, &B, &PC);
        break;
      case 0x44:
        cycles = ld_r8_dest_r8_src(H, &B, &PC);
        break;
      case 0x45:
        cycles = ld_r8_dest_r8_src(L, &B, &PC);
        break;
      case 0x47:
        cycles = ld_r8_dest_r8_src(A, &B, &PC);
        break;
      case 0x48:
        cycles = ld_r8_dest_r8_src(B, &C, &PC);
        break;
      case 0x49:
        cycles = ld_r8_dest_r8_src(C, &C, &PC);
        break;
      case 0x4a:
        cycles = ld_r8_dest_r8_src(D, &C, &PC);
        break;
      case 0x4b:
        cycles = ld_r8_dest_r8_src(E, &C, &PC);
        break;
      case 0x4c:
        cycles = ld_r8_dest_r8_src(H, &C, &PC);
        break;
      case 0x4d:
        cycles = ld_r8_dest_r8_src(L, &C, &PC);
        break;
      case 0x4f:
        cycles = ld_r8_dest_r8_src(A, &C, &PC);
        break;
      case 0x50:
        cycles = ld_r8_dest_r8_src(B, &D, &PC);
        break;
      case 0x51:
        cycles = ld_r8_dest_r8_src(C, &D, &PC);
        break;
      case 0x52:
        cycles = ld_r8_dest_r8_src(D, &D, &PC);
        break;
      case 0x53:
        cycles = ld_r8_dest_r8_src(E, &D, &PC);
        break;
      case 0x54:
        cycles = ld_r8_dest_r8_src(H, &D, &PC);
        break;
      case 0x55:
        cycles = ld_r8_dest_r8_src(L, &D, &PC);
        break;
      case 0x57:
        cycles = ld_r8_dest_r8_src(A, &D, &PC);
        break;
      case 0x58:
        cycles = ld_r8_dest_r8_src(B, &E, &PC);
        break;
      case 0x59:
        cycles = ld_r8_dest_r8_src(C, &E, &PC);
        break;
      case 0x5a:
        cycles = ld_r8_dest_r8_src(D, &E, &PC);
        break;
      case 0x5b:
        cycles = ld_r8_dest_r8_src(E, &E, &PC);
        break;
      case 0x5c:
        cycles = ld_r8_dest_r8_src(H, &E, &PC);
        break;
      case 0x5d:
        cycles = ld_r8_dest_r8_src(L, &E, &PC);
        break;
      case 0x5f:
        cycles = ld_r8_dest_r8_src(A, &E, &PC);
        break;
      case 0x60:
        cycles = ld_r8_dest_r8_src(B, &H, &PC);
        break;
      case 0x61:
        cycles = ld_r8_dest_r8_src(C, &H, &PC);
        break;
      case 0x62:
        cycles = ld_r8_dest_r8_src(D, &H, &PC);
        break;
      case 0x63:
        cycles = ld_r8_dest_r8_src(E, &H, &PC);
        break;
      case 0x64:
        cycles = ld_r8_dest_r8_src(H, &H, &PC);
        break;
      case 0x65:
        cycles = ld_r8_dest_r8_src(L, &H, &PC);
        break;
      case 0x67:
        cycles = ld_r8_dest_r8_src(A, &H, &PC);
        break;
      case 0x68:
        cycles = ld_r8_dest_r8_src(B, &L, &PC);
        break;
      case 0x69:
        cycles = ld_r8_dest_r8_src(C, &L, &PC);
        break;
      case 0x6a:
        cycles = ld_r8_dest_r8_src(D, &L, &PC);
        break;
      case 0x6b:
        cycles = ld_r8_dest_r8_src(E, &L, &PC);
        break;
      case 0x6c:
        cycles = ld_r8_dest_r8_src(H, &L, &PC);
        break;
      case 0x6d:
        cycles = ld_r8_dest_r8_src(L, &L, &PC);
        break;
      case 0x6f:
        cycles = ld_r8_dest_r8_src(A, &L, &PC);
        break;
      case 0x77:
        cycles = ld_r16_r8(A, (mem_map[PC+2] << 8) + mem_map[PC+1], &PC, mem_map);
        break;
      case 0x88:
        cycles = adc_a_r8(&A, &B, &F, &PC);
        break;
      case 0x89:
        cycles = adc_a_r8(&A, &C, &F, &PC);
        break;
      case 0x8a:
        cycles = adc_a_r8(&A, &D, &F, &PC);
        break;
      case 0x8b:
        cycles = adc_a_r8(&A, &E, &F, &PC);
        break;
      case 0x8c:
        cycles = adc_a_r8(&A, &H, &F, &PC);
        break;
      case 0x8d:
        cycles = adc_a_r8(&A, &L, &F, &PC);
        break;
      case 0xa8:
        cycles = xor_a_r8(&A, &B, &F, &PC);
        break;
      case 0xa9:
        cycles = xor_a_r8(&A, &C, &F, &PC);
        break;
      case 0xaa:
        cycles = xor_a_r8(&A, &D, &F, &PC);
        break;
      case 0xab:
        cycles = xor_a_r8(&A, &E, &F, &PC);
        break;
      case 0xac:
        cycles = xor_a_r8(&A, &H, &F, &PC);
        break;
      case 0xad:
        cycles = xor_a_r8(&A, &L, &F, &PC);
        break;
      case 0xaf:
        cycles = xor_a_r8(&A, &A, &F, &PC);
        break;
      case 0xc0:
        cycles = ret_cc(F, 0, &SP, &PC, mem_map);
        break;
      case 0xc1:
        cycles = pop_r16(&B, &C, &SP, &PC, mem_map);
        break;
      case 0xc4:
        cycles = call_cc_n16((mem_map[PC+2] << 8) + mem_map[PC+1], F, 0, &SP, &PC, mem_map);
        break;
      case 0xc5:
        cycles = push_r16(B, C, &SP, &PC, mem_map);
        break;
      case 0xc8:
        cycles = ret_cc(F, 2, &SP, &PC, mem_map);
        break;
      case 0xc9:
        cycles = ret(&SP, &PC, mem_map);
        break;
      case 0xcc:
        cycles = call_cc_n16((mem_map[PC+2] << 8) + mem_map[PC+1], F, 2, &SP, &PC, mem_map);
        break;
      case 0xcd:
        cycles = call_cc_n16((mem_map[PC+2] << 8) + mem_map[PC+1], F, 4, &SP, &PC, mem_map);
        break;
      case 0xd0:
        cycles = ret_cc(F, 1, &SP, &PC, mem_map);
        break;
      case 0xd1:
        cycles = pop_r16(&D, &E, &SP, &PC, mem_map);
        break;
      case 0xd4:
        cycles = call_cc_n16((mem_map[PC+2] << 8) + mem_map[PC+1], F, 1, &SP, &PC, mem_map);
        break;
      case 0xd5:
        cycles = push_r16(D, E, &SP, &PC, mem_map);
        break;
      case 0xd8:
        cycles = ret_cc(F, 3, &SP, &PC, mem_map);
        break;
      case 0xdc:
        cycles = call_cc_n16((mem_map[PC+2] << 8) + mem_map[PC+1], F, 3, &SP, &PC, mem_map);
        break;
      case 0xe0:
        cycles = ld_ff00_n8_A(A, mem_map[PC+1], &PC, mem_map);
        break;
      case 0xe1:
        cycles = pop_r16(&H, &L, &SP, &PC, mem_map);
        break;
      case 0xe2:
        cycles = ld_ff00_C_A(A, C, &PC, mem_map);
        break;
      case 0xe5:
        cycles = push_r16(H, L, &SP, &PC, mem_map);
        break;
      case 0xf0:
        cycles = ld_A_ff00_n8(&A, mem_map[PC+1], &PC, mem_map);
        break;
      case 0xf1:
        cycles = pop_r16(&A, &F, &SP, &PC, mem_map);
        break;
      case 0xf2:
        cycles = ld_A_ff00_C(&A, C, &PC, mem_map);
        break;
      case 0xf5:
        cycles = push_r16(A, F, &SP, &PC, mem_map);
        break;
      case 0xcb:
        // Debugging
        // printf("Opcode: %02x\n", mem_map[PC]); // DEBUG: Print current byte in hex
        switch (mem_map[PC+1]) {
          case 0x10:
            cycles = rl_r8(&B, &F, &PC);
            break;
          case 0x11:
            cycles = rl_r8(&C, &F, &PC);
            break;
          case 0x12:
            cycles = rl_r8(&D, &F, &PC);
            break;
          case 0x13:
            cycles = rl_r8(&E, &F, &PC);
            break;
          case 0x14:
            cycles = rl_r8(&H, &F, &PC);
            break;
          case 0x15:
            cycles = rl_r8(&L, &F, &PC);
            break;
          case 0x17:
            cycles = rl_r8(&A, &F, &PC);
            break;
          case 0x18:
            cycles = rr_r8(&B, &F, &PC);
            break;
          case 0x19:
            cycles = rr_r8(&C, &F, &PC);
            break;
          case 0x1a:
            cycles = rr_r8(&D, &F, &PC);
            break;
          case 0x1b:
            cycles = rr_r8(&E, &F, &PC);
            break;
          case 0x1c:
            cycles = rr_r8(&H, &F, &PC);
            break;
          case 0x1d:
            cycles = rr_r8(&L, &F, &PC);
            break;
          case 0x1f:
            cycles = rr_r8(&A, &F, &PC);
            break;
          case 0x40:
            cycles = bit_u3_r8(0, B, &F, &PC);
            break;
          case 0x41:
            cycles = bit_u3_r8(0, C, &F, &PC);
            break;
          case 0x42:
            cycles = bit_u3_r8(0, D, &F, &PC);
            break;
          case 0x43:
            cycles = bit_u3_r8(0, E, &F, &PC);
            break;
          case 0x44:
            cycles = bit_u3_r8(0, H, &F, &PC);
            break;
          case 0x45:
            cycles = bit_u3_r8(0, L, &F, &PC);
            break;
          case 0x47:
            cycles = bit_u3_r8(0, A, &F, &PC);
            break;
          case 0x48:
            cycles = bit_u3_r8(1, B, &F, &PC);
            break;
          case 0x49:
            cycles = bit_u3_r8(1, C, &F, &PC);
            break;
          case 0x4a:
            cycles = bit_u3_r8(1, D, &F, &PC);
            break;
          case 0x4b:
            cycles = bit_u3_r8(1, E, &F, &PC);
            break;
          case 0x4c:
            cycles = bit_u3_r8(1, H, &F, &PC);
            break;
          case 0x4d:
            cycles = bit_u3_r8(1, L, &F, &PC);
            break;
          case 0x4f:
            cycles = bit_u3_r8(1, A, &F, &PC);
            break;
          case 0x50:
            cycles = bit_u3_r8(2, B, &F, &PC);
            break;
          case 0x51:
            cycles = bit_u3_r8(2, C, &F, &PC);
            break;
          case 0x52:
            cycles = bit_u3_r8(2, D, &F, &PC);
            break;
          case 0x53:
            cycles = bit_u3_r8(2, E, &F, &PC);
            break;
          case 0x54:
            cycles = bit_u3_r8(2, H, &F, &PC);
            break;
          case 0x55:
            cycles = bit_u3_r8(2, L, &F, &PC);
            break;
          case 0x57:
            cycles = bit_u3_r8(2, A, &F, &PC);
            break;
          case 0x58:
            cycles = bit_u3_r8(3, B, &F, &PC);
            break;
          case 0x59:
            cycles = bit_u3_r8(3, C, &F, &PC);
            break;
          case 0x5a:
            cycles = bit_u3_r8(3, D, &F, &PC);
            break;
          case 0x5b:
            cycles = bit_u3_r8(3, E, &F, &PC);
            break;
          case 0x5c:
            cycles = bit_u3_r8(3, H, &F, &PC);
            break;
          case 0x5d:
            cycles = bit_u3_r8(3, L, &F, &PC);
            break;
          case 0x5f:
            cycles = bit_u3_r8(3, A, &F, &PC);
            break;
          case 0x60:
            cycles = bit_u3_r8(4, B, &F, &PC);
            break;
          case 0x61:
            cycles = bit_u3_r8(4, C, &F, &PC);
            break;
          case 0x62:
            cycles = bit_u3_r8(4, D, &F, &PC);
            break;
          case 0x63:
            cycles = bit_u3_r8(4, E, &F, &PC);
            break;
          case 0x64:
            cycles = bit_u3_r8(4, H, &F, &PC);
            break;
          case 0x65:
            cycles = bit_u3_r8(4, L, &F, &PC);
            break;
          case 0x67:
            cycles = bit_u3_r8(4, A, &F, &PC);
            break;
          case 0x68:
            cycles = bit_u3_r8(5, B, &F, &PC);
            break;
          case 0x69:
            cycles = bit_u3_r8(5, C, &F, &PC);
            break;
          case 0x6a:
            cycles = bit_u3_r8(5, D, &F, &PC);
            break;
          case 0x6b:
            cycles = bit_u3_r8(5, E, &F, &PC);
            break;
          case 0x6c:
            cycles = bit_u3_r8(5, H, &F, &PC);
            break;
          case 0x6d:
            cycles = bit_u3_r8(5, L, &F, &PC);
            break;
          case 0x6f:
            cycles = bit_u3_r8(5, A, &F, &PC);
            break;
          case 0x70:
            cycles = bit_u3_r8(6, B, &F, &PC);
            break;
          case 0x71:
            cycles = bit_u3_r8(6, C, &F, &PC);
            break;
          case 0x72:
            cycles = bit_u3_r8(6, D, &F, &PC);
            break;
          case 0x73:
            cycles = bit_u3_r8(6, E, &F, &PC);
            break;
          case 0x74:
            cycles = bit_u3_r8(6, H, &F, &PC);
            break;
          case 0x75:
            cycles = bit_u3_r8(6, L, &F, &PC);
            break;
          case 0x77:
            cycles = bit_u3_r8(6, A, &F, &PC);
            break;
          case 0x78:
            cycles = bit_u3_r8(7, B, &F, &PC);
            break;
          case 0x79:
            cycles = bit_u3_r8(7, C, &F, &PC);
            break;
          case 0x7a:
            cycles = bit_u3_r8(7, D, &F, &PC);
            break;
          case 0x7b:
            cycles = bit_u3_r8(7, E, &F, &PC);
            break;
          case 0x7c:
            cycles = bit_u3_r8(7, H, &F, &PC);
            break;
          case 0x7d:
            cycles = bit_u3_r8(7, L, &F, &PC);
            break;
          case 0x7f:
            cycles = bit_u3_r8(7, A, &F, &PC);
            break;
          default:
            debug(A, B, C, D, E, F, H, L, PC, SP, opcodes_run, total_cycles, mem_map, 1);
        } 
        total_cycles = total_cycles + cycles; // Add amount of cycles executed
        break;
      default:
        debug(A, B, C, D, E, F, H, L, PC, SP, opcodes_run, total_cycles, mem_map, 0);
    }
    opcodes_run++; // Add 1 to opcodes_run
    total_cycles = total_cycles + cycles; // Add amount of cycles executed
  }
}