# RSSI-data collection and Processing 

## Introduction

This project is part of my bachelor thesis. It is designed to extract and process RSSI values from Bluetooth Low Energy (BLE) packets from AirTags. 
[This repository](https://github.com/jimmywong2003/nrf5-ble-scan-filter-example) has been used as a foundation for the nrf Application.

### Main components 

1. The folder `ble_app_uart_adv_scan` contains the source code including the main.c file. It can scan BLE packets, filters AirTags and logs the corresponding RSSI value together with the MAC-Adress.
2. The folder `Experiments` contains a subfolder [Evaluation](Experiments/Evaluation) tools written in python for creating and analyzing CSV files out of the raw scanning logs. The subfolder `Results` contains raw        

### Requirement
* nRF5 SDK 17.02
* nRF52840 DK Board (pca10056)
* Segger Embedded Studio 4.5 or later
