#ifndef SALA_PLATFORM_SPECIFICS_HPP_INCLUDED
#   define SALA_PLATFORM_SPECIFICS_HPP_INCLUDED

#   include <cstdint>

namespace platform_linux_64_bit
{
    // The implementation is based on this document (search there for 'va_list'):
    //      https://raw.githubusercontent.com/wiki/hjl-tools/x86-psABI/x86-64-psABI-1.0.pdf
    //
    // The interpretation of VA_START instructions must perform an initialization of a single
    // instance of this structure (it must be present as a local variable of the executed
    // variadic function; its address is in the argument of VA_START) as discussed below
    // for individual fields.
    //
    // The interpretation of VA_END instructions must perform release of memory allocated
    // for the variadic parameters (see field 'reg_save_area' for more details).
    //
    // The interpretation of VA_ARG instructions does NOT require any action. According to
    // inspection of code produced by Clang the effect of the 'va_arg' macro is already
    // encoded in the body of the variadic function.
    //
    // NOTE: All fields, except 'overflow_arg_area', are used for "our purposes", which differ
    //       from their original purposes stated in the document above.
    struct va_list
    {
        // We must initialize this field to the number of allocated bytes pointed to by
        // 'reg_save_area' incremented by 256.
        // NOTE: The value in the field > 255 ensures the code will always branch as we expect. 
        std::uint32_t gp_offset;

        // We must initialize this field to value 256.
        // NOTE: The value in the field > 255 ensures the code will always branch as we expect. 
        std::uint32_t fp_offset;

        // A pointer into an array of 8-bytes long items. Each item is 8-bytes aligned
        // variadic parameter. They appear in the array in the order as they were
        // passed to the variadic function.
        //
        // Initially it must points to the first byte of the first item. 
        //
        // The 'va_arg' macro is supposed to move (shift) the pointer to the first byte
        // of the next item in the array. But that is encoded in the body of the variadic
        // function.
        void *overflow_arg_area;

        // We use this pointer for storing the address of the array 8-byte items discussed
        // above.
        //
        // The interpreter must allocate the array on heap as part of the interpretation of
        // the VA_START instruction. The memory must be released during interpretation of
        // the VA_END instruction. The size of the allocated memory (incremented by 256) must
        // be stored in the field 'fp_offset'.
        //
        // The value of this pointer should not change between VA_START and VA_END instructions.
        void *reg_save_area;
    };

    static_assert(sizeof(va_list) == 24ULL);
}

#endif
