# Awake for Windows

A lightweight system tray application that keeps your PC awake by sending the F15 key every minute.

## Features

- System tray icon with right-click menu
- **Color-coded icon**: Green when active, gray when inactive
- Toggle active/inactive mode manually
- **Timer modes**: 15min, 30min, 45min, 1hr, 2hr, 4hr, 6hr, 8hr, 12hr, 16hr, 20hr, 24hr
- **Custom timer**: Enter any duration from 1 to 9000 hours
- **Inactive timer**: Keep program inactive for a specified time, then auto-activate
- Shows remaining time in tooltip when using timer
- **Double-click** tray icon to quickly toggle active/inactive
- No external dependencies
- Compiled with TCC (Tiny C Compiler)

## Compilation

### Requirements
- TCC (Tiny C Compiler) - Download from: https://bellard.org/tcc/

### Steps
1. Place `awake.c` in a folder
2. Run `compile.bat` (or use the command below)

Manual compilation:
```
tcc -o awake.exe awake.c -luser32 -lgdi32 -lshell32
```

## Usage

1. Run `awake.exe`
2. A system tray icon will appear (gray square = inactive, green square = active)
3. **Double-click** the tray icon to toggle active/inactive
4. **Right-click** the tray icon to access the menu:
   - **About**: Program information
   - **Active for**: Set a timer to keep PC awake for a specific duration
     - Preset options: 15min to 24hr
     - **Custom...**: Enter hours (1-9000)
   - **Inactive for**: Stay inactive for a duration, then auto-activate
     - Preset options: 15min to 24hr
     - **Custom...**: Enter hours (1-9000)
   - **Show Dialog**: Display current status
   - **Active**: Toggle with checkmark indicator
   - **Exit**: Close the application

## How It Works

When active, the program sends the F15 keyboard key every 60 seconds. This key is rarely used by applications and prevents Windows from entering sleep mode or activating the screensaver.

## Notes

- The F15 key is chosen because it's unlikely to interfere with other applications
- The tray icon color changes: **green** = active, **gray** = inactive
- The system tray tooltip shows the current status (Active/Inactive) and remaining time if a timer is set
- Timer mode automatically deactivates when time expires
- Custom time input accepts hours (1-9000), equivalent to 60 minutes to 540,000 minutes
