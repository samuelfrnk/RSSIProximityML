# RSSI-data collection and Processing 

## Introduction

This project is part of my bachelor thesis. The focus is on scanning BLE packets containing RSSI data emitted from AirTags with a [skript](ble_app_uart_adv_scan/main.c) compatible with a nRF52840 DK Board (pca10056). Further it also contains [RSSI Data](Experiments/Results/Data_CSV) and tools used for processing the data.
[This repository](https://github.com/jimmywong2003/nrf5-ble-scan-filter-example) has been used as a foundation for the nrf Application.

### Main components 

1. The folder [ble_app_uart_adv_scan](ble_app_uart_adv_scan) contains the source code including the main.c file. It can scan BLE packets, filters AirTags and logs the corresponding RSSI value together with the MAC-Adress.
2. The folder [Experiments](Experiments) contains both data and tools used to gather the data. 

### Experiments

| Experiment Nr. | Entries             | Data                                      | Plots                | Metadata                                   | Distance (m)                        |
|----------------|---------------------|-------------------------------------------|----------------------|--------------------------------------------|-------------------------------------|
| 1              | 2496 RSSI values     | [Experiment_1.csv](Experiments/Results/Data_CSV/Experiment_1.csv) | [Plots](Experiments/Results/Plots/Experiment_1) | [Metadata](Experiments/Results/Overview_Data/Experiment_1_and_2.csv)  | 0.0 - 8.5                          |
| 2              | 707 RSSI values      | [Experiment_2.csv](Experiments/Results/Data_CSV/Experiment_2.csv) | [Plots](Experiments/Results/Plots/Experiment_2) | [Metadata](Experiments/Results/Overview_Data/Experiment_1_and_2.csv)   | 10.0 - 50.0                         |
| 3              | 5869 RSSI values     | [Experiment_3.csv](Experiments/Results/Data_CSV/Experiment_3.csv) | [Plots](Experiments/Results/Plots/Experiment_3) | [Metadata](Experiments/Results/Overview_Data/Experiment_3.csv)   | 0.0 - 10.0                         |


Notes: 
- Based on insights from Experiments 1 and 2, the methodology was adjusted, leading to more concise data collection in Experiment 3.

       

### Requirement
* nRF5 SDK 17.02
* nRF52840 DK Board (pca10056)
* Segger Embedded Studio 4.5 or later
