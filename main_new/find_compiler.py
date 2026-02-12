
import os

def find_stdio():
    search_paths = [
        r"C:\Keil_v5\ARM\ARMCC\include",
        r"C:\Keil_v5\ARM\ARMCLANG\include",
        r"C:\Program Files (x86)\GNU Arm Embedded Toolchain",
        r"C:\Program Files\GNU Arm Embedded Toolchain",
        r"C:\Ac6\SystemWorkbench\plugins"
    ]
    
    found_paths = []
    
    for base_path in search_paths:
        if os.path.exists(base_path):
            for root, dirs, files in os.walk(base_path):
                if 'stdio.h' in files:
                    found_paths.append(root)
                    # Don't go too deep or find too many
                    if len(found_paths) > 5:
                        return found_paths
    return found_paths

if __name__ == "__main__":
    paths = find_stdio()
    for p in paths:
        print(p)
