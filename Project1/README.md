# CS 3502 Project 1: Multi-Threaded Banking System

**Author:** Ethan Diawara
**Course:** CS 3502 - Operating Systems
**Date:** July 2026

---

## Project Overview

This project implements a multi-threaded banking system in four phases to demonstrate key operating systems concepts:


## Compilation

All phases compile with the same command:

```bash
gcc -Wall -pthread phase1.c -o phase1
gcc -Wall -pthread phase2.c -o phase2
gcc -Wall -pthread phase3.c -o phase3
gcc -Wall -pthread phase4.c -o phase4


## Execution
./phase1   # Race condition demo (no locks)
./phase2   # Mutex-protected system (no race conditions)
./phase3   # Deadlock creation (will hang - press Ctrl+C)
./phase4   # Deadlock resolution (lock ordering)
