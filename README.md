# **sala**

**Sala** is an acronym for **S**imple **A**ssembly **LA**nguage. Its
instructions are similar to those in **LLVM**. However, everything
is considerably simplified. For instance, registers are all just
genuine local (stack) variables. There are no types associated with
variables or any data. Functions return values via the first parameter.
Constant and data segment is initialized via regular code (inside the
initializer routine). The code is not in SSA form. And more. See the
header file `program.hpp` for details about the instruction set.

The library also comprises `Interpreter` class providing interpreted
execution of a **sala** program. The `Sanitizer` class can be used to
prevent crash of the entire `Interpreter`, if the execution would trigger
a serious defect in the program.

On little-endian Linux, integer `EXTEND` and `TRUNCATE` support byte widths
from one through eight, including three-byte memory accesses; floating-point
conversions retain their existing supported widths. Bytewise `AND` also supports
these widths for masks emitted with odd-width conversions. `InputFlow` propagates
dependencies through the same integer conversions and masks.

Linux i386 variadic calls use a four-byte pointer `va_list` and four-byte-rounded
argument slots. The argument area belongs to the variadic function's stack
record: `va_copy` copies a cursor, `va_end` does not invalidate other cursors,
and returning releases the area. The sanitizer and input-flow analyzer follow
that lifetime. This does not add LP64 `va_copy` or other target ABIs.

The native ILP32 pointer model still divides each pointer into a 16-bit segment
and a 16-bit offset. Objects (and combined static/constant storage) requiring
offsets above 65535 are not supported; widening just the offset would violate
the target's four-byte pointer layout. ILP32 variadic areas are rejected before
allocation when their rounded extent exceeds 65532, preserving the one-past
cursor without truncation.

## **Dependencies**

**libutility**
