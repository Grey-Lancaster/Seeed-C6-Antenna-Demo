Import("env")
import os

def patch_improv_library(source, target, env):
    # Find the library path
    lib_path = None
    for path in env.get("LIBSOURCE_DIRS"):
        improv_path = os.path.join(path, "Improv WiFi Library", "src", "ImprovWiFiLibrary.cpp")
        if os.path.exists(improv_path):
            lib_path = improv_path
            break
    
    if not lib_path:
        print("Could not find Improv WiFi Library to patch")
        return
    
    print(f"Patching Improv WiFi Library at: {lib_path}")
    
    # Read the file
    with open(lib_path, 'r') as f:
        content = f.read()
    
    # Apply patches for ESP32-C6
    content = content.replace(
        'WiFi.begin(ssid, password);',
        'WiFi.begin((char*)ssid, password);'
    )
    
    content = content.replace(
        'WiFi.SSID(id).c_str(),',
        'WiFi.SSID(id),'
    )
    
    # Write back
    with open(lib_path, 'w') as f:
        f.write(content)
    
    print("Library patched for ESP32-C6 compatibility")

env.AddPreAction("buildprog", patch_improv_library)