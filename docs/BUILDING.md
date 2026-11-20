# Build Septet from source

The root `machine-bundle.json` is TE Builder’s build recipe. It points to `septet/`, pins the Open-XY commit and lists the required SDK patches in order with SHA-256 digests. TE Builder 0.5.7 can build directly from this checkout via **File → Import Git Repository…**.

## Standalone Open-XY build

Use Git, a Python version supported by the pinned Open-XY SDK, an Open-XY Blackfin toolchain for your host, and your own compatible OS 1.2.1 `os.ldr`. Keep the firmware and generated device files private.

Run these commands from the Septet checkout. Use a fresh SDK directory for this build.

```sh
septet_dir="$PWD"
sdk_dir="$septet_dir/../open-xy-septet-sdk"
git clone https://github.com/etcetc000/open-xy.git "$sdk_dir"
git -C "$sdk_dir" checkout --detach 28dd1a2e36599108c5eecf1a03f3b28601e50b39

python3 - "$sdk_dir" <<'PY'
import hashlib
import json
from pathlib import Path
import subprocess
import sys

sdk = Path(sys.argv[1])
manifest = json.loads(Path("machine-bundle.json").read_text())
for patch in manifest["patches"]:
    path = Path(patch["path"]).resolve()
    if hashlib.sha256(path.read_bytes()).hexdigest() != patch["sha256"]:
        raise SystemExit(f"Patch checksum failed for {path.name}")
    subprocess.run(["git", "-C", str(sdk), "apply", "--whitespace=nowarn", str(path)], check=True)
PY
```

From the patched SDK directory, fetch the pinned toolchain and build. Set `stock_os` to the absolute path of your local firmware image.

```sh
stock_os="/absolute/path/to/os.ldr"
cd "$sdk_dir"
python3 sdk.py toolchain fetch
python3 sdk.py build --machine "$septet_dir/septet" --stock-os "$stock_os" \
  --profiling off --out "$septet_dir/../septet-build-0.1.0"
```

If you already have a supported toolchain, pass its bundle directory with `--compiler` and its `bin` directory with `--toolchain` to the build command. Consult [Open-XY’s pinned toolchain documentation](https://github.com/etcetc000/open-xy/blob/28dd1a2e36599108c5eecf1a03f3b28601e50b39/docs/toolchain.md) for host support.

## Verify and package

From the Septet checkout, with `sdk_dir` set as above, run the host DSP and graph tests and check the release ZIP.

```sh
c++ -std=c++11 -O2 -Iseptet -I"$sdk_dir/sdk/include" tests/host.cpp -o /tmp/septet-host
/tmp/septet-host
python3 tools/package_manager.py --check
```

Run `python3 tools/package_manager.py` to regenerate the ZIP. The scripts in `tools/` also regenerate the checked-in coefficient, response and display assets.
