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
  printf("RESULT: %d\n", result);
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
  *F |= 0 << 6;
  *F |= 1 << 5;
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 1; // Return number of cycles (in M-cycles)
}

uint8_t ld_r16_n16(uint16_t n16, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC) {
  // Set r16 to n16
  // printf("HIGH (BDH): %x LOW (CEL): %x\n", *r8_HIGH, *r8_LOW); // DEBUG
  uint16_t r16 = (*r8_HIGH << 8) + *r8_LOW; // Join r8_HIGH and r8_LOW
  r16 = n16;
  // printf("r16: %x\n", r16); // DEBUG
  // Split r8_HIGH and r8_LOW
  *r8_HIGH = (r16 >> 8) & 0xff;
  *r8_LOW = r16 & 0xff;
  // printf("HIGH (BDH): %x LOW (CEL): %x\n", *r8_HIGH, *r8_LOW); // DEBUG
  *PC = *PC + 3; // 3 byte opcode, add 3 to PC
  return 3; // Return number of cycles (in M-cycles)
}

uint8_t ld_r16_A(uint8_t *A, uint16_t r16, uint16_t *PC, uint8_t *mem_map) {
  // Set A to value r16 points to
  *A = mem_map[r16];
  *PC = *PC + 3; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_HLID_n16(uint8_t *A, uint8_t *r8_HIGH, uint8_t *r8_LOW, uint16_t *PC, uint8_t *mem_map, uint8_t is_increment) {
  // Set A to value r16 points to
  // printf("HIGH (BDH): %x LOW (CEL): %x\n", *r8_HIGH, *r8_LOW); // DEBUG
  uint16_t r16 = (*r8_HIGH << 8) + *r8_LOW; // Join r8_HIGH and r8_LOW
  *A = mem_map[r16];
  // printf("r16: %x\n", r16); // DEBUG
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
  // printf("HIGH (BDH): %x LOW (CEL): %x\n", *r8_HIGH, *r8_LOW); // DEBUG
  *PC = *PC + 1; // 1 byte opcode, add 1 to PC
  return 2; // Return number of cycles (in M-cycles)
}

uint8_t ld_SP_n16(uint16_t n16, uint16_t *SP, uint16_t *PC) {
  // Set SP to n16
  *SP = n16;
  *PC = *PC + 3; // 3 byte opcode, add 3 to PC
  return 3; // Return number of cycles (in M-cycles)
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

  // 
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

  // ETC.
  uint8_t is_running = 1;
  uint8_t cycles = 0;
  uint32_t total_cycles = 0;

  // While loop for CPU fetch, decode, execute process
  while (is_running == 1) {
    // Debugging
    printf("Opcode: %x\n", mem_map[PC]); // Print current byte in hex
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
      case 0x11:
        cycles = ld_r16_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &D, &E, &PC);
        break;
      case 0x12:
        cycles = ld_r16_A(&A, (E << 8) + D, &PC, mem_map);
        break;
      case 0x21:
        cycles = ld_r16_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &H, &L, &PC);
        break;
      case 0x22:
        cycles = ld_HLID_n16(&A, &H, &L, &PC, mem_map, 1);
        break;
      case 0x31:
        cycles = ld_SP_n16((mem_map[PC+2] << 8) + mem_map[PC+1], &SP, &PC);
        break;
      case 0x32:
        cycles = ld_HLID_n16(&A, &H, &L, &PC, mem_map, 0);
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
      case 0xcb: PC++; cycles = 1;
        // Debugging
        printf("Opcode: %x\n", mem_map[PC]); // Print current byte in hex
        switch (mem_map[PC]) {
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
            printf("Unemulated opcode %x", mem_map[PC]);
            return 1;
        } 
        total_cycles = total_cycles + cycles; // Add amount of cycles executed
        // Debugging print statements
        printf("A: %x B: %x C: %x D: %x E: %x F: %x H: %x L: %x\n", A, B, C, D, E, F, H, L);
        printf("Z: %x N: %x H: %x C: %x\n", (F >> 7) & 1, (F >> 6) & 1, (F >> 5) & 1, (F >> 4) & 1);
        printf("TOTAL CYCLES: %d\n", total_cycles); // Print total cycles
        printf("PROGRAM COUNTER: %d\n\n", PC); // Print program counter
      default:
        printf("Unemulated opcode %x", mem_map[PC]);
        return 1;
    }
    total_cycles = total_cycles + cycles; // Add amount of cycles executed
    // Debugging print statements
    printf("A: %x B: %x C: %x D: %x E: %x F: %x H: %x L: %x\n", A, B, C, D, E, F, H, L);
    printf("Z: %x N: %x H: %x C: %x\n", (F >> 7) & 1, (F >> 6) & 1, (F >> 5) & 1, (F >> 4) & 1);
    printf("TOTAL CYCLES: %d\n", total_cycles); // Print total cycles
    printf("PROGRAM COUNTER: %d\n\n", PC); // Print program counter
  }
}