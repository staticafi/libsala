#ifndef SALA_PROGRAM_HPP_INCLUDED
#   define SALA_PROGRAM_HPP_INCLUDED

#   include <cstdint>
#   include <string>
#   include <vector>
#   include <unordered_map>
#   include <limits>

namespace sala {


struct SourceBackMapping;
struct Constant;
struct Variable;
struct Instruction;
struct BasicBlock;
struct Function;
struct Program;


struct SourceBackMapping
{
    std::uint32_t line{ 0U };
    std::uint32_t column{ 0U };
};


struct Constant
{
    Program* program() const { return program_; }
    std::uint32_t index() const { return index_; }
    std::size_t num_bytes() const { return bytes().size(); }
    std::vector<std::uint8_t> const& bytes() const { return bytes_; }

    void set_program(Program* const program) { program_ = program; }
    void set_index(std::uint32_t const index) { index_ = index; }
    void push_back_byte(std::uint8_t const byte) { bytes_.push_back(byte); }
private:
    Program* program_{ nullptr };
    std::uint32_t index_{};
    std::vector<std::uint8_t> bytes_{};
};


struct Variable
{
    enum struct Region
    {
        STATIC,
        STACK
    };

    Program* program() const { return program_; }
    std::uint32_t function_index() const { return function_index_; }
    std::uint32_t index() const { return index_; }
    Region region() const { return region_; }
    std::size_t num_bytes() const { return num_bytes_; }
    bool is_external() const { return is_external_; }
    SourceBackMapping const& source_back_mapping() const { return back_mapping_; }

    void set_program(Program* const program) { program_ = program; }
    void set_function_index(std::uint32_t const index) { function_index_ = index; }
    void set_index(std::uint32_t const index) { index_ = index; }
    void set_region(Region region) { region_ = region; }
    void set_num_bytes(std::size_t const num_bytes) { num_bytes_ = num_bytes; }
    void set_external(bool const state) { is_external_ = state; }
    SourceBackMapping& source_back_mapping() { return back_mapping_; }
private:
    Program* program_{ nullptr };
    std::uint32_t function_index_{ 0U };
    std::uint32_t index_{};
    Region region_{ Region::STATIC };
    std::size_t num_bytes_{ 0U };
    bool is_external_{ false };
    SourceBackMapping back_mapping_{};
};


struct Instruction
{
    enum struct Opcode
    {
        // In the descriptions of individual instructions below we use the following notations:
        // - Instruction modifiers:
        //      n - NONE
        //      s - SIGNED
        //      u - UNSIGNED
        //      f - FLOATING (where no operand can be (quiet) NAN)
        //      w - FLOATING_UNORDERED (where any operand can be (quiet) NAN)
        //      i - either s or u
        //      g - either u or f or w
        //      x - represents any of above, except n
        // - Instruction operand descriptors:
        //      s - static variable
        //      l - local variable
        //      p - parameter
        //      c - constant
        //      f - fuction
        //      v - any of s, l, p
        //      x - represents any of above
        //   We write a number N behind the descriptor representing the index
        //   of the variable/constant/function. For example:
        //      sN - static variable at the index N
        //      pN - function parameter at the index N
        //      lN - local variable at the index N
        //      cN - constant at the index N
        //      fN - fuction at the index N
        //      vN - either sN, pN, or lN
        //      rN - either vN or fN
        //      nN - either vN or cN
        //      xN - any of the preceding
        //   We use brackets [] to indicate and optional argument, e.g. [cN]
        //   says that the instruction may have a constant as an argument.

        // __INVALID__ n
        // Indicates that we failed to compile the corresponding LLVM instruction.
        __INVALID__,

        // NOP n
        // Does nothing.
        NOP,

        // HALT n
        // Represents error/unreachable program location. Execution of the instruction
        // is a fatal error (because the execution reached an erroneous state) and
        // the execution should halt immediately. 
        HALT,

        // ADDRESS n vN rM
        // Corresponds to a C expression: vN = &rM
        // Copies the address of rM to bytes in vN.
        // Assumptions: vN.num_bytes() == sizeof(void*).
        ADDRESS,

        // LOAD n vN vM
        // Corresponds to a C expression: vN = *vM
        // Copies vN.num_bytes() bytes at the address
        // stored in bytes in vM to bytes in vN.
        // Assumptions: vM.num_bytes() == sizeof(void*).
        LOAD,

