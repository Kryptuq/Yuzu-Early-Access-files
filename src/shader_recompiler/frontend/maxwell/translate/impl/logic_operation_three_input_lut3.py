# SPDX-FileCopyrightText: 2022 degasus <markus@selfnet.de>
# SPDX-License-Identifier: WTFPL

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
def cmp(lhs, rhs):
    if lhs is None: # new entry
        return True
    if lhs[3] > rhs[3]: # costs
        return True
    if lhs[3] < rhs[3]: # costs
        return False
    if len(lhs[0]) > len(rhs[0]): # string len
        return True
    if len(lhs[0]) < len(rhs[0]): # string len
        return False
    if lhs[0] > rhs[0]: # string sorting
        return True
    if lhs[0] < rhs[0]: # string sorting
        return False
    assert lhs == rhs, "redundant instruction, bug in brute force"
    return False
def register(imm, instruction, count, latency):
    # Use the sum of instruction count and latency as costs to evaluate which combination is best
    costs = count + latency

    old = optimized_calls.get(imm, None)
    new = (instruction, count, latency, costs)

    # Update if new or better
    if cmp(old, new):
        optimized_calls[imm] = new
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
    register((~imm) & 255, 'ir.BitwiseNot({})'.format(instruction), 0.099, 0.099) # slightly cheaper NEG on inputs

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
