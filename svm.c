#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define RESET   "\033[0m"
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"

struct VirtualMachine {
    uint8_t ram[4096];

    // vm uses the regs[15] register for division reminder
    // vm uses the regs[14] register for printing strings 
    uint8_t regs[16];
    uint16_t addres_call_reg; // register used for adsresses
    uint8_t flags;
    uint16_t pc; // program counter
    uint16_t dc; // data counter, vm uses it for printing strings/iterating
    uint16_t sp; // stack pointer
};

int main(int argc, char *argv[]) {
    // sys variables
    char filename[128];
    bool debug;

    // variables for interptitation
    int medium;
    int div_remainder;
    uint16_t opcode;
    int text_section_start;

    // initialize VMs state
    struct VirtualMachine vm;
    memset(vm.ram, 0, 4096);
    memset(vm.regs, 0, 16);
    vm.pc = 0;

    // fetch arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-n") == 0) | (strcmp(argv[i], "--name") == 0))
            if (argc < i + 2) {
                printf("Fatal error: no filename provided\n");
                exit(1);
            } else {
                strcpy(filename, argv[i + 1]);
                i++;
            }
        else if ((strcmp(argv[i], "-d") == 0) | (strcmp(argv[i], "--debug") == 0))
            debug = true;
        else {
            printf("Invalid argument: %s\n"
                   "Here are valid arguments:\n"
                   "-n [name]      --name [name]       Provide a name for VM\n"
                   "-d             --debug             Turn on debug mode\n", argv[i]);
            return 1;
        }
    }

    // get program data
    FILE* file = NULL;
    size_t size;

    // get files size
    file = fopen(filename, "rb");
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

    text_section_start = opcode; // this variable will be deleted in the future updates to make VM more separated
                                 // from variables, that are not in the VM directly and that are not "realistic"
                                 // to have in a such VM
    vm.pc = text_section_start;
    vm.sp = 4095;

    while (1) {
        opcode = vm.ram[vm.pc] << 8 | vm.ram[vm.pc + 1]; // each opcode is 2 bytes just like in CHIP-8
        vm.pc += 2; // increase program counter

        if (debug)
            printf(CYAN "Opcode: 0x%x\n" RESET, opcode);
        

        switch (opcode & 0xF000) {
            case 0x0000: // VM state control
                switch (opcode & 0x0F00) {
                    case 0x0000: // Shutdown VM

                        // 0x000n
                        // n - register to use
                        // value from register will be returned

                        if (debug)
                            printf(MAGENTA "VM shutdown with exit code %d\n" RESET, vm.regs[opcode & 0x000F]);
                        return vm.regs[opcode & 0x000F];
                    case 0x0100: // Return from a function
                        vm.pc = vm.addres_call_reg;
                        break;
                }
               
            case 0x1000: // math operations
                switch (opcode & 0x0F00) {
                    case 0x0000: // addition

                        // 0x10xy
                        // x - first operand & location to store output
                        // y - second operand
                        // if result is bigger than 0xFF, the overflow flag is set to one

                        medium = vm.regs[(opcode & 0x00F0) >> 4] + vm.regs[opcode & 0x000F];
                        if (medium > 0xFF) {
                            vm.flags |= 0b1000000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        break;

                    case 0x0100: // subtraction

                        // 0x11xy
                        // x - first operand & location to store output
                        // y - second operand
                        // if result is below 0xFF, the overflow flag is set to one

                        medium = vm.regs[(opcode & 0x00F0) >> 4] - vm.regs[opcode & 0x000F];
                        if (medium < 0) {
                            vm.flags |= 0b1000000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        break;
                    case 0x0200: // multiplication

                        // 0x12xy
                        // x - first operand & location to store output
                        // y - second operand
                        // if result is bigger than 0xFF, the overflow flag is set to one


                        medium = vm.regs[(opcode & 0x00F0) >> 4] * vm.regs[opcode & 0x000F];
                        if (medium > 0xFF) {
                            vm.flags |= 0b1000000;
                        }

                        vm.regs[(opcode & 0x00F0) >> 4] = medium & 0xFF;
                        break;
                    case 0x0300: // division

                        // 0x13xy
                        // x - first operand & location to store output
                        // y - second operand (divider)
                        // if divider is equals to zero, 'zero division' flag is set to one

                        if (vm.regs[opcode & 0x000F] == 0) {
                            vm.flags |= 0b01000000; // zero division flag
                            if (debug)
                                printf(YELLOW "Warning: division by zero" RESET);
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

                        // 0x14xy
                        // x - first operand
                        // y - second operand
                        // if x < y, flags |= 00010000, this is 'cmp' flag

                        medium = vm.regs[(opcode & 0x00F0) >> 4] - vm.regs[opcode & 0x000F];
                        if (medium < 0)
                            vm.flags |= 0b00010000;
                        break;
                }
                break;
            case 0x2000: // mov operation

                // 0x2xyy
                // x - in which register to store
                // yy - value to store

                vm.regs[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
                break;
            case 0x3000: // jumps if cmp flag = 1

                // 0x3nnn
                // nn - opcode nummber to jump

                if (((vm.flags & 0b00010000) >> 4) == 1) {
                    vm.pc = (opcode & 0x0FFF);
                    vm.flags ^= 0b00010000;
                }
                break;

            case 0x4000: // print a null terminated string

                // 0x4nnn
                // nnn - addres of first element of null terminated string

                vm.regs[14] = vm.ram[(opcode & 0x0FFF)];
                vm.dc = 1;
                while (vm.regs[14] != 0x00) {
                    printf("%c", vm.regs[14]);
                    vm.regs[14] = vm.ram[(opcode & 0x0FFF) + vm.dc];
                    vm.dc++;
                }
                break;

            case 0x5000: // call a function
                vm.addres_call_reg = vm.pc;
                if (debug)
                    printf("Return address: 0x%x\n", vm.addres_call_reg);
                vm.pc = opcode & 0x0FFF;
                break;
            
            case 0x6000: // stack operations
                switch (opcode & 0x0F00) {
                    case 0x0000: // push to stack
                        // 0x600x
                        // x - reg to use

                        if ((vm.sp < size) & debug) {
                            printf(RED "Critical: stack overflow\n" RESET);
                            return 1;
                        }

                        vm.ram[vm.sp] = vm.regs[opcode & 0x000F];
                        vm.sp--;
                        break;

                    case 0x0100: // pop from stack to reg
                        // 0x610x
                        // x - reg to use

                        if ((vm.sp >= 4095) & debug) {
                            printf(RED "Critical: there is nothing on stack\n" RESET);
                            return 1;
                        }

                        vm.regs[opcode & 0x000F] = vm.ram[vm.sp + 1];
                        vm.sp++;
                        break;
                }
                break;
            
            case 0x7000: // rmov
                // copies value from one register to another
                // 0x70xy
                // x - where to copy
                // y - what to copy
                vm.regs[opcode & 0x00F0] = vm.regs[opcode & 0x000F];
                break;

            case 0x8000: // jump
                // just a jump
                // 0x8nnn
                // nnn - address
                vm.pc = opcode & 0x0FFF;
                break;
            
            case 0x9000: // logic instructions

                // 0x9abc
                // a - instruction
                // b - first operand & where to store
                // c - second operand

                switch (opcode & 0x0F00) {
                    case 0x0000: // or
                        vm.regs[opcode & 0x00F0] |= vm.regs[opcode & 0x000F];
                        break;

                    case 0x0100: // and
                        vm.regs[opcode & 0x00F0] &= vm.regs[opcode & 0x000F];
                        break;
                    
                    case 0x0200: // xor
                        vm.regs[opcode & 0x00F0] ^= vm.regs[opcode & 0x000F];
                        break;

                    case 0x0300:
                        // this instruction takes only one argument & ignores the c argument
                        // bits of b register will be inversed
                        vm.regs[opcode & 0x00F0] = !vm.regs[opcode & 0x00F0];
                        break;
                }
                
            default:
                if (debug)
                    printf(RED "Critical: no such opcode: 0x%x\n" RESET, opcode);
                return 1;
        }
    }
    
    return 1;
}