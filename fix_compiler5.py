import sys, re

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

code = re.sub(
    r"uint32_t\* patch_ptr = reinterpret_cast<uint32_t\*>\(&bytecode\[patch_ip\]\);\s*\n\s*\*patch_ptr = target_ip;",
    r"bytecode[patch_ip] = (target_ip >> 24) & 0xFF;\n        bytecode[patch_ip+1] = (target_ip >> 16) & 0xFF;\n        bytecode[patch_ip+2] = (target_ip >> 8) & 0xFF;\n        bytecode[patch_ip+3] = target_ip & 0xFF;",
    code
)

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Done")
