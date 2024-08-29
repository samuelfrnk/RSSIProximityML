import csv
from datetime import datetime, timedelta


# This program is dedicated to aggregate the data from the experiments into a CSV file.

filename = 'EXPERIMENT_LOGS.rtf'
distances = ['distanceAtIdx1', 'distanceAtIdx2', '...']

# The exact time when you started the experiment by enabling logs in nRF board for the first time
time_of_start_experiment = datetime(2024, 8, 28, 14, 41, 0)


# This global variable keeps track of the distance of the individual measurements. It will be changed during the
# iteration over the logfile and together with the distances array it will recognize the corresponding
# distance of each measurement.
indexCurrentAsInt = 0


with open(filename, 'r') as file:
    lines = file.readlines()

with (open(filename.split('.')[0] + '.csv', 'w') as csvfile):
    fieldnames = ['RSSI-Value', 'MAC-Address', 'Timestamp', 'Distance']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()

    for i in range(0, len(lines)):
        if lines[i].startswith(' app: Current index: '):
            indexCurrent = lines[i].split(' app: Current index: ')[1][:-2]
            indexCurrentAsInt = int(indexCurrent)

        if lines[i].startswith(' app: AirTag Identified'):
            mac_address = lines[i+1].split(' app: Peer Address is ')[1][:-2]
            rssi_value = int(lines[i+2].split(',')[0].split(' app: RSSI: ')[1])
            time_since_start = int(lines[i+2].split(',')[1].split(' timestamp: ')[1][:-2])
            timestamp_of_measurement = time_of_start_experiment + timedelta(seconds=time_since_start)
            time = timestamp_of_measurement.strftime('%Y-%m-%d %H:%M:%S')
            distance = distances[indexCurrentAsInt]
            writer.writerow({
                fieldnames[0]: rssi_value,
                fieldnames[1]: mac_address,
                fieldnames[2]: time,
                fieldnames[3]: distance
            })



