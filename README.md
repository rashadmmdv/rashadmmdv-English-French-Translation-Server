# English-French Translation Server

## Overview  
This project implements a translation server that dynamically updates its dictionary from files and processes client translation requests.

## Versions  

### **Version 1: Basic Server-Client**  
- **Server:** Stores word pairs, processes client requests, and returns translations.  
- **Client:** Sends random translation requests via signals (`SIGUSR1` for English-to-French, `SIGUSR2` for French-to-English).  
- **Dictionary Updates:** Loads word pairs from files at startup.  

### **Version 2: Shared Memory & Message Queue**  
- **Translation Writer:** Reads word pairs, categorizes them, and sends them to a message queue.  
- **Translation Reader:** Retrieves pairs from the queue and stores them in shared memory.  
- **Threads:** Enables concurrent processing.  
- **Dynamic Updates:** Periodically scans for new dictionary files.  

### **Version 3: Unified Server**  
- **Integrated Server:** Combines all functionalities, manages shared memory, and handles real-time dictionary updates.  
- **Missing Words Handling:** Reloads dictionary files if a requested word is missing.  

**[Read the full report](./Translation_Server_Project_Report.pdf)** 
