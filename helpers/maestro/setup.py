from setuptools import setup, find_packages

setup(
    name="maestro",
    version="0.2.0",
    packages=find_packages(),
    install_requires=[
        "ultralytics>=8.1.0",
        "huggingface-hub>=0.20.3",
        "Pillow>=10.0.0",
        "numpy>=1.24.0",
        "pywin32>=306",
        "opencv-python>=4.8.0",
        "pynput>=1.7.6",
        "keyboard>=0.13.5",
        "tqdm>=4.66.0",
        "colorama>=0.4.6"
    ],
    entry_points={
        "console_scripts": [
            "maestro=maestro.cli.main:main",
        ],
    },
) 