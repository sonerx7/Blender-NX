import os
import shutil
import subprocess

def deploy():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(root, "build")
    bundle_dir = os.path.join(build_dir, "bundle", "nexvyn-blender.nxvapp")

    src_mod = os.path.join(build_dir, "nexvyn-blender.nxvmod")
    dst_mod = os.path.join(bundle_dir, "nexvyn-blender.nxvmod")
    shutil.copy2(src_mod, dst_mod)

    src_json = os.path.join(root, "app.json")
    dst_json = os.path.join(bundle_dir, "app.json")
    shutil.copy2(src_json, dst_json)

    src_icon = os.path.join(root, "icon.png")
    dst_icon = os.path.join(bundle_dir, "icon.png")
    shutil.copy2(src_icon, dst_icon)

    pack_tool = r"C:\Users\soned\Documents\antigravityProjects\NintendoSwitchProjects\Nexvyn-OS-NX\SDK\Languages\c\native-kit\tools\nxvapp_pack.py"
    pkg_out = os.path.join(build_dir, "nexvyn-blender.nxvapp")
    cmd = [ "python", pack_tool, bundle_dir, "-o", pkg_out ]
    subprocess.run(cmd, check=True)

    appdata = os.environ.get("APPDATA", "")
    target_app_dir = os.path.join(appdata, "Ryujinx", "sdcard", "NexvynOS", "Applications", "com.nexvyn.blender.nxvapp")
    os.makedirs(target_app_dir, exist_ok=True)

    sd_mod = os.path.join(target_app_dir, "nexvyn-blender.nxvmod")
    shutil.copy2(src_mod, sd_mod)

    sd_pkg = os.path.join(target_app_dir, "nexvyn-blender.nxvapp")
    shutil.copy2(pkg_out, sd_pkg)

    print(f"Successfully packaged and deployed to: {target_app_dir}")
    print(f"Mod size: {os.path.getsize(sd_mod)} bytes")
    print(f"Pkg size: {os.path.getsize(sd_pkg)} bytes")

if __name__ == '__main__':
    deploy()
