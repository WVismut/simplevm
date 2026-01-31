#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct VirtualMachine {
    uint8_t ram[8128];
    uint8_t stack[256];

    // vm uses the regs[15] register for division reminder
    // vm uses the regs[14] register for printing strings 
    uint8_t regs[16];
    uint8_t flags;
    uint16_t pc; // program counter
    uint16_t dc; // data counter, vm uses it for printing strings/iterating
    uint8_t sp; // stack pointer
    uint8_t bp; // base 

int main(int argc, char *argv[]) {
    // variables for interptitation
    int medium;
    int div_remainder;
    uint16_t opcode;
    int text_section_start;

    // initialize VMs state
    struct VirtualMachine vm;
    memset(vm.ram, 0, 8128);
    memset(vm.regs, 0, 16);
    vm.pc = 0;

    // get program data
    FILE* file = NULL;
    size_t size;

    // get files size
    file = fopen(argv[1], "rb");
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);
    if (size > 8128) {
        printf("File size is too big\n");
        return 1;
    }

    // load ram
    fread(vm.ram, 1, size, file);

    // get position of code section
    // everything before code section is interpreted as data section
    // in the data section you can store any data, null-terminated strings for example
    opcode = vm.ram[vm.pc] << 8 | vm.ram[vm.pc + 1];

    text_section_start = opcode;
    vm.pc = text_section_start;

    while (1) {
        opcode = vm.ram[vm.pc] << 8 | vm.ram[vm.pc + 1]; // each opcode is 2 bytes just like in CHIP-8
        vm.pc += 2; // increase program counter

        switch (opcode & 0xF000) {
            case 0x0000: // VM state control
                printf("VM shutdown with exit code %d\n", vm.regs[opcode & 0x000F]);
                return vm.regs[opcode & 0x000F];
                break;
            case 0x1000: // math operations
                switch (opcode & 0x0F00) {
                    case 0x0000: // addition
                        /*
                        0x10xy
                        x - first operand & location to store output
                        y - second operand
                        if result is bigger than 0xFF, the overflow flag is set to one
                        */

                        medium = vm.regs[(opcode & 0x00F0) >> 4] + vm.regs[opcode & 0x000F];
                        if (medium > 0xFF) {
                            vm.flags |= 0b1000000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        break;

                    case 0x0100: // subtraction
                        /*
                        0x11xy
                        x - first operand & location to store output
                        y - second operand
                        if result is below 0xFF, the overflow flag is set to one
                        */

                        medium = vm.regs[(opcode & 0x00F0) >> 4] - vm.regs[opcode & 0x000F];
                        if (medium < 0) {
                            vm.flags |= 0b1000000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        break;
                    case 0x0200: // multiplication
                        /*
                        0x12xy
                        x - first operand & location to store output
                        y - second operand
                        if result is bigger than 0xFF, the overflow flag is set to one
                        */

                        medium = vm.regs[(opcode & 0x00F0) >> 4] * vm.regs[opcode & 0x000F];
                        if (medium > 0xFF) {
                            vm.flags |= 0b1000000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        break;
                    case 0x0300: // division
                        /*
                        0x13xy
                        x - first operand & location to store output
                        y - second operand (divider)
                        if divider is equals to zero, 'zero division' flag is set to one
                        */

                        if (vm.regs[opcode & 0x000F] == 0) {
                            vm.flags |= 0b01000000; // zero division flag
                            break;
                        }

                        medium = vm.regs[(opcode & 0x00F0) >> 4] / vm.regs[opcode & 0x000F];
                        div_remainder = vm.regs[(opcode & 0x00F0) >> 4] % vm.regs[opcode & 0x000F];

                        if (div_remainder != 0) {
                            vm.flags |= 0b00100000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        vm.regs[15] = div_remainder;
                        break;
                    case 0x0400: // compare two values
                        /*
                        0x14xy
                        x - first operand
                        y - second operand
                        if x < y, flags |= 00010000, this is 'cmp' flag
                        */

                        medium = vm.regs[(opcode & 0x00F0) >> 4] - vm.regs[opcode & 0x000F];
                        if (medium < 0)
                            vm.flags |= 0b00010000;
                        break;
                }
                break;
            case 0x2000: // mov operation
                /*
                0x2xyy
                x - in which register to store
                yy - value to store
                */
                vm.regs[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
                break;
            case 0x3000: // jumps
                /*
                0x3nnn
                nn - opcode nummber to jump
                */
                if (((vm.flags & 0b00010000) >> 4) == 1) {
                    vm.pc = (opcode & 0x0FFF);
                    vm.flags ^= 0b00010000;
                }
                break;
            case 0x4000: // print a null terminated string
                /*
                0x4nnn
                nnn - addres of first element of null terminated string
                */
                vm.regs[14] = vm.ram[(opcode & 0x0FFF)];
                vm.dc = 1;
                while (vm.regs[14] != 0x00) {
                    printf("%c", vm.regs[14]);
                    vm.regs[14] = vm.ram[(opcode & 0x0FFF) + vm.dc];
                    vm.dc++;
                }
                break;
        }
    }
    
    return 1;
}