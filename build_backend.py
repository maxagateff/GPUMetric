from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

from setuptools import build_meta as _build_meta


ROOT_DIR = Path(__file__).resolve().parent
BUILD_DIR = ROOT_DIR / "build" / "cmake"
PACKAGE_DIR = ROOT_DIR / "src" / "gpumetric"
LIB_DIR = PACKAGE_DIR / "lib"


def _run_cmake() -> None:
    """
    Configure and build the native GPUMetric library using CMake.
    """

    cmake = shutil.which("cmake")

    if cmake is None:
        raise RuntimeError(
            "CMake was not found. "
            "Please install CMake before building GPUMetric."
        )

    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    LIB_DIR.mkdir(parents=True, exist_ok=True)

    configure_command = [
        cmake,
        "-S",
        str(ROOT_DIR),
        "-B",
        str(BUILD_DIR),
        "-DCMAKE_BUILD_TYPE=Release",
    ]

    subprocess.run(
        configure_command,
        check=True,
    )

    build_command = [
        cmake,
        "--build",
        str(BUILD_DIR),
        "--config",
        "Release",
    ]

    subprocess.run(
        build_command,
        check=True,
    )

    _copy_native_library()


def _copy_native_library() -> None:
    """
    Copy the CMake-built shared library into the Python package.
    """

    candidates = [
        BUILD_DIR / "libgpumetric.so",
        BUILD_DIR / "lib" / "libgpumetric.so",
        BUILD_DIR / "Release" / "libgpumetric.so",
        ]

    source = next(
        (path for path in candidates if path.is_file()),
        None,
    )

    if source is None:
        raise RuntimeError(
            "CMake completed successfully, but "
            "libgpumetric.so was not found."
        )

    destination = LIB_DIR / "libgpumetric.so"

    shutil.copy2(
        source,
        destination,
    )

    print(
        f"GPUMetric native library copied to: {destination}"
    )


def build_wheel(
        wheel_directory,
        config_settings=None,
        metadata_directory=None,
):
    _run_cmake()

    return _build_meta.build_wheel(
        wheel_directory,
        config_settings,
        metadata_directory,
    )


def build_editable(
        wheel_directory,
        config_settings=None,
        metadata_directory=None,
):
    _run_cmake()

    return _build_meta.build_editable(
        wheel_directory,
        config_settings,
        metadata_directory,
    )


def build_sdist(
        sdist_directory,
        config_settings=None,
):
    return _build_meta.build_sdist(
        sdist_directory,
        config_settings,
    )


def prepare_metadata_for_build_wheel(
        metadata_directory,
        config_settings=None,
):
    return _build_meta.prepare_metadata_for_build_wheel(
        metadata_directory,
        config_settings,
    )


def prepare_metadata_for_build_editable(
        metadata_directory,
        config_settings=None,
):
    return _build_meta.prepare_metadata_for_build_editable(
        metadata_directory,
        config_settings,
    )


def get_requires_for_build_wheel(config_settings=None):
    return _build_meta.get_requires_for_build_wheel(
        config_settings
    )


def get_requires_for_build_editable(config_settings=None):
    return _build_meta.get_requires_for_build_editable(
        config_settings
    )


def get_requires_for_build_sdist(config_settings=None):
    return _build_meta.get_requires_for_build_sdist(
        config_settings
    )