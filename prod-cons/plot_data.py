import pandas as pd
import matplotlib.pyplot as plt
import argparse
import os
from itertools import cycle
from matplotlib.ticker import MaxNLocator

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot data from a file and save as PDF.")
parser.add_argument("filename", help="Input text file with 3 columns: prod_num, cons_num, time")
args = parser.parse_args()

# Read data
df = pd.read_csv(args.filename, sep=r'\s+', header=None, names=["prod_num", "cons_num", "time"])

# Create the plot
plt.figure(figsize=(10, 6))

# Marker and color cycles
markers = cycle(['o', 's', '^', 'd', '*', 'x', '+', 'v', '<', '>'])
colors = cycle(plt.rcParams['axes.prop_cycle'].by_key()['color'])

# Plot each group with a different marker and color
for prod_num, group in df.groupby("prod_num"):
    plt.plot(
        group["cons_num"],
        group["time"],
        label=f"P={prod_num}",
        marker=next(markers),
        color=next(colors)
    )

plt.xlabel("Q")
plt.ylabel("Time [us]")
plt.title("Average waiting time")
plt.legend()
plt.grid(True)

# Force x-axis to show integer ticks only
plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))

# Save the plot
output_pdf = "figure.pdf"
plt.savefig(output_pdf)
plt.close()

print(f"Plot saved to {output_pdf}")