        // STORE n vN vM
        // Corresponds to a C expression: *vN = vM
        // Copies vM.num_bytes() bytes in vM to bytes 
        // at the address stored in vN.
        // Assumptions: vN.num_bytes() == sizeof(void*).
        STORE,

        // COPY n vN xM
        // Corresponds to a C expression: vN = xM
        // Copies bytes in vM to bytes in vN.
        // Assumptions: vN.num_bytes() == xM.num_bytes().
        COPY,

        // MEMCPY n vN vM nH
        // Corresponds to a C expression: memcpy(vN, vM, nH)
        // Let 'm' be the integer of type size_t stored in
        // bytes of nH, 's' be the address (pointer) stored
        // in bytes of vM, and 'd' be the address (pointer)
        // stored in bytes of vN. The instruction copies m bytes
        // at addresses [s,s+m) to bytes at addresses [d,d+m).
        // Assumptions: vN.num_bytes() == sizeof(void*).
        //              The memory regions [s,s+m) and [d,d+m) may NOT overlap.
        MEMCPY,

        // MEMMOVE n vN vM nH
        // Corresponds to a C expression: memcpy(vN, vM, nH)
        // Let 'm' be the integer of type size_t stored in
        // bytes of nH, 's' be the address (pointer) stored
        // in bytes of vM, and 'd' be the address (pointer)
        // stored in bytes of vN. The instruction copies m bytes
        // at addresses [s,s+m) to bytes at addresses [d,d+m).
        // The memory regions [s,s+m) and [d,d+m) MAY overlap.
        // NOTE: MEMMOVE differs from MEMCPY only in the fact
        //       that the memory regions may overlap.
        // Assumptions: vN.num_bytes() == sizeof(void*).
        MEMMOVE,

        // MEMSET n vN nM nH
        // Corresponds to a C expression: memset(vN, nM, nH)
        // Let 'v' be the integer of type std::uint8_t stored in
        // the byte of nM, 'n' be the unsigned integer stored in
        // bytes of nH, and 'p' be the address (pointer) stored
        // in bytes of vN. The instruction sets n bytes
        // at addresses [p,p+n) to the value 'v'.
        // Assumptions: vN.num_bytes() == sizeof(void*), nM.num_bytes() == 1.
        MEMSET,

        // MOVEPTR n vN vM nH cG
        // Corresponds to a C expression: void* vN = (void*)((char*)vM + (nH * cG))
        // Let 'n' be the integer stored in bytes of nH,
        // 'm' be the integer stored in bytes of cG, and
        // 'p' be the address (pointer) stored in bytes of vM.
        // The instruction computes the address (pointer)
        // p + (n * m) and stores it to bytes in vN.
        // Assumptions: vN.num_bytes() == vM.num_bytes() == sizeof(void*),
        //              m > 0.
        // NOTE: The number m represents 'sizeof' of an element
        //       and the number n the count of elements to 'move'
        //       the pointer vM over.
        MOVEPTR,

        // ALLOCA n vN nM cH
        // Corresponds to a C code:
        //     case: nM > 1                            case: nM == 1
        //     ----------------------------------------------------------
        //     void* vN;                               void* vN;
        //     T a[nM];                                T a;
        //     vN = (void*)&a[0];                      vN = (void*)&a;
        // Let 'n' be the integer stored in bytes of nM and 'm' be the
        // integer stored in bytes of cH (which must be equal to sizeof(T)).
        // The instruction allocates n * m bytes on the stack and stores
        // the address (pointer) to the allocated memory to bytes in vN.
        // Assumptions:
        //     vN.num_bytes() == sizeof(void*),
        //     n >= 0, m > 0,
        //     and each execution of ALLOCA instruction must be enclosed by
        //         executions of STACKSAVE and STACKRESTORE instructions:
        //             ...STACKSAVE...ALLOCA...ALLOCA...STACKRESTORE...    <-- OK: ALLOCAs are enclosed.
        //             ...STACKSAVE...STACKRESTORE...ALLOCA...             <-- WRONG: ALLOCA is NOT enclosed.
        ALLOCA,

