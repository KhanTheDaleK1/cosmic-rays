import datetime
import zoneinfo
import sys
import os

def generate_version_string():
    """
    Generates a version string in the format dd.mm.yyyy-tttt
    locked to US Central Time.
    """
    try:
        # Set timezone strictly to US Central Time
        central_tz = zoneinfo.ZoneInfo("US/Central")
    except zoneinfo.ZoneInfoNotFoundError:
        # Fallback for systems without zoneinfo database (like some Windows/CI envs)
        # Using a fixed offset if possible or just failing gracefully
        print("Error: Timezone 'US/Central' not found. Ensure you are using Python 3.9+.")
        sys.exit(1)

    # Get the current time in the specified timezone
    now_central = datetime.datetime.now(central_tz)

    # Format the time: dd.mm.yyyy-tttt
    version_str = now_central.strftime("%d.%m.%Y-%H%M")
    
    return version_str

def write_to_header(version_str, filepath="Source/Version.h"):
    """
    Writes the generated version string to a C++ header file.
    """
    # Ensure directory exists
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    
    header_content = f"""// AUTO-GENERATED VERSION FILE
// Do not edit manually.
#pragma once

#ifndef PROJECT_VERSION_STRING
#define PROJECT_VERSION_STRING "{version_str}"
#endif
"""
    with open(filepath, "w") as f:
        f.write(header_content)
    print(f"Successfully wrote version {version_str} to {filepath}")

if __name__ == "__main__":
    current_version = generate_version_string()
    print(f"Generated Version: {current_version}")
    
    # Get filepath from command line if provided
    target_path = sys.argv[1] if len(sys.argv) > 1 else "Source/Version.h"
    write_to_header(current_version, target_path)
