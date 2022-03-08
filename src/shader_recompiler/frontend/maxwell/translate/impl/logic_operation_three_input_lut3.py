# Copyright © 2022 degasus <markus@selfnet.de>
# This work is free. You can redistribute it and/or modify it under the
# terms of the Do What The Fuck You Want To Public License, Version 2,
# as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

from itertools import product

# The primitive instructions
OPS = {
    'ir.BitwiseAnd({}, {})' : (2, 1, lambda a,b: a&b),
    'ir.BitwiseOr({}, {})' : (2, 1, lambda a,b: a|b),
    'ir.BitwiseXor({}, {})' : (2, 1, lambda a,b: a^b),
    'ir.BitwiseNot({})' : (1, 0.1, lambda a: (~a) & 255), # Only tiny cost, as this can often inlined in other instructions
}

# Our database of combination of instructions
optimized_calls = {}
def register(imm, instruction, count, latency):
    # Use the sum of instruction count and latency as costs to evaluate which combination is best
    costs = count + latency + len(instruction) * 0.0001

    # Update if new or better
    if imm not in optimized_calls or optimized_calls[imm][3] > costs:
        optimized_calls[imm] = (instruction, count, latency, costs)
        return True

    return False

# Constants: 0, 1 (for free)
register(0, 'ir.Imm32(0)', 0, 0)
register(255, 'ir.Imm32(0xFFFFFFFF)', 0, 0)

# Inputs: a, b, c (for free)
ta = 0xF0
tb = 0xCC
tc = 0xAA
inputs = {
    ta : 'a',
    tb : 'b',
    tc : 'c',
}
for imm, instruction in inputs.items():
    register(imm, instruction, 0, 0)

# Try to combine two values from the db with an instruction.
# If it is better than the old method, update it.
while True:
    registered = 0
    calls_copy = optimized_calls.copy()
    for OP, (argc, cost, f) in OPS.items():
        for args in product(calls_copy.items(), repeat=argc):
            # unpack(transponse) the arrays
            imm = [arg[0] for arg in args]
            value = [arg[1][0] for arg in args]
            count = [arg[1][1] for arg in args]
            latency = [arg[1][2] for arg in args]

            registered += register(
                f(*imm),
                OP.format(*value),
                sum(count) + cost,
                max(latency) + cost)
    if registered == 0:
        # No update at all? So terminate
        break

# Hacky output. Please improve me to output valid C++ instead.
s = """    case {imm}:
        return {op};"""
for imm in range(256):
    print(s.format(imm=imm, op=optimized_calls[imm][0]))
