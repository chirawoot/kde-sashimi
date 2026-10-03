# kde-sashimi

A fast, lightweight native C++/Qt6 file preview utility designed for KDE Plasma (optimized for Dolphin on openSUSE Tumbleweed).

## Features
- **Lightning Fast Preview:** Supports Images, PDFs, Videos, Audio, Text, and Office documents (via Headless LibreOffice-to-PDF conversion with MD5 caching).
- **Multi-modal Navigation:** Use UI Buttons, Arrow Keys (`Left`/`Right`/`Up`/`Down`), or shortcuts (`Space`/`Esc`).
- **Dolphin Sync:** Automatically updates the file selection/highlighting in the background Dolphin file manager via DBus (`org.freedesktop.FileManager1`).
- **Single-Instance Architecture:** Uses `QLocalServer`/`QLocalSocket` to instantly switch files in the existing preview window without spawning multiple processes.
- **Quick Action:** Open the previewed file directly with its default system application using the `Enter` key.

---

## Prerequisites

Make sure you have Qt6 development packages and CMake installed on your system (e.g., openSUSE Tumbleweed, Arch Linux, or Fedora):

```bash
# openSUSE Tumbleweed example
sudo zypper install cmake gcc-c++ qt6-base-devel qt6-multimedia-devel qt6-pdf-devel qt6-dbus-devel libreoffice

Building and Installation
Clone the repository and compile using CMake:

git clone [https://github.com/your-username/kde-sashimi.git](https://github.com/your-username/kde-sashimi.git)
cd kde-sashimi

# Configure and Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install to system (e.g., /usr/local/bin)
sudo cmake --install build

Usage
You can launch it directly from the terminal:

kde-sashimi /path/to/image.png

Integrating with KDE Dolphin:
Open Dolphin Settings -> General -> Services (or use Custom Actions).

Create a new Service/Action to run kde-sashimi %f when a file is selected or triggered.
