import itertools
import sys

all_exps = {}

class Exp:
    def __init__(self):
        self._l = []
        self._n = 0

    def record(self, l):
        self._l = map(lambda (x, y): x + y,
                itertools.izip_longest(self._l, l, fillvalue=0))
        self._n += 1

    def print_exp(self, k):
        self._l = map(lambda x: x / (0.0+self._n), self._l)
        self._n = None # prevent more records of the experiment
        print ','.join(map(str, list(k) + self._l))

class Experiment:
    def __init__(self, kwargs):
        self.kwargs = kwargs
        assert kwargs["./dpsn"] == "./dpsn"
        self.ALPHA = kwargs["ALPHA"]
        self.BETA = kwargs["BETA"]
        self.K = kwargs["K"]
        self.NT = kwargs["NT"]
        self.EPS = kwargs["EPS"]
        self.METHOD = kwargs["METHOD"]
        self.METHODARG = kwargs["METHODARG"]
        self.TESTTHRESH = kwargs["TESTTHRESH"]
        self.RESOLUTION = kwargs["RESOLUTION"]
        self.DATASET = kwargs["DATASET"]
        self.SEED = kwargs["SEED"]
        self.SENSORS = self.DATASET.split('_')[1][1:]
        self.tree_data_leaf = {}
        self.tree_data_all = {}
        self.unif_data = {}
        self.absolute_votes = {}
        self.relative_votes = {}
        self.prob_votes = {}

    def start_absolute(self, c):
        k = int(c)
        self.absolute_votes[k] = {}
        self._d = self.absolute_votes[k]

    def start_relative(self, c):
        k = float(c)
        self.relative_votes[k] = {}
        self._d = self.relative_votes[k]

    def start_probabilistic(self, c):
        k = float(c)
        self.prob_votes[k] = {}
        self._d = self.prob_votes[k]

    def record_height(self, h):
        self.height = int(h)

    def record_data(self, threshold, star_values, bar_values):
        threshold = float(threshold)
        assert len(star_values) == 7 and len(bar_values) == 7
        d = { "sv" : map(float, star_values)
            , "bv" : map(float, bar_values)
            }
        if not self.tree_data_leaf.has_key(threshold):
            self.tree_data_leaf[threshold] = d
        elif not self.tree_data_all.has_key(threshold):
            self.tree_data_all[threshold] = d
        elif not self.unif_data.has_key(threshold):
            self.unif_data[threshold] = d
        elif not self._d.has_key(threshold):
            self._d[threshold] = d
        else:
            raise Exception("Repeated threshold: This case should not be reached!")

    def record_resolution(self, r):
        self.resolution = int(r)

    def print_exp(self):
        l = []
        for d in [self.tree_data_leaf, self.tree_data_all, self.unif_data] +\
            [self.absolute_votes[k] for k in sorted(self.absolute_votes.keys())] +\
            [self.relative_votes[k] for k in sorted(self.relative_votes.keys())] +\
            [self.prob_votes[k] for k in sorted(self.prob_votes.keys())]:
            for k in sorted(d.keys()):
                l += d[k]['sv']
                l += d[k]['bv']
        l1 = [self.ALPHA, self.BETA, self.K, self.NT, self.EPS, self.METHOD,
                self.METHODARG, self.TESTTHRESH, self.RESOLUTION,
                self.SENSORS]
        l2 = [self.height, self.resolution]

        key = tuple(l1)
        o = all_exps.get(key, Exp())
        o.record(l2 + l)
        all_exps[key] = o

for fname in sys.argv[1:]:
    exp = None
    with open(fname, "r") as f:
        line = f.readline()                                                   # Called with: argc=12
        while line:
            assert line.startswith("Called")
            line = f.readline().split()                                       # ./dpsn 0.2 0.2 5 3 0.2 t 3 0.01 1 datasets/debug_N20000_theta20.dat 42
            exp = Experiment(dict(zip(["./dpsn", "ALPHA", "BETA", "K", "NT",
                "EPS", "METHOD", "METHODARG", "TESTTHRESH", "RESOLUTION",
                "DATASET", "SEED"], line)))
            line = f.readline().split(':')[1].split()[0]                      # Sanitization finished, grid height: 3
            exp.record_height(line)
            while True:
                line = f.readline()                                           # Threshold: 15.00 | Values: 5106 647 6455 5808 13530 0.10 0.57 | 5106 725 6597 5872 13530 0.11 0.57
                if not line.startswith("Threshold"):
                    if line.startswith("Fine"):                               # Fine grid built, size 100 x 100
                        exp.record_resolution(line.split()[-1])
                        continue
                    if line.startswith("Testing on absolute"):                # Testing on absolute positive votes 1
                        exp.start_absolute(line.split()[-1])
                        continue
                    if line.startswith("Testing on relative"):                # Testing on relative positive votes 0.5
                        exp.start_relative(line.split()[-1])
                        continue
                    if line.startswith("Testing on probabilistic"):           # Testing on probabilistic weigths, 0.5
                        exp.start_probabilistic(line.split()[-1])
                        continue
                    break
                line = line.split()
                exp.record_data(line[1], line[4:11], line[12:19])
            exp.print_exp()

#print '======================================================================'
for k in sorted(all_exps.keys()):
    all_exps[k].print_exp(k)
