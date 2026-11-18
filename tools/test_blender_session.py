import os
import sys
import time
import subprocess
import shutil

SDCARD = os.path.join(os.environ.get("APPDATA", ""), "Ryujinx", "sdcard")
SD = os.path.join(SDCARD, "NexvynOS")
AUTORUN = os.path.join(SD, "autorun.txt")
AUTOSHOTS = os.path.join(SD, "autoshots")

RYUJINX = os.path.join(os.path.expanduser("~"), "OneDrive", "Masaüstü", "ryujinx", "Ryujinx.exe")
NRO = r"C:\Users\soned\Documents\antigravityProjects\NintendoSwitchProjects\SnxOS-NX\NexvynOS.nro"
ARTIFACT_DIR = r"C:\Users\soned\.gemini\antigravity\brain\adf317fd-ab7b-4a92-b277-ef3a58aae1af"

def run_test():
    names = ["store_1", "store_2", "store_3", "store_4", "store_5"]
    shots = [os.path.join(AUTOSHOTS, f"{n}.png") for n in names]

    for s in shots:
        if os.path.isfile(s):
            try:
                os.remove(s)
            except Exception:
                pass

    autorun_script = (
        "wait 3\n"
        "unlock\n"
        "wait 2\n"
        "trust com.nexvyn.blender\n"
        "wait 1\n"
        "launch com.nexvyn.blender\n"
        "wait 5\n"
        "click 400 420\n"
        "wait 2.5\n"
        "shot store_1\n"
        "wait 1\n"
        "click 468 14\n"
        "wait 2.5\n"
        "shot store_2\n"
        "wait 1\n"
        "click 603 14\n"
        "wait 2.5\n"
        "shot store_3\n"
        "wait 1\n"
        "click 729 14\n"
        "wait 2.5\n"
        "shot store_4\n"
        "wait 1\n"
        "click 785 14\n"
        "wait 2.5\n"
        "shot store_5\n"
        "wait 1.5\n"
        "exit\n"
    )
    with open(AUTORUN, "w", encoding="utf-8") as f:
        f.write(autorun_script)
    print("Prepared autorun.txt for 5 Store Showcase screenshots")

    si = subprocess.STARTUPINFO()
    si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    si.wShowWindow = 7

    proc = subprocess.Popen([RYUJINX, NRO], startupinfo=si, cwd=os.path.dirname(RYUJINX))
    print(f"Ryujinx running with PID {proc.pid}")

    t0 = time.time()
    max_wait = 140
    try:
        while time.time() - t0 < max_wait:
            time.sleep(2)
            if proc.poll() is not None:
                break
            if all(os.path.isfile(s) for s in shots):
                print("All 5 Store Showcase screenshots captured!")
                time.sleep(3)
                break
    finally:
        if proc.poll() is None:
            subprocess.run(["taskkill", "/F", "/PID", str(proc.pid), "/T"], capture_output=True)

    if os.path.isfile(AUTORUN):
        os.remove(AUTORUN)

    dest_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "screenshots", "store")
    os.makedirs(dest_dir, exist_ok=True)
    for idx, s in enumerate(shots, 1):
        if os.path.isfile(s):
            dst = os.path.join(dest_dir, f"{idx}.png")
            shutil.copy2(s, dst)
            shutil.copy2(s, os.path.join(ARTIFACT_DIR, f"store_{idx}.png"))
            print(f"{idx}.png: EXISTS ({os.path.getsize(s)} bytes)")
        else:
            print(f"{idx}.png: MISSING")

if __name__ == '__main__':
    run_test()
