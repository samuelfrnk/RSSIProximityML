import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('Experiment_2.csv')

# Plot the data with distinct colors for each MAC address
plt.figure(figsize=(10, 6))

mac_addresses = df['MAC-Address'].unique()
colors = plt.cm.tab20(range(len(mac_addresses)))

# Plot each MAC address with a distinct color
for i, mac in enumerate(mac_addresses):
    mac_data = df[df['MAC-Address'] == mac]
    plt.scatter(mac_data['Distance'], mac_data['RSSI-Value'], color=colors[i], label=mac)

plt.ylim(-95, -12)


plt.xlabel('Distance')
plt.ylabel('RSSI Value')
plt.title('RSSI and Distance by MAC Address')
plt.grid(True)

plt.show()
