from setuptools import setup, find_packages

setup(
    name="ui_ctrl_v2",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "ultralytics>=8.1.0",
        "huggingface-hub>=0.20.3",
        "Pillow>=10.0.0",
        "numpy>=1.24.0",
        "pywin32>=306",
        "opencv-python>=4.8.0"
    ],
    entry_points={
        'console_scripts': [
            'ui-ctrl=ui_ctrl_v2.cli:main',
        ],
    },
) 