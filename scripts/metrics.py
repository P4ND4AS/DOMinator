import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics.csv")
plt.plot(df["Episode"], df["Total_PnL"], label="Total PnL ($)")
plt.xlabel("Episode")
plt.ylabel("Total PnL")
plt.title("PnL par Episode")
plt.legend()
plt.savefig("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/pnl_plot.png")
plt.show()