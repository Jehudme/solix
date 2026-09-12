import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

# Replace pointer casts with big endian writes
def replace_patch(code, ptr_name):
    target = f"""uint32_t* {ptr_name} = reinterpret_cast<uint32_t*>(&bytecode[patch_ip]);
            *{ptr_name} = """
    
    # We'll use a regex to replace it
    import re
    code = re.sub(
        r"uint32_t\* ([a-zA-Z0-9_]+) = reinterpret_cast<uint32_t\*>\(&bytecode\[([a-zA-Z0-9_]+)\]\);\s*\*\1 = ([a-zA-Z0-9_]+);",
        r"bytecode[\2] = (\3 >> 24) & 0xFF;\n            bytecode[\2+1] = (\3 >> 16) & 0xFF;\n            bytecode[\2+2] = (\3 >> 8) & 0xFF;\n            bytecode[\2+3] = \3 & 0xFF;",
        code
    )
    return code

code = replace_patch(code, "patch_ptr")

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Done")
