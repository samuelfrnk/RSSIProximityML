# Bachelor Thesis Samuel Frank

## 1. Introduction

This repository is part of my bachelor thesis. The work is dealing with BLE based Tracker which operate in a Crowded Source Finding Network (COFN). The technology is analysed using Apple's concrete implementation which is called AirTag in the Find My network. The focus is on security aspects and the misuse of technology for stalking purposes. 

It contains all the resources that were used to collect a labelled BLE RSSI dataset in various experiments. 

A combined dataset was then used in a second step to train and evaluate a classifier that classifies proximity based on the RSSI values together with the additional input features indoor and LOS. A version of the Decision Tree classifier achieved a performance score of 84%. This performance score balances accuracy, F1-score and a custom overfitting penalty 

In a final step, the selected classifier was extracted from the training environment using ONNX and implemented in HomeScout, an Android protection application developed by the Communication Systems Group of the Institute of Informatics at the University of Zurich. 


## 2. Overview Components

### **Data** 
- Final labeled [CSV Dataset](Experiments/Results/Data_CSV/Combined_Data/combined_data.csv) which combines all Experiments and contains over 13'300 entries.
  The corresponding sha256 hash of the final data set can be found [here](Experiments/Results/Data_CSV/Combined_Data/dataset_hash.txt). The script used to hash the dataset can be found [here](Experiments/Evaluation/Hash_Script/dataset_hash.py) for integrity checks. 
- [CSV Data](Experiments/Results/Data_CSV) from the individual data collection Experiments.
- An [Application](ble_app_uart_adv_scan/main.c) used in an embedded environment together with a nRF52840 DK Board (pca10056) to scan, filter and log BLE packets emitted from AirTags.
 [This repository](https://github.com/jimmywong2003/nrf5-ble-scan-filter-example) has been used as a foundation for the Application and was modified with an AirTag filter and other features.
- Several [Python Scripts](Experiments/Evaluation/Conversion_Skripts) have been used to transform the raw log files into CSV structure within the Data pipeline.

### **Classifier**  
- The final evaluation of the models including the performance scores is located in [this Jupyter Notebook](ML_Analysis/This_Work/Final_Evaluation/ML_Analysis_BA.ipynb). 
- [Initial Training and Evaluation](ML_Analysis/Darios_Notebook/Experiment_3/ML_Analysis_Experiment3.ipynb) based on the [work by Dario](https://github.com/dariomonopoli-dev/Bachelor_thesis_code) with comparison to a [binary classification](ML_Analysis/Darios_Notebook/Experiment_3/Binary_Bins/ML_Analysis_Experiment3_BinaryBins_Smoothend.ipynb) modeling leading to initial iterative refinment to a binary classification. 

### Porting to HomeScout

- The adjusted [HomeScout Sourcode](HomeScout) including the introduced RSSI shielding classifier logic is also part of the repository. 

## 3. Further Information 

### **Data** 

More information about the performed data experiments and their metadata : 

| Experiment Nr. | Entries             | Data                                      | Metadata                                   | Distance (m)                        |
|----------------|---------------------|-------------------------------------------|--------------------------------------------|-------------------------------------|
| 1              | 2496 RSSI values     | [Experiment_1.csv](Experiments/Results/Data_CSV/Experiment_1.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_1_and_2.csv)  | 0.0 - 8.5                          |
| 2              | 707 RSSI values      | [Experiment_2.csv](Experiments/Results/Data_CSV/Experiment_2.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_1_and_2.csv)   | 10.0 - 50.0                         |
| 3              | 5869 RSSI values     | [Experiment_3.csv](Experiments/Results/Data_CSV/Experiment_3.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_3.csv)   | 0.0 - 10.0                         |
| 4              | 956 RSSI values     | [Experiment_4.csv](Experiments/Results/Data_CSV/Experiment_4.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_4.csv)   | 0.0 - 2.0                         |
| 5              | 3599 RSSI values     | [Experiment_5.csv](Experiments/Results/Data_CSV/Experiment_5.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_4.csv)   | 0.0 - 2.0                         |
| 6              | 4135 RSSI values     | [Experiment_6.csv](Experiments/Results/Data_CSV/Experiment_6.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_6.csv)   | 0.0 - 2.0                         |
| 7              | 3422 RSSI values     | [Experiment_7.csv](Experiments/Results/Data_CSV/Experiment_7.csv) | [Metadata](Experiments/Results/Overview_Data/Experiment_7.csv)   | 0.0 - 2.0                         |


Notes: 
- Based on insights from Experiments 1 and 2, the methodology was adjusted, leading to more concise data collection in Experiment 3.
- Experiment 4 was conducted to examine the influence of the battery life onto the RSSI values.
- Experiment 5 was conducted outside. Before the data was aggregated inside the large dataset, entries corresponding to a Mac addresses which had a RSSI value over 30 were filtered out over the all distances.
- Experiment 6 was conducted indoor and NLOS.  
- Experiment 7 was conducted outdoor and NLOS.  

Data Pipeline : 


![image](https://github.com/user-attachments/assets/5d9d7021-80a7-4aaf-9e24-c2a899927a1f)




 ### **Classifier** 

Perormance Score of the evaluated models: 

![image](https://github.com/user-attachments/assets/e26fdd74-ea0a-4d73-9f5a-c668f994c836)


### **HomeScout Integration** 

Overview over the Architecture of HomeScout as well as Areas that needed modifications highlighted in red : 

![image](https://github.com/user-attachments/assets/842d4769-e0ce-4244-9c95-7585b9f8f054)




### Requirement

* nRF5 SDK 17.02
* nRF52840 DK Board (pca10056)
* Segger Embedded Studio 4.5 or later
