import sys, re

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

code = re.sub(
    r"uint32_t ([a-zA-Z0-9_]+) = \*reinterpret_cast<const uint32_t\*>\(&bytecode\[i\]\);",
    r"uint32_t \1 = (bytecode[i] << 24) | (bytecode[i+1] << 16) | (bytecode[i+2] << 8) | bytecode[i+3];",
    code
)
code = re.sub(
    r"uint32_t ([a-zA-Z0-9_]+) = \*reinterpret_cast<const uint32_t\*>\(&bcode\[i\]\);",
    r"uint32_t \1 = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];",
    code
)

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Done")
