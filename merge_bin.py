Import("env")
import os
import subprocess

def merge_bin_after_build(source, target, env):
    print("Creating merged binary for web flasher...")
    
    build_dir = env.subst("$BUILD_DIR")
    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin") 
    firmware = os.path.join(build_dir, "firmware.bin")
    merged = os.path.join(build_dir, "firmware_merged.bin")
    
    # Check if all files exist
    if not all(os.path.exists(f) for f in [bootloader, partitions, firmware]):
        print("ERROR: Some binary files not found, skipping merge")
        return
    
    # Use PlatformIO's esptool with corrected settings
    python_exe = env.subst("$PYTHONEXE")
    esptool_path = os.path.join(env.PioPlatform().get_package_dir("tool-esptoolpy"), "esptool.py")
    
    # Create merged binary with proper ESP32 settings
    cmd = [
        python_exe, esptool_path, "--chip", "esp32", "merge_bin", 
        "-o", merged,
        "--flash_mode", "dio", 
        "--flash_freq", "40m", 
        "--flash_size", "4MB",
        "0x1000", bootloader,    # ESP32 bootloader offset
        "0x8000", partitions,    # Partition table offset
        "0xe000", os.path.join(env.PioPlatform().get_package_dir("framework-arduinoespressif32"), "tools", "sdk", "esp32", "bin", "boot_app0.bin"),  # Boot app0
        "0x10000", firmware      # Application offset
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=build_dir)
        if result.returncode == 0 and os.path.exists(merged):
            size = os.path.getsize(merged)
            print(f"SUCCESS: Web flasher binary created: {merged} ({size} bytes)")
        else:
            print(f"Warning: Could not create complete merged binary, using simple version")
            # Fallback to simple merge
            simple_cmd = [
                python_exe, esptool_path, "--chip", "esp32", "merge_bin", 
                "-o", merged, "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB",
                "0x1000", bootloader, "0x8000", partitions, "0x10000", firmware
            ]
            subprocess.run(simple_cmd)
            
    except Exception as e:
        print(f"Exception running esptool: {e}")

env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin_after_build)