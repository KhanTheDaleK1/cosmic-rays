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

def write_to_header(version_str, filepath="source/Version.h"):
    """
    Writes the generated version string and plugin definitions to a C++ header file.
    """
    # Ensure directory exists
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    
    header_content = f"""// AUTO-GENERATED VERSION FILE
// Do not edit manually.
#pragma once

#ifndef PROJECT_VERSION_STRING
#define PROJECT_VERSION_STRING "{version_str}"
#endif

// Official JUCE Plugin Metadata
#ifndef JucePlugin_Name
#define JucePlugin_Name "Cosmic Rays"
#endif

#ifndef JucePlugin_VersionString
#define JucePlugin_VersionString "{version_str}"
#endif

#ifndef JucePlugin_Manufacturer
#define JucePlugin_Manufacturer "Beechem"
#endif
"""
    with open(filepath, "w") as f:
        f.write(header_content)
    print(f"Successfully wrote version {version_str} and plugin metadata to {filepath}")

def update_version_txt(version_str, filepath="version.txt"):
    """
    Updates the version.txt file with the new version string.
    """
    with open(filepath, "w") as f:
        f.write(version_str)
    print(f"Successfully updated {filepath} to {version_str}")

def update_docs_badge(version_str, filepath="docs/index.html"):
    """
    Updates the hardcoded version badge in the documentation index.html.
    """
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found. Skipping badge update.")
        return

    import re
    with open(filepath, "r") as f:
        content = f.read()

    # Pattern to match the badge div with any version string
    pattern = r'(<div class="badge" id="release-badge">BETA v).*?( AVAILABLE NOW</div>)'
    replacement = r'\g<1>' + version_str + r'\g<2>'
    
    new_content = re.sub(pattern, replacement, content)
    
    if new_content != content:
        with open(filepath, "w") as f:
            f.write(new_content)
        print(f"Successfully updated badge in {filepath} to v{version_str}")
    else:
        print(f"Note: No changes needed or pattern not found in {filepath}")

if __name__ == "__main__":
    current_version = generate_version_string()
    print(f"Generated Version: {current_version}")
    
    # Update Version.h
    target_path = sys.argv[1] if len(sys.argv) > 1 else "source/Version.h"
    write_to_header(current_version, target_path)
    
    # Update version.txt
    update_version_txt(current_version)

    # Update docs badge
    update_docs_badge(current_version)
