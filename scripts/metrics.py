import pandas as pd
import matplotlib.pyplot as plt

# Charger les fichiers CSV
metrics_df = pd.read_csv("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics.csv")
trades_df = pd.read_csv("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/trades.csv")

# 1. Barplot des PnL totaux par trajectoire
plt.figure(figsize=(10, 5))
plt.bar(metrics_df["Episode"], metrics_df["Total_PnL"], color="blue", alpha=0.7)
plt.xlabel("Episode")
plt.ylabel("Total PnL ($)")
plt.title("Total PnL par Episode")
plt.grid(True, axis="y")
plt.savefig("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/pnl_barplot.png")
plt.close()

# 2. Equity Curve (PnL cumulatif par trade pour chaque trajectoire)
plt.figure(figsize=(10, 5))
for episode in metrics_df["Episode"]:
    episode_trades = trades_df[trades_df["Episode"] == episode]
    cumulative_pnl = episode_trades["PnL"].cumsum()
    plt.plot(episode_trades["Timestep_Exit"], cumulative_pnl, label=f"Episode {episode}")
plt.xlabel("Timestep")
plt.ylabel("Cumulative PnL ($)")
plt.title("Equity Curve par Episode")
plt.legend()
plt.grid(True)
plt.savefig("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/equity_curve.png")
plt.close()

# 3. Max Drawdown par trajectoire
plt.figure(figsize=(10, 5))
plt.plot(metrics_df["Episode"], metrics_df["Max_Drawdown"], marker="o", color="red")
plt.xlabel("Episode")
plt.ylabel("Max Drawdown ($)")
plt.title("Max Drawdown par Episode")
plt.grid(True)
plt.savefig("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/max_drawdown.png")
plt.close()

# 4. Sharpe Ratio par trajectoire
plt.figure(figsize=(10, 5))
plt.plot(metrics_df["Episode"], metrics_df["Sharpe_Ratio"], marker="o", color="green")
plt.xlabel("Episode")
plt.ylabel("Sharpe Ratio")
plt.title("Sharpe Ratio par Episode")
plt.grid(True)
plt.savefig("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/sharpe_ratio.png")
plt.close()