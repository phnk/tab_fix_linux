# tab_fix_linux
Linux port of [tab_fix](https://github.com/phnk/tab_fix) - changing the functionality of Alt+Tab to custom keyboard binds for window switching. This is an early version and there are bugs.

## Known Bugs
Currently none. Please report issues you find in issues.

## Overview
`tab_fix_linux` provides an alternative to the traditional Alt+Tab window switching behavior on Linux systems. Instead of cycling through windows in a stack, this tool allows you to bind specific windows to keyboard shortcuts for instant access.

## Dependencies
This project requires [gse-window-api](https://github.com/phnk/gse-window-api), a GNOME Shell extension that provides window management capabilities similar to the Windows API, allowing you to:
- List currently open windows
- Activate specific windows by index

## Installation
### Prerequisites
1. qt6
2. qt6-tools
3. gdm
4. gnome

5. Install the gse-window-api GNOME Shell extension:
```bash
# Clone the gse-window-api repository
git clone git@github.com:phnk/gse-window-api.git

# Follow the installation instructions in the gse-window-api README
```

6. Clone this repository:
```bash
git clone git@github.com:phnk/tab_fix_linux.git
cd tab_fix_linux
```

7. Install any additional dependencies as specified in the project files.

8. Build and run
```bash
make
QT_QPA_PLATFORM=xcb ./bin/tabfix_client
```

## Usage
1. Start the program.
2. Bind a global hotkey in Settings -> Keyboard -> Keyboard shortcuts -> Custom Shortcut
3. Command: `qdbus6 org.phnk.TabFixHotkey /org/phnk/TabFixHotkey ShowWindow`

## Autostart
Can autostart by adding the file `tabfix.desktop` to your autostart directory with the entry:
```
[Desktop Entry]
Type=Application
Name=TabFix Client
Exec=env QT_QPA_PLATFORM=xcb /path/to/tabfix_client
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
```

## How It Works
Unlike the traditional Alt+Tab behavior which cycles through windows based on recent usage, `tab_fix_linux` uses the gse-window-api to:
1. Query the list of open windows
2. Map specific windows to keyboard shortcuts
3. Activate windows directly via their index

## Differences from Windows Version
The Linux port relies on the GNOME Shell Extension ecosystem rather than native Windows APIs, which means:
- Requires GNOME desktop environment
- Uses gse-window-api as an intermediary for window management
- May have different configuration options compared to the Windows version

## Contributing
Please feel free to submit issues or pull requests.

## Platform Support
This tool has only been tested on:
- **OS**: Arch
- **Display Server**: gdm and Gnome
- **QT version**: Qt6

It may work on other Linux distributions and configurations, but compatibility is not guaranteed.

## Related Projects
- [tab_fix](https://github.com/phnk/tab_fix) - Original Windows version
- [gse-window-api](https://github.com/phnk/gse-window-api) - GNOME Shell window management API

## Author
[phnk](https://github.com/phnk)