        // STACKSAVE n lN
        // Saves the current stack pointer to bytes of the local variable lN.
        // Assumptions:
        //     lN.num_bytes() == sizeof(void*).
        //     The variable lN must be available since the start of the function. 
        //         So it may not be allocated by ALLOCA instruction.
        //     An execution of each STACKSAVE instruction must be followed by
        //         the corresponding execution of STACKRESTORE instruction,
        //         just like brackets work. Moreover, the corresponding
        //         STACKRESTORE instruction obtains the saved stack pointer.
        //         Examples:
        //             ...STACKSAVE lN...STACKRESTORE lN...                                    <-- OK
        //             ...STACKSAVE lN...STACKSAVE lM...STACKRESTORE lM...STACKRESTORE lN...   <-- OK
        //             ...STACKSAVE lN...STACKSAVE lM...STACKRESTORE lN...STACKRESTORE lM...   <-- WRONG!
        STACKSAVE,

        // STACKRESTORE n lN
        // Restores the stack pointer to the address stored in bytes of the
        // local variable lN.
        // Assumptions:
        //     lN.num_bytes() == sizeof(void*).
        //     The address in the variable lN must be exactly the one saved by the
        //         corrsponding STACKSAVE instruction (the variable can be different,
        //         but the value (the address) in it must be the one saved by STACKSAVE).
        //     An execution of each STACKRESTORE instruction must be preceeded by
        //         the corresponding execution of STACKSAVE instruction,
        //         just like brackets work. Moreover, the corresponding
        //         Examples:
        //             ...STACKSAVE lN...STACKRESTORE lN...                                    <-- OK
        //             ...STACKSAVE lN...COPY lM lN...STACKRESTORE lM...                       <-- OK
        //             ...STACKSAVE lN...STACKSAVE lM...STACKRESTORE lM...STACKRESTORE lN...   <-- OK
        //             ...STACKSAVE lN...STACKSAVE lM...STACKRESTORE lN...STACKRESTORE lM...   <-- WRONG!
        STACKRESTORE,

        // MALLOC n vN nM
        // Let m by an unsigned integer number in bytes of nM. The instruction
        // allocates m bytes in the program heap and stores the address of the
        // first allocated byte into bytes of vN. However, on failure, no byte
        // is allocated and the NULL pointer is written to bytes of vN.
        // Assumptions: m > 0, vN.num_bytes() == sizeof(void*).
        MALLOC,

        // FREE n vN
        // Let p by the address (pointer) stored in bytes of vN. The instruction
        // frees (releases) the sequence of bytes, where p points to the first
        // byte of the sequence.
        // Assumptions: The address p must be exactly the address returned by some preceding
        //              MALLOC instruction and the current instruction is the first FREE
        //              instruction executed for that address since that MALLOC.
        FREE,

        // ADD x vN nM nH
        // Corresponds to a C expression: vN = nM + nH
        // Adds numbers stored in bytes in nM and nH and
        // stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        ADD,

        // SUB x vN nM nH
        // Corresponds to a C expression: vN = nM - nH
        // Subtracts numbers stored in bytes in nM and nH and
        // stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        SUB,

        // MUL x vN nM nH
        // Corresponds to a C expression: vN = nM * nH
        // Multiplies numbers stored in bytes in nM and nH and
        // stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        MUL,

        // DIV x vN nM nH
        // Corresponds to a C expression: vN = nM / nH
        // Divides numbers stored in bytes in nM and nH and
        // stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        DIV,

        // REM i vN nM nH
        // Corresponds to a C expression: vN = nM % nH
        // Computes the remainder of integers stored in bytes in nM and nH and
        // stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        REM,

        // AND n vN nM nH
        // Corresponds to a C expression: vN = nM & nH
        // Applies the bit-wise AND operation on numbers stored in bytes in
        // nM and nH and stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        AND,

        // OR n vN nM nH
        // Corresponds to a C expression: vN = nM | nH
        // Applies the bit-wise OR operation on numbers stored in bytes in
        // nM and nH and stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        OR,

        // XOR n vN nM nH
        // Corresponds to a C expression: vN = nM ^ nH
        // Applies the bit-wise XOR operation on numbers stored in bytes in
        // nM and nH and stores the result to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        // NOTE: Bit-wise NOT operation is implemented using XOR with the
        //       other argument being -1. 
        XOR,

        // SHL n vN nM nH
        // Corresponds to a C expression: vN = nM << nH
        // Shifts bits of the integer number stored in bytes in nM, where
        // the count of the shifted bits is given by the unsigned integer
        // number stored in bytes of nH, and stores the result to bytes in vN.
        // The shifted number accepts zeros from the right.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        //              The integer stored in bytes of nH is always treated as unsigned. 
        SHL,

