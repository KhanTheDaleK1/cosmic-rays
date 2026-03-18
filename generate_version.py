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
        # On Windows, this requires the 'tzdata' pip package
        central_tz = zoneinfo.ZoneInfo("US/Central")
    except zoneinfo.ZoneInfoNotFoundError:
        # Fallback: Manual offset for US Central (CST is UTC-6, CDT is UTC-5)
        # We try to detect if we're in DST, but a fixed -6 is safer than failing.
        print("Warning: 'US/Central' not found in system database. Falling back to UTC-6.")
        central_tz = datetime.timezone(datetime.timedelta(hours=-6))

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

def update_version_txt(version_str, filepath="version.txt"):
    """
    Updates the version.txt file with the new version string.
    """
    with open(filepath, "w") as f:
        f.write(version_str)
    print(f"Successfully updated {filepath} to {version_str}")

if __name__ == "__main__":
    current_version = generate_version_string()
    print(f"Generated Version: {current_version}")
    
    # Update Version.h
    target_path = sys.argv[1] if len(sys.argv) > 1 else "Source/Version.h"
    write_to_header(current_version, target_path)
    
    # Update version.txt
    update_version_txt(current_version)
