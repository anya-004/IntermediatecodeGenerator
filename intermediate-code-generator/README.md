# Intermediate Code Generator / Optimizer (C + Lex/Flex)

This mini-project reads simple three-address code (TAC), tokenizes it with Lex/Flex, and optimizes it in C.

## Optimizations included

1. **Constant propagation and constant folding** – substitutes known integer values and evaluates expressions such as `t1 = 10 + 20` into `t1 = 30`.
2. **Common sub-expression elimination (CSE)** – reuses the result of a repeated expression when its operands have not been reassigned.
3. **Dead code elimination (DCE)** – removes assignments whose values are never used by a later statement that produces observable output.

## Input format

Use one statement per line. Supported statements:

```text
variable = value
variable = value + value
variable = value - value
variable = value * value
variable = value / value
variable = value % value
print variable
```

A `value` can be a variable name or an integer (including a negative integer). Use `print` for values that should remain observable after dead-code elimination. Blank lines, `#` comments, `//` comments, and semicolons are accepted.

This teaching version handles straight-line TAC. It does not implement control-flow statements, arrays, function calls, floating-point values, or C source parsing.

## Run in GitHub Codespaces

Open the repository in Codespaces, then open the VS Code terminal (`Terminal` → `New Terminal`). Run:

```bash
sudo apt update
sudo apt install -y flex build-essential
make
./optimizer input.tac
```

The optimized TAC is printed to the terminal. The summary is written to standard error.

To save the optimized output to a file:

```bash
./optimizer input.tac > optimized.tac
```

To rebuild after editing the source:

```bash
make clean
make
```

## Expected output for the included sample

```text
a = sourceA
b = sourceB
sum1 = a + b
sum2 = sum1
constant = 30
answer = 30 + sum2
print answer
```

Why? `10 * 3` is folded into `30`, the repeated `a + b` expression is reused (`sum2 = sum1`), and the unused assignment is removed. The final `print` is retained.

## Files

- `lexer.l` – Flex rules that recognize identifiers, numbers, operators, `print`, comments, and line endings.
- `optimizer.h` – shared token definitions.
- `optimizer.c` – parser, optimization passes, and output generation.
- `input.tac` – sample input.
- `Makefile` – build commands.
