# Sample three-address code for the optimizer
# sourceA and sourceB represent values supplied by an earlier program stage.
a = sourceA
b = sourceB
unused = 99
sum1 = a + b
sum2 = a + b
constant = 10 * 3
answer = sum2 + constant
print answer
