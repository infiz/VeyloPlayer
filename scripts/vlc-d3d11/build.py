"""Build the patched VLC 3.0.24 Windows output plugin with MinGW-w64.

Run under Ubuntu 24.04 / WSL with g++-mingw-w64-x86-64-posix installed.
Only the output plugin is rebuilt; the verified VLC core and codecs are retained.
"""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile


VERSION = "3.0.24"
SOURCE_SHA256 = "e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf"
RECIPE = Path(__file__).resolve().parent
SOURCES = [
    "video_output/win32/" + name
    for name in (
        "direct3d11.c", "d3d11_quad.c", "d3d11_shaders.c", "common.c",
        "d3d11_scaler.cpp", "d3d11_tonemap.cpp", "events.c", "sensors.cpp",
        "win32touch.c",
    )
] + ["video_chroma/" + name for name in ("copy.c", "d3d11_fmt.c", "dxgi_fmt.c")]


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if sha256(args.archive) != SOURCE_SHA256:
        raise SystemExit("VLC source archive checksum mismatch")
    for tool in ("x86_64-w64-mingw32-gcc", "x86_64-w64-mingw32-g++",
                 "x86_64-w64-mingw32-dlltool", "x86_64-w64-mingw32-strip",
                 "x86_64-w64-mingw32-windres", "patch"):
        if not shutil.which(tool):
            raise SystemExit("Install g++-mingw-w64-x86-64-posix and patch in WSL first")

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    # Extract only the verified files needed by this module. Re-extraction also
    # restores pristine inputs before applying the patch on subsequent builds.
    prefixes = (f"vlc-{VERSION}/include/", f"vlc-{VERSION}/modules/video_chroma/",
                f"vlc-{VERSION}/modules/video_output/win32/")
    extra = {f"vlc-{VERSION}/modules/codec/avcodec/va_surface.h",
             f"vlc-{VERSION}/src/libvlccore.sym"}
    with tarfile.open(args.archive) as archive:
        members = [m for m in archive.getmembers()
                   if m.isfile() and (m.name.startswith(prefixes) or m.name in extra)]
        archive.extractall(output / "source", members=members, filter="data")
    source = output / "source" / f"vlc-{VERSION}"
    shutil.copyfile(RECIPE / "config.h", output / "config.h")
    with (output / "build.log").open("w") as log:
        def run(command, cwd=None):
            result = subprocess.run(command, cwd=cwd, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            log.write(result.stdout)
            log.flush()
            if result.returncode:
                raise SystemExit(result.stdout)
            return result.stdout

        run(["patch", "--batch", "--fuzz=0", "-p1", "-i", str(RECIPE / "planar-422.patch")], source)
        objects = []
        for relative in SOURCES:
            path = source / "modules" / relative
            obj = output / (path.stem + ".o")
            compiler = "x86_64-w64-mingw32-" + ("g++" if path.suffix == ".cpp" else "gcc")
            flags = [] if path.suffix == ".cpp" else ["-Werror=implicit-function-declaration"]
            run([compiler, "-c", str(path), "-o", str(obj), "-O2", "-DHAVE_CONFIG_H",
                 "-fno-strict-aliasing", "-Wno-deprecated-declarations",
                 f"-ffile-prefix-map={source}=vlc-{VERSION}",
                 "-I" + str(output), "-I" + str(source / "include"),
                 "-I" + str(source / "modules/video_output"), *flags])
            objects.append(str(obj))

        definition = output / "libvlccore.def"
        definition.write_text("LIBRARY libvlccore.dll\nEXPORTS\n"
                              + (source / "src/libvlccore.sym").read_text())
        imports = output / "libvlccore.a"
        run(["x86_64-w64-mingw32-dlltool", "-d", str(definition), "-l", str(imports)])
        resource = output / "plugin-resource.o"
        run(["x86_64-w64-mingw32-windres", str(RECIPE / "plugin.rc"), str(resource)])
        objects.append(str(resource))
        plugin = output / "libdirect3d11_plugin.dll"
        run(["x86_64-w64-mingw32-g++", "-shared", "-static", "-Wl,--no-insert-timestamp",
             "-Wl,--exclude-all-symbols", "-o", str(plugin), *objects, str(imports),
             "-lole32", "-luuid", "-lgdi32", "-luser32", "-lpropsys", "-ladvapi32"])
        run(["x86_64-w64-mingw32-strip", "--strip-unneeded", str(plugin)])
        compiler_version = run(["x86_64-w64-mingw32-g++", "--version"]).splitlines()[0]

    # Retain the build-tool runtime notices alongside the modified plugin.
    for package in ("gcc-mingw-w64-base", "mingw-w64-common"):
        shutil.copyfile(Path("/usr/share/doc") / package / "copyright",
                        output / (package + "-copyright.txt"))
    manifest = {
        "vlc_version": VERSION,
        "source_sha256": SOURCE_SHA256,
        "inputs": {name: sha256(RECIPE / name)
                   for name in ("build.py", "config.h", "planar-422.patch", "plugin.rc")},
        "compiler": compiler_version,
        "plugin_sha256": sha256(plugin),
    }
    (output / "veylo-d3d11-build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Built Direct3D 11 plugin with planar 10-bit 4:2:2 support", flush=True)


if __name__ == "__main__":
    main()
