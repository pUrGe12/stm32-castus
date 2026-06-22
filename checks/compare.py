import sys, re

TOL = 0.05

def load(path):
    pairs = []
    for line in open(path):
        m = re.search(r'x_milli=(-?\d+)\s+y_milli=(-?\d+)', line)
        if m:
            pairs.append((int(m.group(1)) / 1000.0, int(m.group(2)) / 1000.0))
    return pairs

clean, test = load(sys.argv[1]), load(sys.argv[2])

if not clean or not test:
    sys.exit("ERROR: no x_milli/y_milli pairs parsed")

n = min(len(clean), len(test))
worst = (0.0, 0.0)
first_break = None
bad = 0

for i in range(n):
    (cx, cy), (tx, ty) = clean[i], test[i]
    d = abs(ty - cy)
    if d > worst[0]:
        worst = (d, cx)
    if d > TOL:
        bad += 1
        if first_break is None:
            first_break = (cx, cy, ty, d)

print(f"compared {n} samples, tol={TOL}")
print(f"max |Δy| = {worst[0]:.4f} at x={worst[1]:.4f}")
if first_break:
    x, cy, ty, d = first_break
    print(f"first divergence at x={x:.4f}: golden y={cy:.4f}  actual y={ty:.4f}  Δ={d:.4f}")
print(f"{bad}/{n} samples beyond tolerance")
sys.exit(1 if bad else 0)
