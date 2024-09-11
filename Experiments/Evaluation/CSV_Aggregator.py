import pandas as pd

file_name = 'Experiment_with_No_Batteries.csv'
df = pd.read_csv(file_name)

df['Timestamp'] = pd.to_datetime(df['Timestamp'], errors='coerce')

grouped = df.groupby('Distance').agg(
    average_rssi=('RSSI-Value', 'mean'),
    total_measurements=('RSSI-Value', 'count'),
    number_different_mac_addresses=('MAC-Address', pd.Series.nunique),
    time_between_first_and_last_measurement=('Timestamp', lambda x: x.max() - x.min()),
    lowest_rssi=('RSSI-Value', 'min'),
    highest_rssi=('RSSI-Value', 'max')
).reset_index()

grouped['time_between_first_and_last_measurement'] = grouped['time_between_first_and_last_measurement'].dt.total_seconds()

overall = pd.DataFrame({
    'Distance': ['Overall'],
    'average_rssi': [df['RSSI-Value'].mean()],
    'total_measurements': [df['RSSI-Value'].count()],
    'number_different_mac_addresses': [df['MAC-Address'].nunique()],
    'time_between_first_and_last_measurement': [(df['Timestamp'].max() - df['Timestamp'].min()).total_seconds()],
    'lowest_rssi': [df['RSSI-Value'].min()],
    'highest_rssi': [df['RSSI-Value'].max()]
})

grouped = pd.concat([grouped, overall], ignore_index=True)
grouped.to_csv('aggregated_data_with_overall.csv', index=False)
