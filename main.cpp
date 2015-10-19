#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <array>
#include <string.h>

using namespace std;

#define OPCODE_COND(COND, VAL) if (opcode == COND) return VAL
#define OPCODE_RANGE(LOWERB, UPPERB, VAL) if (opcode <= LOWERB && opcode >= UPPERB) return VAL

typedef std::list<std::vector<uint8_t>> sstack_t;

void process_opcode(uint8_t *&ip, sstack_t &stack, sstack_t &alt_stack, bool &execute, vector<bool> &conditional_nestting,
                    bool &transaction_is_valid);

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

    while (script < script_start + script_len) {

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
    sstack_t alt_stack;
    uint8_t *ip = bytecode;


    bool execute = true;
    bool transaction_is_valid = true;
    vector<bool> conditional_nesting;

    while (ip < bytecode + bytecode_len) {
        process_opcode(ip, stack, alt_stack, execute, conditional_nesting, transaction_is_valid);
        ip++;
    }
    printf("Transaction is considered %svalid.\n", transaction_is_valid ? "" : "in");
    printf("Stack has %lu element(s).\n", stack.size());

    puts("~~STACKBOTTOM~~");
    print_stack(stack);
    puts("~~STACKTOP~~");
    puts("");
    printf("Alternative stack has %lu element(s).\n", alt_stack.size());

    puts("~~ALTSTACKBOTTOM~~");
    print_stack(alt_stack);
    puts("~~ALTSTACKTOP~~");

    free(bytecode);

    return 0;
}

// Utils

/// Returns true if the vector can be considered true.
inline bool vec_truth(vector<uint8_t> &vec) {

    // An empty vector is considered false.
    if (vec.empty()) {
        return false;
    }

    // To be true the first byte must be other than be 0x00 or 0x80.
    if (vec.front() & 0x7F) {
        return true;
    }

    // Start from the second item and check if all bytes are zero.
    for (auto it = vec.begin() + 1; it != vec.end(); it++) {
        if (*it != 0) { return true; }
    }
    return false;
}

///
inline uint64_t vec_to_long(vector<uint8_t> &vec) {

    if (vec.size() <= 8) {
        // Everything is little endian (for once it makes me happy).
        uint64_t ret = 0;
        memcpy(&ret, &vec[0], vec.size());
        return ret;
    }
    else {
        // Todo: throw something to say: "HAY, WE CANNOT REPRESENT THIS NUMBER NATIVELY! :'("
        return 0;
    }
}


// Words
/// Constants

inline void op_false(sstack_t &stack, bool &execute) {
    if (execute) { stack.emplace_back(vector<uint8_t>(0)); };
}

inline void op_pushn(uint8_t *&ip, sstack_t &stack, bool &execute) {
    uint_fast8_t bytes_to_push = ip[0];
    if (execute) {
        stack.emplace_back(vector<uint8_t>(ip + 1, ip + 1 + *ip));
    }
    ip += *ip;
}

inline void op_pushdataN(uint8_t *&ip, sstack_t &stack, bool &execute) {

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

    if (execute) {
        stack.emplace_back(vector<uint8_t>(ip + 2, ip + 2 + bytes_to_push));
    }

    ip += bytes_to_push + indicator_len;
}

inline void op_1negate(uint8_t *&ip, sstack_t &stack, bool &execute) {
    if (execute) {
        stack.emplace_back(vector<uint8_t>() = {(uint8_t) -1});
    }
}

inline void op_true(uint8_t *&ip, sstack_t &stack, bool &execute) {
    if (execute) {
        stack.emplace_back(vector<uint8_t>() = {1});
    }
}

inline void op_N(uint8_t *&ip, sstack_t &stack, bool &execute) {
    if (execute) {
        uint8_t opcode = *ip;
        uint8_t byte_to_push = (uint8_t) (opcode - 80);
        stack.emplace_back(vector<uint8_t>() = {byte_to_push});
    }
}

/// Flow control

inline void op_if(uint8_t *&ip, sstack_t &stack, bool &execute, vector<bool> &conditional_nesting) {

    if (execute) {

        conditional_nesting.push_back(true);

        bool truth = vec_truth(stack.back());

        if (!truth) {
            execute = false;
        }
        stack.pop_back();
    }
    else {
        // Tell to ignore anything until next OP_ENDIF.
        conditional_nesting.push_back(false);
    }

}

