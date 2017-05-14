import math
import random

# arguments
xmin = 0
xmax = 1000
ymin = xmin
ymax = xmax
M = 100
m = 20
theta = 80
N = 20000
filename = 'sz_N{}_theta{}_sz{}.dat'.format(N, theta, xmax)
random.seed(42)

# model of real world
def model_fun(x, y):
    #a = (x/100 - .75) ** 2 + (y /100 - .65) ** 2
    #a = math.sqrt(a)
    #ret = 90 - 250 * math.sin(a)/(a + 10)
    d = (x - 60) ** 2 + (y - 15) ** 2
    ret = m + M * math.exp(-d/500)
    if ret > M: ret = M
    return ret

# don't change below
def generate_point():
    x = random.uniform(xmin, xmax-1)
    y = random.uniform(ymin, ymax-1)
    return (x, y, model_fun(x, y))

with open(filename, 'w') as f:
    f.write('{} {} {} {}\n'.format(xmin, ymin, xmax, ymax))
    f.write('{} {} {}\n'.format(M, theta, N))
    for i in xrange(N):
        f.write('{0[0]:5.2f} {0[1]:5.2f} {0[2]:5.2f}\n'.format(generate_point()))
