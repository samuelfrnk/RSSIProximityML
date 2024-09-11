import csv
from datetime import datetime, timedelta
import pandas as pd


# This program is dedicated to aggregate the data from the experiments into a CSV file.

filename = 'TEST_DUMMY'
distances = [0, 0.1, 0.2, 0.3, 3]

# The exact time when you started the experiment by enabling logs in nRF board for the first time
time_of_start_experiment = datetime(2024, 9, 11, 15, 45, 0)


# This global variable keeps track of the distance of the individual measurements. It will be changed during the
# iteration over the logfile and together with the distances array it will recognize the corresponding
# distance of each measurement.
indexCurrentAsInt = 0

filename_rtf = filename + '.rtf'

with open(filename_rtf, 'r') as file:
    lines = file.readlines()

with (open(filename_rtf.split('.')[0] + '.csv', 'w') as csvfile):
    fieldnames = ['RSSI-Value', 'MAC-Address', 'Timestamp', 'Distance']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()

    for i in range(0, len(lines)):
        if lines[i].startswith('<info> app: Current index: '):
            indexCurrent = lines[i].split('<info> app: Current index: ')[1][:-2]
            indexCurrentAsInt = int(indexCurrent)

        if lines[i].startswith('<info> app: AirTag Identified'):
            mac_address = lines[i+1].split('<info> app: Peer Address is ')[1][:-2]
            rssi_value = int(lines[i+2].split(',')[0].split('<info> app: RSSI: ')[1])
            time_since_start = int(lines[i+2].split(',')[1].split('timestamp: ')[1][:-2])
            timestamp_of_measurement = time_of_start_experiment + timedelta(seconds=time_since_start)
            time = timestamp_of_measurement.strftime('%Y-%m-%d %H:%M:%S')
            distance = distances[indexCurrentAsInt]
            writer.writerow({
                fieldnames[0]: rssi_value,
                fieldnames[1]: mac_address,
                fieldnames[2]: time,
                fieldnames[3]: distance
            })


filename_csv = filename + '.csv'
df = pd.read_csv(filename_csv)

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
grouped.to_csv(filename + '_aggregated.csv', index=False)
