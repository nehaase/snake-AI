import pandas as pd
import matplotlib.pyplot as plt

with open('stats_raw.csv', 'r') as f:
    stats = pd.read_csv(f)

#plt.plot(stats['game_nr'], stats['score'])

print(stats)