        // SHR i vN nM nH
        // Corresponds to a C expression: vN = nM >> nH
        // Shifts bits of the integer number stored in bytes in nM, where
        // the count of the shifted bits is given by the unsigned integer
        // number stored in bytes of nH, and stores the result to bytes in vN.
        // If the modifier i is u, then the shifted number accepts zeros from
        // the left (i.e., this is the logical shift). Otherwise (i.e., i is s),
        // the shifted number accepts zeros or ones from the left, depending
        // on the value of the most significant bit in the number in nM (i.e.,
        // this is the arithmetic shift).
        // NOTE: When i is u, then nM is treated as unsigned integer; otherwise
        //       nM is treated as signed integer. So, we have:
        //           vN = (unsigned)nM >> (unsigned)nH // The modifier i is u.
        //           vN = (signed)nM >> (unsigned)nH // The modifier i is s.
        // Assumptions: vN.num_bytes() == nM.num_bytes() == nH.num_bytes().
        //              The integer stored in bytes of nH is always treated as unsigned. 
        SHR,

        // NEG f vN nM
        // Corresponds to a C expression: vN = -nM
        // Copies bytes in nM with the sign bit flipped to bytes in vN.
        // Assumptions: vN.num_bytes() == nM.num_bytes().
        //              Both vM and nM are assumed to be of float/double type.
        // NOTE: For integers the negations is expressed by SUB: vN = 0 - nM
        NEG,

        // EXTEND x vN nM
        // Corresponds to a C expression: T vN = (T)nM // nM has a type S
        //     where S is an integer type if and only if T is an integer type.
        //     Examples: S=short,T=int; S=unsigned short,T=int; S=float,T=double;
        // The value of the source type S stored in bytes in nM is extended to the
        // value of vN.num_bytes() bytes (which is the size of the target type T),
        // and the extended value is stored to bytes in vN.
        // The instruction thus performs casting between two signed/unsigned integer
        // types or between two floating point types. But never between an integer
        // and a floating point type. The descriptor x determines the type of type
        // extension: signed/unsigned/floating.
        // Assumptions: vN.num_bytes() > nM.num_bytes().
        EXTEND,

        // TRUNCATE g vN nM
        // Corresponds to a C expression: T vN = (T)nM // nM has a type S
        //     where S is an integer type if and only if T is an integer type.
        //     Examples: S=int,T=short; S=unsigned int,T=short; S=double,T=float;
        // The value of the source type S stored in bytes in nM is truncated to the
        // value of vN.num_bytes() bytes (which is the size of the target type T),
        // and the truncated value is stored to bytes in vN.
        // The instruction thus performs casting between two signed/unsigned integer
        // types or between two floating point types. But never between an integer
        // and a floating point type. The descriptor g determines the type of type
        // extension: unsigned/floating. (we do NOT have 'signed' truncate).
        // Assumptions: vN.num_bytes() < nM.num_bytes().
        TRUNCATE,

        // F2I i vN nM
        // Corresponds to a C expression: T vN = (T)nM // nM has a type S
        //     where S is a floating point type and T is an integer type.
        //     Examples: S=float,T=short; S=double,T=int; S=float,T=unsigned int;
        // The value of the source floating point type S stored in bytes in nM is
        // casted to the value of the integer type T of vN.num_bytes() bytes (where
        // signed/unsigned is determined by the descriptor x), and the casted
        // value is then stored to bytes in vN.
        // The instruction thus performs casting from a floating point value to a 
        // signed/unsigned integer (based on the descriptor x).
        F2I,

        // I2F i vN nM
        // Corresponds to a C expression: T vN = (T)nM // nM has a type S
        //     where S is an integer type and T is a floating point type.
        //     Examples: S=short,T=float; S=int,T=double; S=unsigned int,T=float;
        // The value of the source integer type S stored in bytes in nM (where
        // signed/unsigned is determined by the descriptor x) is casted to the
        // value of the floating point type T of vN.num_bytes() bytes, and the
        // casted value is then stored to bytes in vN.
        // The instruction thus performs casting from a floating point value to a 
        // signed/unsigned integer (based on the descriptor x).
        I2F,

