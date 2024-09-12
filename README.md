# RSSI-data collection and Processing 

## Introduction

This project is part of my bachelor thesis. It is designed to extract and process RSSI values from Bluetooth Low Energy (BLE) packets from AirTags. 
[This repository](https://github.com/jimmywong2003/nrf5-ble-scan-filter-example) has been used as a foundation for the nrf Application.

### Main components 

1. The folder [ble_app_uart_adv_scan](ble_app_uart_adv_scan) contains the source code including the main.c file. It can scan BLE packets, filters AirTags and logs the corresponding RSSI value together with the MAC-Adress.
2. The folder [Experiment](Experiments) contains both data and tools used to gather the data. 

### First Experiment

The first data gathering experiment took place in the BINZ building of the university of zurich. The distance was gradually increased from 0m to 8.5m. The set contains 2'496 entries: 
- [Raw logging files](Experiments/Results/Raw%20Data/Experiment1_0m_8.5m.rtf)
- [Standardized labeled CSV files](Experiments/Results/Processed%20Data/Experiment1_0m_8.csv)
- [Metadata ](Experiments/Results/Overview%20Data/Aggregated_Ex_1_and_2.csv) about the set grouped by distance.

### Second Experiment

The first data gathering experiment took place in the BINZ building of the university of zurich. The distance was gradually increased from 9m to 55m. The set contains 801 entries: 
- [Raw logging files](Experiments/Results/Raw%20Data/Experiment2_9m_55m.rtf)
- [Standardized labeled CSV files](Experiments/Results/Processed%20Data/Experiment2_9m_55m.csv)
- [Metadata ](Experiments/Results/Overview%20Data/Aggregated_Ex_1_and_2.csv) about the set grouped by distance.

       

### Requirement
* nRF5 SDK 17.02
* nRF52840 DK Board (pca10056)
* Segger Embedded Studio 4.5 or later
