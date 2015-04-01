import sys

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

    def record_height(self, h):
        self.height = int(h)

    def record_data(self, threshold, star_j, star_fr, bar_j, bar_fr):
        threshold = float(threshold)
        d = { "star_j" : float(star_j)
            , "star_fr" : float(star_fr)
            , "bar_j" : float(bar_j)
            , "bar_fr" : float(bar_fr)
            }
        if not self.tree_data_leaf.has_key(threshold):
            self.tree_data_leaf[threshold] = d
        elif not self.tree_data_all.has_key(threshold):
            self.tree_data_all[threshold] = d
        elif not self.unif_data.has_key(threshold):
            self.unif_data[threshold] = d
        else:
            raise Exception("Repeated threshold: This case should not be reached!")

    def record_resolution(self, r):
        self.resolution = int(r)

    def print_exp(self):
        l = []
        for d in [self.unif_data]:
            for k in sorted(d.keys()):
                l += [d[k]['star_j'], d[k]['star_fr'], d[k]['bar_j'], d[k]['bar_fr']]
        print ','.join(map(str, [self.ALPHA, self.BETA, self.K, self.NT,
            self.EPS, self.METHOD, self.METHODARG, self.TESTTHRESH,
            self.RESOLUTION, self.SENSORS, self.SEED,
            self.resolution] + l))

for fname in sys.argv[1:]:
    exp = None
    with open(fname, "r") as f:
        line = f.readline()                                                   # Called with: argc=12
        while line:
            assert line.startswith("Called")
            line = f.readline().split(     )                                  # ./dpsn 0.2 0.2 5 3 0.2 t 3 0.01 1 datasets/debug_N20000_theta20.dat 42 
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
                    else:
                        break
                line = line.split()
                exp.record_data(line[1], line[9], line[10],
                        line[17], line[18])
            exp.print_exp()