inline void op_notif(uint8_t *&ip, sstack_t &stack, bool &execute, vector<bool> &conditional_nesting) {

    if (execute) {

        conditional_nesting.push_back(true);

        bool truth = vec_truth(stack.back());

        if (truth) {
            execute = false;
        }
        stack.pop_back();
    }
    else {
        // Tell to ignore anything until next OP_ENDIF.
        conditional_nesting.push_back(false);
    }

}

inline void op_else(uint8_t *&ip, sstack_t &stack, bool &execute, vector<bool> &conditional_nesting) {

    bool in_executable_branch = conditional_nesting.back();

    if (in_executable_branch) {
        execute = !execute;
    }
}

inline void op_endif(uint8_t *&ip, sstack_t &stack, bool &execute, vector<bool> &conditional_nesting) {

    bool in_executable_branch = conditional_nesting.back();
    conditional_nesting.pop_back();

    if (in_executable_branch) {
        execute = true;
    }
}

inline void op_verify(sstack_t &stack, bool &execute, bool &transaction_is_valid) {

    if (execute) {

        bool truth = vec_truth(stack.back());

        if (!truth) {
            transaction_is_valid = false;
        }
    }
}

inline void op_return(bool &execute, bool &transaction_is_valid) {

    if (execute) {
        transaction_is_valid = false;
    }
}

/// Stack

inline void op_toaltstack(sstack_t &stack, sstack_t &alt_stack, bool execute) {
    if (execute) {
        alt_stack.emplace_back(std::move(stack.back()));
        stack.pop_back();
    }
}
inline void op_fromaltstack(sstack_t &stack, sstack_t &alt_stack, bool execute) {
    if (execute) {
        stack.emplace_back(std::move(alt_stack.back()));
        alt_stack.pop_back();
    }
}
inline void op_dup(sstack_t &stack, bool execute) {
    if (execute) {
        stack.emplace_back(vector<uint8_t>(stack.back()));
    }
}
inline void op_ifdup(sstack_t &stack, bool execute) {
    if (execute) {
        if (vec_truth(stack.back())) {
            op_dup(stack, execute);
        }
    }
}
inline void op_depth(sstack_t &stack, bool execute) {
    if (execute) {
        stack.emplace_back(vector<uint8_t>() = { stack.size() });
    }
}
inline void op_drop(sstack_t &stack, bool execute) {
    if (execute) {
        stack.pop_back();
    }
}

// Remove second to top stack item.
inline void op_nip(sstack_t &stack, bool execute) {
    if (execute) {
        stack.erase(--stack.end());
    }
}

// Remove second to top stack item.
inline void op_over(sstack_t &stack, bool execute) {
    if (execute) {
        stack.emplace_back(*(--stack.end()));
    }
}

// Copies the second-to-top stack item to the top.
inline void op_pick(sstack_t &stack, bool execute) {
    if (execute) {

        uint64_t nth_back = vec_to_long(stack.back());
        stack.pop_back();

        auto stack_itr = stack.end();
        std::prev(stack_itr, nth_back);

        stack.emplace_back(*stack_itr);
    }
}

inline void helper_op_rotfamily(sstack_t &stack, uint64_t nth) {
    stack.splice(stack.end(), stack, std::prev(stack.end(), nth));
}

// Moves the second-to-top stack item to the top.
inline void op_rot(sstack_t &stack, bool execute) {
    if (execute) {
        helper_op_rotfamily(stack, 3);
    }
}

// Moves the nth-to-top stack item to the top. Nth is the number on the TOS.
inline void op_roll(sstack_t &stack, bool execute) {
    if (execute) {

        uint64_t nth_back = vec_to_long(stack.back());
        stack.pop_back();
        helper_op_rotfamily(stack, nth_back);
    }
}

// Moves the second-to-top stack item to the top.
inline void op_swap(sstack_t &stack, bool execute) {
    if (execute) {
        helper_op_rotfamily(stack, 2);
    }
}

inline void op_tuck(sstack_t &stack, bool execute) {
    if (execute) {
        stack.emplace(std::prev(stack.end(), 2), vector<uint8_t>() = stack.back());
    }
}

inline void op_2drop(sstack_t &stack, bool execute) {
    if (execute) {
        stack.pop_back();
        stack.pop_back();
    }
}

