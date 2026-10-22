# MXKEY SmartCard Emulator

This project emulates MXKEY smartcard for mobileEx software.

Since server is long dead and smartcards have counter(when it reaches 0 software can't be used), emulation is the only fix for it. 

MXKEY application is available on [GDrive](https://drive.google.com/file/d/11gC7HQBfWaNcIQaWZQ8v7YvMYowpTJZk/view) (password 1)

## Build Environment

This project is built using Microsoft Visual Studio 6.0 (VS6).

Emulator project compiles into DLL. It should be named "winmm.dll" for app to load it automatically on startup.

Rpl generator uses emulator dll for older app version to generate and parse rpl for DCT4 phones.

## Acknowledgements

Portions of the reverse-engineering analysis, particularly the identification and understanding of hashing functions, were assisted by AI tools.