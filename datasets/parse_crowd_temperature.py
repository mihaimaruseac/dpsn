import matplotlib.pyplot as plt
import numpy as np

xs = []
ys = []
ss = []

with open('crowd_temperature.csv', 'r') as f:
    f.readline()
    for line in f:
        taxid, date, time, lat, lon, temp = line.split(',')
        xs.append(float(lat))
        ys.append(float(lon))
        ss.append(float(temp))

#for t in xrange(2, 26, 2):
#    print t
#    plt.scatter(xs, ys, c=map(lambda x: x < t, ss))
#    plt.show()

#print min(xs), max(xs)
#print min(ys), max(ys)
#print min(ss), max(ss), np.percentile(ss, 25),\
#        np.percentile(ss, 50), np.percentile(ss, 75)

for x, y, s in zip(xs, ys, ss):
    print '{:.4f} {:.4f} {:.4f}'.format(x, y, s)