        // P2I u vN vM
        // Corresponds to a C expression: unsigned int vN = (unsigned int)vM
        //     where unsigned int* is the type of vM.
        // The value of the pointer type stored in bytes of nM is
        // casted to a value of the unsigned integer type and the casted
        // value is then stored to bytes of vN.
        P2I,

        // I2P u vN vM
        // Corresponds to a C expression: unsigned int* vN = (unsigned int*)vM
        //     where unsigned int is the type of vM.
        // The value of the unsigned integer type stored in bytes of nM is
        // casted to a value of a pointer type and the casted
        // value is then stored to bytes of vN.
        I2P,

        // LESS x vN nM nH
        // Corresponds to a C expression: vN = nM < nH
        // Compares numbers, using the operator <, stored in bytes in nM and nH and
        // then stores the result (either 0 or 1) to the single byte in vN.
        // Depending on the modifier x, the operator < performs either signed, unsigned,
        // or floating point comparison.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        LESS,

        // LESS_EQUAL x vN nM nH
        // Corresponds to a C expression: vN = nM <= nH
        // Compares numbers, using the operator <=, stored in bytes in nM and nH and
        // then stores the result (either 0 or 1) to the single byte in vN.
        // Depending on the modifier x, the operator <= performs either signed, unsigned,
        // or floating point comparison.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        LESS_EQUAL,

        // GREATER x vN nM nH
        // Corresponds to a C expression: vN = nM > nH
        // Compares numbers, using the operator >, stored in bytes in nM and nH and
        // then stores the result (either 0 or 1) to the single byte in vN.
        // Depending on the modifier x, the operator > performs either signed, unsigned,
        // or floating point comparison.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        GREATER,

        // GREATER_EQUAL x vN nM nH
        // Corresponds to a C expression: vN = nM >= nH
        // Compares numbers, using the operator >=, stored in bytes in nM and nH and
        // then stores the result (either 0 or 1) to the single byte in vN.
        // Depending on the modifier x, the operator >= performs either signed, unsigned,
        // or floating point comparison.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        GREATER_EQUAL,

        // EQUAL g vN nM nH
        // Corresponds to a C expression: vN = nM == nH
        // Compares numbers, using the operator ==, stored in bytes in nM and nH and
        // then stores the result (either 0 or 1) to the single byte in vN.
        // Depending on the modifier g, the operator == performs either unsigned
        // or floating point comparison.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        EQUAL,

        // UNEQUAL g vN nM nH
        // Corresponds to a C expression: vN = nM != nH
        // Compares numbers, using the operator !=, stored in bytes in nM and nH and
        // then stores the result (either 0 or 1) to the single byte in vN.
        // Depending on the modifier g, the operator != performs either unsigned
        // or floating point comparison.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        UNEQUAL,

        // ISNAN w vN nM nH
        // Corresponds to a C expression: vN = isnan(nM) || isnan(nH)
        // Checks whether any of two floating point numbers stored in nM and nH
        // is (quiet) NAN. The result (0 or 1) is then stored to vN.
        // Assumptions: vN.num_bytes() == 1,
        //              vM.num_bytes() == nH.num_bytes().
        ISNAN,

        // JUMP n
        // The last instruction of a basic block with exactly 1 successor.
        // Transfers the execution to the first instruction of the successor basic block.
        JUMP,

        // BRANCH n vN
        // The last instruction of a basic block with exactly 2 successors.
        // Transfers the execution to the first instruction of one of the two successor
        // basic blocks. The choice is made according to the value in the byte vN.bytes().front().
        // If the byte is 0, then the execution is transferred to the first successor
        // basic bock (i.e., to vN.successors().at(0)), otherwise it is transferred
        // to the second successor basic block (i.e., to vN.successors().at(1))
        // Assumptions: vN.num_bytes() == 1,
        //              vN.bytes().front() is either 0 or 1.
        BRANCH,

        // CALL n rN [p0 xH1 ... xHm]
        // Corresponds to some of these C expressions:
        //     rN([p0, xH1, ..., xHm])    // rN is actually fN
        //     (*rN)([p0, xH1, ..., xHm]) // rN is actually vN holding the pointer to the called function
        // rN is either the called function index (i.e., rN is actually fN) or
        // a pointer to the called function (i.e., rN is actually vN); p0 is the optional
        // "return-value-pointer" parameter containing the address where to write the return
        // value (p0 is present only for functions returning a value); xH1 ... xHm are
        // values to be passed to the called function as "regular" parameters.
        // NOTE: For a function with ellipsis parameter '...' the number of parameters
        // of the call instruction can be greater than the number of formal parameters
        // of the called function.
        CALL,

