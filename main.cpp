#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <array>

using namespace std;

#define OPCODE_COND(COND, VAL) if (opcode == COND) return VAL
#define OPCODE_RANGE(LOWERB, UPPERB, VAL) if (opcode <= LOWERB && opcode >= UPPERB) return VAL

typedef std::list<std::vector<uint8_t>> sstack_t;

void process_opcode(uint8_t *&ip, sstack_t &stack);

const char *opcode_to_str(uint8_t opcode) {

    // Constants

    OPCODE_COND(0, "OP_FALSE");
    OPCODE_RANGE(1, 75, "N/A");
    OPCODE_COND(76, "OP_PUSHDATA1");
    OPCODE_COND(77, "OP_PUSHDATA2");
    OPCODE_COND(78, "OP_PUSHDATA4");
    OPCODE_COND(79, "OP_NEGATE");
    OPCODE_COND(81, "OP_TRUE");
    OPCODE_COND(82, "OP_2");
    OPCODE_COND(83, "OP_3");
    OPCODE_COND(84, "OP_4");
    OPCODE_COND(85, "OP_5");
    OPCODE_COND(86, "OP_6");
    OPCODE_COND(87, "OP_7");
    OPCODE_COND(88, "OP_8");
    OPCODE_COND(89, "OP_9");
    OPCODE_COND(90, "OP_10");
    OPCODE_COND(91, "OP_11");
    OPCODE_COND(92, "OP_12");
    OPCODE_COND(93, "OP_13");
    OPCODE_COND(94, "OP_14");
    OPCODE_COND(95, "OP_15");
    OPCODE_COND(96, "OP_16");

    // Flow control
    OPCODE_COND(97, "OP_NOP");
    OPCODE_COND(99, "OP_IF");
    OPCODE_COND(100, "OP_NOTIF");
    OPCODE_COND(103, "OP_ELSE");
    OPCODE_COND(104, "OP_ENDIF");
    OPCODE_COND(105, "OP_VERIFY");
    OPCODE_COND(106, "OP_RETURN");


    return 0;

}

int print_script(uint8_t *script, size_t script_len) {
    uint8_t *script_start = script;

    while(script < script_start + script_len) {

        const char *opcode_str = opcode_to_str(*script);

        if (opcode_str == NULL) {
            printf("<UNDEF %X> ", *script);
        }
        else {
            printf("%s ", opcode_str);
        }
        script++;
    }
    puts("");
    return 0;
}

int print_stack(sstack_t &stack) {
    for (vector<uint8_t> &item : stack) {
        for (uint8_t &byte : item) {
            printf("%02X", byte);
        }
        puts("");
    }
    return 0;
}

int main() {

    // Load program
    std::string script;
    std::getline(std::cin, script);

    size_t bytecode_len = script.length() / 2;
    uint8_t *bytecode = (uint8_t *) malloc(bytecode_len);

    const char *script_str = script.c_str();
    for (unsigned long i = 0; i < bytecode_len; i++) {
        sscanf(script_str + 2 * i, "%02x", bytecode + i);
    }

    // Print program
    print_script(bytecode, bytecode_len);

    // Execute program

    sstack_t stack;
    uint8_t *ip = bytecode;

    while(ip < bytecode + bytecode_len) {
        process_opcode(ip, stack);
        ip++;
    }


    puts("~~STACKBOTTOM~~");
    print_stack(stack);
    puts("~~STACKTOP~~");
    printf("%lu\n", stack.size());

    free(bytecode);

    return 0;
}

// Words
/// Constants
inline void op_false(sstack_t &stack) {
    stack.emplace_back(vector<uint8_t>(0));
}
inline void op_pushn(uint8_t *&ip, sstack_t &stack) {
    uint_fast8_t bytes_to_push = ip[0];
    stack.emplace_back(vector<uint8_t>(ip + 1, ip + 1 + *ip));
    ip += *ip;
}
inline void op_pushdataN(uint8_t *&ip, sstack_t &stack) {

    uint8_t opcode = *ip;
    int_fast8_t indicator_len;
    uint32_t bytes_to_push;

    switch (opcode) {
        case 76:
            bytes_to_push = ip[1];
            indicator_len = 1;
            break;
        case 77:
            bytes_to_push = *((uint16_t *) ip + 1);
            indicator_len = 2;
            break;
        case 78:
            bytes_to_push = *((uint32_t *) ip + 1);
            indicator_len = 4;
            break;

        default:
            // todo: throw some kind of error. we messed up.
            return;
    }

    stack.emplace_back(vector<uint8_t>(ip + 2, ip + 2 + bytes_to_push));
    ip += bytes_to_push + indicator_len;
}
inline void op_1negate(uint8_t *&ip, sstack_t &stack) {
    stack.emplace_back(vector<uint8_t>() = {(uint8_t) -1 });
}
inline void op_true(uint8_t *&ip, sstack_t &stack) {
    stack.emplace_back(vector<uint8_t>() = {1});
}
inline void op_N(uint8_t *&ip, sstack_t &stack) {

    uint8_t opcode = *ip;
    uint8_t byte_to_push = (uint8_t) (opcode - 80);
    stack.emplace_back(vector<uint8_t>() = {byte_to_push});

}

void process_opcode(uint8_t *&ip, sstack_t &stack) {

    uint8_t opcode = *ip;

    // Constants
    if (opcode == 0) { op_false(stack); return;}
    if (opcode >= 1 && opcode <= 75) { op_pushn(ip, stack); ip+=opcode; return; }
    if (opcode >= 76 && opcode <= 78) { op_pushdataN(ip, stack); return; }
    if (opcode == 79) { op_1negate(ip, stack); return; }
    if (opcode == 80) { op_true(ip, stack); return; }
    if (opcode >= 82 && opcode <= 96) { op_N(ip, stack); return; }

}
