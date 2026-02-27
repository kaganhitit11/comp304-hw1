#!/usr/bin/env python3
import os
import sys
import shutil

BLUE = "\033[94m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
RESET = "\033[0m"
BOLD = "\033[1m"

def get_size(path):
    total = 0
    try:
        if os.path.isfile(path): 
            return os.path.getsize(path)
        for dirpath, _, filenames in os.walk(path):
            for f in filenames:
                fp = os.path.join(dirpath, f)
                if not os.path.islink(fp): 
                    total += os.path.getsize(fp)
    except: 
        return 0
    return total

def run(target="."):
    if not os.path.exists(target):
        print(f"Path '{target}' not found.")
        return

    term_width = shutil.get_terminal_size((80, 20)).columns
    max_bar_width = term_width - 35 

    data = []
    try:
        for entry in os.listdir(target):
            path = os.path.join(target, entry)
            size_mb = get_size(path) / (1024 * 1024)
            data.append((entry, size_mb))
    except Exception as e:
        print(f"Error: {e}"); return

    data.sort(key=lambda x: x[1], reverse=True)
    data = data[:15]

    if not data:
        print("Empty.")
        return

    max_size = max(item[1] for item in data)
    
    print(f"\n{BOLD}{BLUE}🥤 SODA: Analyzing {os.path.abspath(target)}{RESET}\n")

    for name, size in data:
        bar_len = int((size / max_size) * max_bar_width) if max_size > 0 else 0
        bar = "█" * bar_len
        
        color = GREEN if size < 50 else (YELLOW if size < 500 else RED)
        
        print(f"{name[:15]:<16} | {color}{bar}{RESET} {size:>8.2f} MB")
    
    print(f"\n{BOLD}{BLUE}Sorting: Largest to Smallest{RESET}\n")

if __name__ == "__main__":
    run(sys.argv[1] if len(sys.argv) > 1 else ".")