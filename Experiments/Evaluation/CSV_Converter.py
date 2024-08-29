import csv

# This program is dedicated to aggregate the data from the experiments into a CSV file.

filename = 'YOUR TXT FILE NAME'
distances = ['DistanceIdx1', 'DistanceIdx2', 'DistanceIdx3', '...']

# This global variable keeps track of the distance of the individual measurements. It will be changed during the
# iteration over the logfile and together with the distances array it will recognize the corresponding
# distance of each measurement.

indexCurrentAsInt = 0


with open(filename, 'r') as file:
    lines = file.readlines()

with open('output.csv', 'w') as csvfile:
    fieldnames = ['RSSI-Value', 'MAC-Address', 'Timestamp', 'Distance']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()

    for i in range(0, len(lines)):
        if lines[i].startswith(' app: Current index: '):
            indexCurrent = lines[i].split(' app: Current index: ')[1][:-2]
            indexCurrentAsInt = int(indexCurrent)

        if lines[i].startswith(' app: AirTag Identified'):
            print(lines[i+1].split(' app: Peer Address is ')[1][:-2])
            print(int(lines[i+2].split(',')[0].split(' app: RSSI: ')[1]))
            print(int(lines[i+2].split(',')[1].split(' timestamp: ')[1][:-2]))
            print(indexCurrentAsInt)
