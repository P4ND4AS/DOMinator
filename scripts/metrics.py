import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import matplotlib.colors as mcolors
import numpy as np


plt.style.use("seaborn-v0_8-whitegrid")
plt.rcParams.update({
    "font.size": 11,
    "axes.titlesize": 13,
    "axes.labelsize": 12,
    "legend.fontsize": 9,
    "font.family": "DejaVu Sans"
})


def get_color_gradient(start_hex, end_hex, n):
    start_rgb = np.array(mcolors.to_rgb(start_hex))
    end_rgb = np.array(mcolors.to_rgb(end_hex))
    return [mcolors.to_hex(start_rgb + (end_rgb - start_rgb) * i / (n - 1)) for i in range(n)]

metrics_df = pd.read_csv("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics.csv")
trades_df = pd.read_csv("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/trades.csv")


pnl_colors = ["#1cba7d" if val >= 0 else "#ba4343" for val in metrics_df["Total_PnL"]]
sharpe_colors = ["#1cba7d" if val >= 0 else "#ba4343" for val in metrics_df["Sharpe_Ratio"]]


fig, axs = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)

# 1. PnL
axs[0, 0].bar(metrics_df["Episode"], metrics_df["Total_PnL"], color=pnl_colors)
axs[0, 0].set_xlabel("Traj")
axs[0, 0].set_ylabel("($)")
axs[0, 0].set_title("Total PnL")
axs[0, 0].grid(True, axis="y")

# 2. Max Drawdowns
axs[0, 1].bar(metrics_df["Episode"], metrics_df["Max_Drawdown"], color="#737272")
axs[0, 1].set_xlabel("Traj")
axs[0, 1].set_ylabel("($)")
axs[0, 1].set_title("Max Drawdown")
axs[0, 1].grid(True, axis="y")

# 3. Sharpe Ratios
axs[1, 0].bar(metrics_df["Episode"], metrics_df["Sharpe_Ratio"], color=sharpe_colors)
axs[1, 0].set_xlabel("Traj")
axs[1, 0].set_title("Sharpe Ratio")
axs[1, 0].grid(True, axis="y")

# 4. Equity Curves
episodes = metrics_df["Episode"].tolist()
n_curves = len(episodes)
colors = get_color_gradient("#ba4343", "#1cba7d", n_curves)

num_labels = 5
label_indices = np.linspace(0, n_curves - 1, num=num_labels, dtype=int)

for i, episode in enumerate(episodes):
    episode_trades = trades_df[trades_df["Episode"] == episode]
    cumulative_pnl = episode_trades["PnL"].cumsum()
    label = f"Traj : {episode}" if i in label_indices else None
    axs[1, 1].plot(episode_trades["Timestep_Exit"], cumulative_pnl, label=label, color=colors[i])

axs[1, 1].set_xlabel("Transition")
axs[1, 1].set_ylabel("($)")
axs[1, 1].set_title("Equity Curve")
axs[1, 1].legend(fontsize=8)
axs[1, 1].grid(True)

fig.suptitle("Summary of learning phase of AI Trading Agent", fontsize=15)


plt.show()
plt.savefig("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics_plot.png", dpi=300, bbox_inches='tight')