        // RET n
        // The last instruction of a basic block with exactly 0 successors.
        // Implements the return from the currently executed function to its callee.
        // Returning from the entry function (main) yields program termination.
        // Assumptions: The basic bock of this instruction does not have any successor block.
        RET,

        // VA_START n vN
        // Let 'p' be the address (pointer) stored in bytes of vN.
        // Initializes the list of variable function parameters (packed into the '...' C parameter)
        // and stores to the address (pointer) to the first variable function parameter
        // to the bytes pointed by 'p'.
        // NOTE: The instruction does NOT change the bytes of vN (unless 'p' points actually to vN).
        // NOTE: The layout of variable function parameters on the stack is platform/compiler
        //       dependent. However, the alignment of variable function parameters on the stack
        //       is typically 8 bytes for all primitive types. 
        // Assumptions: vN.num_bytes() == sizeof(void*).
        //              There must follow an execution of the corresponding VA_END instruction.
        VA_START,

        // VA_END n vN
        // Let 'p' be the address (pointer) stored in bytes of vN.
        // Releases the list of variable function parameters (packed into the '...' C parameter).
        // The address (pointer) to SOME (but typically the last) variable function parameter
        // must be stored in bytes pointed by 'p'.
        // NOTE: The pointer to the list of parameters to release is NOT stores in the bytes of vN.
        // Assumptions: vN.num_bytes() == sizeof(void*).
        //              There must be a preceding execution of the corresponding VA_START instruction.
        VA_END,

        // VA_ARG n vN vM
        // Let 'p' be the address (pointer) stored in bytes of vM. The instruction first copies
        // bytes at addresses [p,p+vN.num_bytes()) to the bytes of vN. Then the address p in the
        // bytes of vN is moved (shifted) to point to the next variable function parameter (packed
        // into the '...' C parameter). The moved address will be stored in bytes of vM.
        // Assumptions: vN.num_bytes() == sizeof(void*).
        //              There must be a preceding execution of the corresponding VA_START instruction.
        //              There must follow an execution of the corresponding VA_END instruction.
        //              The address 'p' must be equal to one returned either be the corresponding VA_START
        //              instruction or a preceding VA_ARG instruction.
        VA_ARG,
    };

    enum struct Modifier
    {
        NONE,
        SIGNED,
        UNSIGNED,
        FLOATING,
        FLOATING_UNORDERED,
    };

    enum struct Descriptor
    {
        STATIC,
        LOCAL,
        PARAMETER,
        CONSTANT,
        FUNCTION,
    };

    std::uint32_t basic_block_index() const { return basic_block_index_; }
    std::uint32_t index() const { return index_; }
    Opcode opcode() const { return opcode_; }
    Modifier modifier() const { return modifier_; }
    std::vector<std::uint32_t> const& operands() const { return operands_; }
    std::vector<Descriptor> const& descriptors() const { return descriptors_; }
    SourceBackMapping const& source_back_mapping() const { return back_mapping_; }

    void set_basic_block_index(std::uint32_t const index) { basic_block_index_ = index; }
    void set_index(std::uint32_t const index) { index_ = index; }
    void set_opcode(Opcode const op) { opcode_ = op; }
    void set_modifier(Modifier const modifier) { modifier_ = modifier; }
    void push_back_operand(std::uint32_t const operand, Descriptor const descriptor);
    void assign(Instruction const& other);
    SourceBackMapping& source_back_mapping() { return back_mapping_; }
private:
    std::uint32_t basic_block_index_{ 0U };
    std::uint32_t index_{ 0U };
    Opcode opcode_{ Opcode::__INVALID__ };
    Modifier modifier_{ Modifier::NONE };
    std::vector<std::uint32_t> operands_{};
    std::vector<Descriptor> descriptors_{};
    SourceBackMapping back_mapping_{};
};


struct BasicBlock
{
    std::uint32_t function_index() const { return function_index_; }
    std::uint32_t index() const { return index_; }
    std::vector<Instruction> const& instructions() const { return instructions_; }
    std::vector<std::uint32_t> const& successors() const { return successors_; }

