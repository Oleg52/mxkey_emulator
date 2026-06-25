# MXKEY SmartCard Emulator

This project emulates MXKEY smartcard for mobileEx software.

Since server is long dead and smartcards have counter(when it reaches 0 software can't be used), emulation is the only fix for it. 

## Build Environment

This project is built using Microsoft Visual Studio 6.0 (VS6).

Emulator project compiles into DLL. It should be named "winmm.dll" for app to load it automatically on startup.

Rpl generator uses emulator dll for older app version to generate and parse rpl for DCT4 phones.

Save helper is needed to create separate process to display file save/open dialog and get file name. 
Obfusucator may have corrupted something, because dialog crashes when used from app process.

## Acknowledgements

Portions of the reverse-engineering analysis, particularly the identification and understanding of hashing functions, were assisted by AI tools.