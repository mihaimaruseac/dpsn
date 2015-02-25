import math
import random

# arguments
xmin = 0
xmax = 100
ymin = 0
ymax = 100
M = 100
theta = 20
N = 200
filename = 'debug_N{}_theta{}.dat'.format(N, theta)
random.seed(42)

# model of real world
def model_fun(x, y):
    a = (x/100 - .75) ** 2 + (y /100 - .65) ** 2
    a = math.sqrt(a)
    ret = 90 - 250 * math.sin(a)/(a + 10)
    assert(0 < ret < M)
    return ret

# don't change below
def generate_point():
    x = random.uniform(xmin, xmax)
    y = random.uniform(ymin, ymax)
    return (x, y, model_fun(x, y))

with open(filename, 'w') as f:
    f.write('{} {} {} {}\n'.format(xmin, ymin, xmax, ymax))
    f.write('{} {} {}\n'.format(M, theta, N))
    for i in xrange(N):
        f.write('{0[0]:5.2f} {0[1]:5.2f} {0[2]:5.2f}\n'.format(generate_point()))