inline void helper_op_dup_family(sstack_t &stack, uint64_t nth) {
    stack.insert(stack.end(), std::prev(stack.end(), nth), stack.end());

}

inline void op_2dup(sstack_t &stack, bool execute) {
    if (execute) {
        helper_op_dup_family(stack, 2);
    }
}

inline void op_3dup(sstack_t &stack, bool execute) {
    if (execute) {
        helper_op_dup_family(stack, 3);
    }
}


// Copies the pair of items two spaces back in the stack to the front.
inline void op_2over(sstack_t &stack, bool execute) {
    if (execute) {
        stack.insert(stack.end(), std::prev(stack.end(), 4), std::prev(stack.end(), 2));
    }
}

// Swaps the top two pairs of items.
inline void op_2swap(sstack_t &stack, bool execute) {
    if (execute) {
        stack.splice(stack.end(), stack, std::prev(stack.end(), 4), std::prev(stack.end(), 2));
    }
}

// Swaps the top two pairs of items.
inline void op_2rot(sstack_t &stack, bool execute) {
    if (execute) {
        stack.splice(stack.end(), stack, std::prev(stack.end(), 6), std::prev(stack.end(), 4));
    }
}

void process_opcode(uint8_t *&ip, sstack_t &stack, sstack_t &alt_stack, bool &execute, vector<bool> &conditional_nesting,
                    bool &transaction_is_valid) {

    uint8_t opcode = *ip;

    // Constants
    if (opcode == 0) {
        op_false(stack, execute);
        return;
    }
    if (opcode >= 1 && opcode <= 75) {
        op_pushn(ip, stack, execute);
        ip += opcode;
        return;
    }
    if (opcode >= 76 && opcode <= 78) {
        op_pushdataN(ip, stack, execute);
        return;
    }
    if (opcode == 79) {
        op_1negate(ip, stack, execute);
        return;
    }
    if (opcode == 81) {
        op_true(ip, stack, execute);
        return;
    }
    if (opcode >= 82 && opcode <= 96) {
        op_N(ip, stack, execute);
        return;
    }

    // Flow control
    if (opcode == 97) { return; }
    if (opcode == 99) {
        op_if(ip, stack, execute, conditional_nesting);
        return;
    }
    if (opcode == 100) {
        op_notif(ip, stack, execute, conditional_nesting);
        return;
    }
    if (opcode == 103) {
        op_else(ip, stack, execute, conditional_nesting);
        return;
    }
    if (opcode == 104) {
        op_endif(ip, stack, execute, conditional_nesting);
        return;
    }
    if (opcode == 105) {
        op_verify(stack, execute, transaction_is_valid);
        return;
    }
    if (opcode == 106) {
        op_return(execute, transaction_is_valid);
        return;
    }

    // Stack
    if (opcode == 107) {
        op_toaltstack(stack, alt_stack, execute);
        return;
    }
    if (opcode == 108) {
        op_fromaltstack(stack, alt_stack, execute);
        return;
    }
    if (opcode == 115) {
        op_ifdup(stack, execute);
        return;
    }
    if (opcode == 116) {
        op_depth(stack, execute);
        return;
    }
    if (opcode == 117) {
        op_drop(stack, execute);
        return;
    }
    if (opcode == 118) {
        op_drop(stack, execute);
        return;
    }
    if (opcode == 119) {
        op_nip(stack, execute);
        return;
    }
    if (opcode == 120) {
        op_over(stack, execute);
        return;
    }
    if (opcode == 121) {
        op_pick(stack, execute);
        return;
    }
    if (opcode == 122) {
        op_roll(stack, execute);
        return;
    }
    if (opcode == 123) {
        op_rot(stack, execute);
        return;
    }
    if (opcode == 124) {
        op_swap(stack, execute);
        return;
    }
    if (opcode == 125) {
        op_tuck(stack, execute);
        return;
    }
    if (opcode == 109) {
        op_2drop(stack, execute);
        return;
    }
    if (opcode == 110) {
        op_2dup(stack, execute);
        return;
    }
    if (opcode == 111) {
        op_3dup(stack, execute);
        return;
    }
    if (opcode == 112) {
        op_2over(stack, execute);
        return;
    }
    if (opcode == 113) {
        op_2rot(stack, execute);
        return;
    }
    if (opcode == 114) {
        op_2swap(stack, execute);
        return;
    }
}
