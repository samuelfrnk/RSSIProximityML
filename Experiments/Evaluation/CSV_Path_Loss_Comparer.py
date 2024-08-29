import pandas as pd
import numpy as np

data = pd.read_csv('Aggregated_Ex_1_and_2.csv')

rssi_ref = -66.79
n = 4


def estimate_distance(rssi, rssi_ref, n):
    return 10 ** ((rssi_ref - rssi) / (10 * n))


data['estimated_distance'] = estimate_distance(data['average_rssi'], rssi_ref, n)

# Convert Distance and estimated_distance to numeric types
data['Distance'] = pd.to_numeric(data['Distance'], errors='coerce')
data['estimated_distance'] = pd.to_numeric(data['estimated_distance'], errors='coerce')

# Calculate the error
data['error'] = np.abs(data['Distance'] - data['estimated_distance'])

result = data[['Distance', 'average_rssi', 'estimated_distance', 'error']]

result.to_csv('evaluation_n_4.csv', index=False)

