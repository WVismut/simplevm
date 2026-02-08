#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LABELS 32

struct label {
    uint32_t value;
    uint16_t addres;
};

uint32_t hash(const char *str) {
    // have no idea how this works
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash;
}

uint16_t high_endian(uint16_t opcode) {
    // Linux x86_64 uses little-endian byte order
    // But svm uses high-endian
    // So, i made this function
    // it flips left part of opcode with the right
    // example: 12 34 -> 34 12

    uint16_t res = 0x0000;
    res |= (opcode & 0xFF) * 0x100;
    res |= (opcode & 0xFF00) / 0x100;
    return res;
}

bool custom_compare(char *first_operand, char *second_operand) {
    char res[128];
    int i;
    // first operand will be converted to lowercase
    for (i = 0; i < strlen(first_operand); i++) {
        res[i] = tolower(first_operand[i]);
    }

    res[i] = 0;

    if (strcmp(res, second_operand) == 0) {
        return true;
    } else {
        return false;
    }
}

int main(int argc, char *argv[]) {
    char delimiters[] = " \n,;:\t";
    char buffer[128];
    char *token;
    char *endptr;
    short lp = 0; // label pointer

    uint32_t address;
    struct label labels[MAX_LABELS];
    uint16_t opcode;
    uint16_t pc;
    uint8_t general_purpose_vars[2];
    
    FILE *asm_file = fopen(argv[1], "r");
    FILE *target_file = fopen(argv[2], "wb");
    if (asm_file == NULL) {
        printf("Error opening asm file!\n");
        return 1;
    } else if (target_file == NULL) {
        printf("Error opening target file!\n");
        return 1;
    }

    uint8_t header[2] = {0x00, 0x02}; // TEMP
    fwrite(header, 1, sizeof(header), target_file); // TEMP
    pc = 0; // TEMP

    while (true) {
        pc += 2;
        opcode = 0;

        if (fgets(buffer, sizeof(buffer), asm_file) == NULL) {
            fclose(asm_file);
            fclose(target_file);
            return 0;
        }

        token = strtok(buffer, delimiters);

        if (custom_compare(token, "mov")) {

            // parse mov instruction
            // usage: mov x,y
            // x - value to store
            // y - where to use
            // opcode: 20xy

            token = strtok(NULL, delimiters);
            general_purpose_vars[0] = strtoul(token, &endptr, 16);

            token = strtok(NULL, delimiters);
            general_purpose_vars[1] = strtoul(token, &endptr, 16);

            opcode = high_endian(0x2000 | (general_purpose_vars[0] * 0x100) | (general_purpose_vars[1]));
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "end")) {

            // end the program with exit code stored in a reg
            // usage: end x
            // opcode 00 0x

            token = strtok(NULL, delimiters);
            general_purpose_vars[0] = strtoul(token, &endptr, 16);
            opcode = high_endian(general_purpose_vars[0]);
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "add")) {

            // adds together values from 2 regs, stores in the first arg
            // usage: add x, y
            // opcode: xy10

            token = strtok(NULL, delimiters); // x
            general_purpose_vars[0] = strtoul(token, &endptr, 16);
            token = strtok(NULL, delimiters); // y
            general_purpose_vars[1] = strtoul(token, &endptr, 16);
            opcode = high_endian(0x1000 | (general_purpose_vars[0] * 0x10) | (general_purpose_vars[1]));
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "sub")) {

            // subtract value from one register by other
            // usage: sub x, y
            // opcode: 11xy

            token = strtok(NULL, delimiters); // x
            general_purpose_vars[0] = strtoul(token, &endptr, 16);
            token = strtok(NULL, delimiters); // y
            general_purpose_vars[1] = strtoul(token, &endptr, 16);
            opcode = high_endian(0x1100 | (general_purpose_vars[0] * 0x10) | (general_purpose_vars[1]));
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "mul")) {

            // multiplies together values from 2 regs, stores in the first arg
            // usage: mul x, y
            // opcode: 12xy

            token = strtok(NULL, delimiters); // x
            general_purpose_vars[0] = strtoul(token, &endptr, 16);
            token = strtok(NULL, delimiters); // y
            general_purpose_vars[1] = strtoul(token, &endptr, 16);
            opcode = high_endian(0x1200 | (general_purpose_vars[0] * 0x10) | (general_purpose_vars[1]));
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "div")) {

            // adds together values from 2 regs, stores in the first arg
            // usage: add x, y
            // opcode 13xy

            token = strtok(NULL, delimiters); // x
            general_purpose_vars[0] = strtoul(token, &endptr, 16);
            token = strtok(NULL, delimiters); // y
            general_purpose_vars[1] = strtoul(token, &endptr, 16);
            opcode = high_endian(0x1300 | (general_purpose_vars[0] * 0x10) | (general_purpose_vars[1]));
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "rmov")) {

            // copies value from one register to another
            // usage: rmov x, y
            // x - target
            // y - value

            token = strtok(NULL, delimiters); // x
            general_purpose_vars[0] = strtoul(token, &endptr, 16);
            token = strtok(NULL, delimiters); // y
            general_purpose_vars[1] = strtoul(token, &endptr, 16);
            opcode = high_endian(0x7000 | (general_purpose_vars[0] * 0x10) | (general_purpose_vars[1]));
            fwrite(&opcode, sizeof(opcode), 1, target_file);

        } else if (custom_compare(token, "label")) {

            // this thing remembers the current address
            // you can jump to a label using jmp instruction

            if (lp > MAX_LABELS) {
                printf("Failed: too much labels\n");
                return 1;
            }

            token = strtok(NULL, delimiters);
            labels[lp].value = hash(token);
            labels[lp].addres = pc;
            pc -= 2;
            lp++;

        } else {
            printf("Fatal error: no such instruction: \"%s\"\n", token);
            return 1;
        }
    }
}