    void set_function_index(std::uint32_t const index) { function_index_ = index; }
    void set_index(std::uint32_t const index) { index_ = index; }
    Instruction& push_back_instruction();
    void push_back_successor(std::uint32_t const succ_index) { successors_.push_back(succ_index); }
    void pop_back_instruction();
    void assign_instruction(std::size_t index, Instruction const& instruction);
    Instruction& instruction_ref(std::uint32_t const idx) { return instructions_.at(idx); }
    Instruction& last_instruction_ref() { return instructions_.back(); }
    std::uint32_t& successor_ref(std::uint32_t const idx) { return successors_.at(idx); }

private:
    std::uint32_t function_index_{ 0U };
    std::uint32_t index_{ 0U };
    std::vector<Instruction> instructions_{};
    std::vector<std::uint32_t> successors_{};
};


struct Function
{
    Program* program() const { return program_; }
    std::string const& name() const { return name_; }
    std::uint32_t index() const { return index_; }
    std::vector<BasicBlock> const& basic_blocks() const { return blocks_; }
    std::vector<Variable> const& parameters() const { return parameters_; }
    std::vector<Variable> const& local_variables() const { return locals_; }
    bool is_external() const { return is_external_; }
    std::size_t initial_stack_bytes() const;
    SourceBackMapping const& source_back_mapping() const { return back_mapping_; }

    void set_program(Program* const program) { program_ = program; }
    void set_name(std::string const& name) { name_ = name; }
    void set_index(std::uint32_t const index) { index_ = index; }
    void set_external(bool const state) { is_external_ = state; }
    BasicBlock& basic_block_ref(std::uint32_t const idx) { return blocks_.at(idx); }
    BasicBlock& push_back_basic_block();
    BasicBlock& last_basic_block_ref() { return blocks_.back(); }
    Variable& push_back_parameter();
    Variable& push_back_local_variable();
    Variable& last_local_variable_ref() { return locals_.back(); }
    SourceBackMapping& source_back_mapping() { return back_mapping_; }
private:
    Program* program_{ nullptr };
    std::string name_{};
    std::uint32_t index_{ 0U };
    std::vector<BasicBlock> blocks_{};
    std::vector<Variable> parameters_{};
    std::vector<Variable> locals_{};
    bool is_external_{ false };
    mutable std::size_t initial_stack_bytes_{ std::numeric_limits<std::size_t>::max() };
    SourceBackMapping back_mapping_{};
};


struct Program
{
    explicit Program();

    std::string const& version() const { return version_; }
    std::string const& system() const { return system_; }
    std::uint16_t num_cpu_bits() const { return num_cpu_bits_; }
    std::string name() const { return name_; }
    std::uint32_t entry_function() const { return entry_function_; }
    std::vector<Function> const& functions() const { return functions_; }
    std::vector<Variable> const& static_variables() const { return variables_; }
    std::vector<Constant> const& constants() const { return constants_; }
    std::vector<std::pair<std::uint32_t, std::string> > const& external_variables() const { return external_variables_; }
    std::vector<std::uint32_t> external_functions() const { return external_functions_; }

    static std::uint32_t static_initializer();
    static std::string static_initializer_name();

    void set_system(std::string const& system) { system_ = system; }
    void set_num_cpu_bits(std::uint16_t const num_bits) { num_cpu_bits_ = num_bits; }
    void set_name(std::string const& name) { name_ = name; }
    void set_entry_function(std::uint32_t const index) { entry_function_ = index; }
    Function& function_ref(std::uint32_t const index) { return functions_.at(index); }
    Variable& static_variable_ref(std::uint32_t const index) { return variables_.at(index); }
    Function& push_back_function(std::string const& func_name);
    Variable& push_back_static_variable();
    Constant& push_back_constant();
    Constant& constant_ref(std::uint32_t const index) { return constants_.at(index); }
    void push_back_external_variable(std::uint32_t const index, std::string const& name);
    void push_back_external_function(std::uint32_t const index);

private:
    std::string version_;
    std::string system_;
    std::uint16_t num_cpu_bits_;
    std::string name_;
    std::uint32_t entry_function_;
    std::vector<Function> functions_;
    std::vector<Variable> variables_;
    std::vector<Constant> constants_;
    std::vector<std::pair<std::uint32_t, std::string> > external_variables_;
    std::vector<std::uint32_t> external_functions_;
};


}

#endif
