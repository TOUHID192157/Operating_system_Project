Demonstration Video link of MiniOs project 
https://drive.google.com/file/d/1XMUzvXY3g-Zpitj3DOnUXdN7xBpvckE7/view?usp=sharing
# MiniOS Project

## Overview
MiniOS is a simulation of a basic operating system implemented in C. It demonstrates:

- Process management using Process Control Blocks (PCBs)
- Memory management with paging and logical-to-physical address translation
- Deadlock avoidance using Banker's Algorithm
- Round-Robin CPU scheduling
- A simple file system for storing and retrieving text files
- Stress testing with multiple processes and resource requests

This project gives a practical understanding of fundamental operating system concepts.

---

## Features
- **Process Management:** Create, run, and monitor multiple processes
- **Memory Management:** Allocate memory in pages and translate logical addresses
- **Resource Allocation:** Manage multiple resources safely using Banker’s Algorithm
- **Scheduler:** Round-Robin scheduling with a time quantum of 2 units
- **File System:** Store and retrieve small files
- **Stress Test:** Test system stability under multiple processes and resource requests

---

## STAR Format: Challenges and Solutions

### Situation
While developing MiniOS, I faced challenges in integrating multiple OS functionalities—like process management, memory paging, resource allocation, and file system—into a stable system that could run interactive commands.

### Task
I needed to simulate multiple processes with dynamic memory allocation and safe resource management, while allowing the user to interact with the system via shell commands.

### Action
I implemented:
- Process creation and state management with PCBs
- Paging-based memory management with frame allocation
- Banker’s Algorithm for deadlock avoidance
- Round-Robin scheduler
- Interactive shell for commands like `create`, `ps`, `addr`, `req`, `store`, `cat`, `dump_mem`
- Stress tests for multiple processes and random resource requests

### Result
- Successfully handled multiple processes and resources simultaneously
- Prevented deadlocks and unsafe resource allocations
- Demonstrated proper process state transitions and memory management
- Maintained system stability under stress conditions



## How to Run

1. Clone the repository:
```bash
git clone https:/https://github.com/TOUHID192157/Cse_323_miniOs
