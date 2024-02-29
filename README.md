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

## **Dependencies**

**libutility**
