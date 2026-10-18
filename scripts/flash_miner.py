# Custom target "Flash NerdMiner": writes the NerdMiner release to an ESP32 miner connected by USB
import hashlib
import os
import subprocess
import sys
import urllib.request
from os.path import isfile, join

Import("env")

VERSION = "1.8.3"
URL = ("https://github.com/BitMaker-hub/NerdMiner_v2/releases/download/"
       "nerdminer-release-V1.8.3/ESP32-devKitv1_factory.bin")
SHA256 = "fe5f7a612eb096bdd7dea65e9977498bd4de13fa934eafd8bf1737ed5f903d60"


def sha256(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


# downloads the image once and keeps it only when the checksum matches
def get_image():
    image = join(env.subst("$PROJECT_DIR"), ".pio", "nerdminer", f"NerdMiner-{VERSION}-ESP32-devKitv1_factory.bin")
    if isfile(image) and sha256(image) == SHA256:
        return image

    os.makedirs(os.path.dirname(image), exist_ok=True)
    print(f"Downloading NerdMiner {VERSION} from {URL}")
    urllib.request.urlretrieve(URL, image + ".part")
    if sha256(image + ".part") != SHA256:
        os.remove(image + ".part")
        return None
    os.replace(image + ".part", image)
    return image


def flash_miner(target, source, env):
    image = get_image()
    if not image:
        sys.stderr.write("Error: the downloaded image does not match the expected SHA-256, nothing was flashed\n")
        return 1

    esptool = join(env.PioPlatform().get_package_dir("tool-esptoolpy"), "esptool.py")
    cmd = [env.subst("$PYTHONEXE"), esptool, "--chip", "esp32", "--baud", env.subst("$UPLOAD_SPEED")]
    if env.subst("$UPLOAD_PORT"):
        cmd += ["--port", env.subst("$UPLOAD_PORT")]
    # the factory image holds bootloader, partition table and firmware, written from address 0
    cmd += ["write_flash", "0x0", image]
    return subprocess.call(cmd)


env.AddCustomTarget(
    name="flash_miner",
    dependencies=None,
    actions=[flash_miner],
    title="Flash NerdMiner",
    description=f"Write NerdMiner {VERSION} to an ESP32 miner connected by USB",
    always_build=True,
)
