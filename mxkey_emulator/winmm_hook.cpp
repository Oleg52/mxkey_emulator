#include <windows.h>
#include "winmm_hook.h"

struct winmm_dll {
	HMODULE dll;
	FARPROC oCloseDriver;
	FARPROC oDefDriverProc;
	FARPROC oDriverCallback;
	FARPROC oDrvGetModuleHandle;
	FARPROC oGetDriverModuleHandle;
	FARPROC oMigrateAllDrivers;
	FARPROC oMigrateSoundEvents;
	FARPROC oNotifyCallbackData;
	FARPROC oOpenDriver;
	FARPROC oPlaySound;
	FARPROC oPlaySoundA;
	FARPROC oPlaySoundW;
	FARPROC oSendDriverMessage;
	FARPROC oWOW32DriverCallback;
	FARPROC oWOW32ResolveMultiMediaHandle;
	FARPROC oWOWAppExit;
	FARPROC oWinmmLogoff;
	FARPROC oWinmmLogon;
	FARPROC o_gfxLogoff;
	FARPROC o_gfxLogon;
	FARPROC oaux32Message;
	FARPROC oauxGetDevCapsA;
	FARPROC oauxGetDevCapsW;
	FARPROC oauxGetNumDevs;
	FARPROC oauxGetVolume;
	FARPROC oauxOutMessage;
	FARPROC oauxSetVolume;
	FARPROC ogfxAddGfx;
	FARPROC ogfxBatchChange;
	FARPROC ogfxCreateGfxFactoriesList;
	FARPROC ogfxCreateZoneFactoriesList;
	FARPROC ogfxDestroyDeviceInterfaceList;
	FARPROC ogfxEnumerateGfxs;
	FARPROC ogfxModifyGfx;
	FARPROC ogfxOpenGfx;
	FARPROC ogfxRemoveGfx;
	FARPROC ojoy32Message;
	FARPROC ojoyConfigChanged;
	FARPROC ojoyGetDevCapsA;
	FARPROC ojoyGetDevCapsW;
	FARPROC ojoyGetNumDevs;
	FARPROC ojoyGetPos;
	FARPROC ojoyGetPosEx;
	FARPROC ojoyGetThreshold;
	FARPROC ojoyReleaseCapture;
	FARPROC ojoySetCapture;
	FARPROC ojoySetThreshold;
	FARPROC omci32Message;
	FARPROC omciDriverNotify;
	FARPROC omciDriverYield;
	FARPROC omciExecute;
	FARPROC omciFreeCommandResource;
	FARPROC omciGetCreatorTask;
	FARPROC omciGetDeviceIDA;
	FARPROC omciGetDeviceIDFromElementIDA;
	FARPROC omciGetDeviceIDFromElementIDW;
	FARPROC omciGetDeviceIDW;
	FARPROC omciGetDriverData;
	FARPROC omciGetErrorStringA;
	FARPROC omciGetErrorStringW;
	FARPROC omciGetYieldProc;
	FARPROC omciLoadCommandResource;
	FARPROC omciSendCommandA;
	FARPROC omciSendCommandW;
	FARPROC omciSendStringA;
	FARPROC omciSendStringW;
	FARPROC omciSetDriverData;
	FARPROC omciSetYieldProc;
	FARPROC omid32Message;
	FARPROC omidiConnect;
	FARPROC omidiDisconnect;
	FARPROC omidiInAddBuffer;
	FARPROC omidiInClose;
	FARPROC omidiInGetDevCapsA;
	FARPROC omidiInGetDevCapsW;
	FARPROC omidiInGetErrorTextA;
	FARPROC omidiInGetErrorTextW;
	FARPROC omidiInGetID;
	FARPROC omidiInGetNumDevs;
	FARPROC omidiInMessage;
	FARPROC omidiInOpen;
	FARPROC omidiInPrepareHeader;
	FARPROC omidiInReset;
	FARPROC omidiInStart;
	FARPROC omidiInStop;
	FARPROC omidiInUnprepareHeader;
	FARPROC omidiOutCacheDrumPatches;
	FARPROC omidiOutCachePatches;
	FARPROC omidiOutClose;
	FARPROC omidiOutGetDevCapsA;
	FARPROC omidiOutGetDevCapsW;
	FARPROC omidiOutGetErrorTextA;
	FARPROC omidiOutGetErrorTextW;
	FARPROC omidiOutGetID;
	FARPROC omidiOutGetNumDevs;
	FARPROC omidiOutGetVolume;
	FARPROC omidiOutLongMsg;
	FARPROC omidiOutMessage;
	FARPROC omidiOutOpen;
	FARPROC omidiOutPrepareHeader;
	FARPROC omidiOutReset;
	FARPROC omidiOutSetVolume;
	FARPROC omidiOutShortMsg;
	FARPROC omidiOutUnprepareHeader;
	FARPROC omidiStreamClose;
	FARPROC omidiStreamOpen;
	FARPROC omidiStreamOut;
	FARPROC omidiStreamPause;
	FARPROC omidiStreamPosition;
	FARPROC omidiStreamProperty;
	FARPROC omidiStreamRestart;
	FARPROC omidiStreamStop;
	FARPROC omixerClose;
	FARPROC omixerGetControlDetailsA;
	FARPROC omixerGetControlDetailsW;
	FARPROC omixerGetDevCapsA;
	FARPROC omixerGetDevCapsW;
	FARPROC omixerGetID;
	FARPROC omixerGetLineControlsA;
	FARPROC omixerGetLineControlsW;
	FARPROC omixerGetLineInfoA;
	FARPROC omixerGetLineInfoW;
	FARPROC omixerGetNumDevs;
	FARPROC omixerMessage;
	FARPROC omixerOpen;
	FARPROC omixerSetControlDetails;
	FARPROC ommDrvInstall;
	FARPROC ommGetCurrentTask;
	FARPROC ommTaskBlock;
	FARPROC ommTaskCreate;
	FARPROC ommTaskSignal;
	FARPROC ommTaskYield;
	FARPROC ommioAdvance;
	FARPROC ommioAscend;
	FARPROC ommioClose;
	FARPROC ommioCreateChunk;
	FARPROC ommioDescend;
	FARPROC ommioFlush;
	FARPROC ommioGetInfo;
	FARPROC ommioInstallIOProcA;
	FARPROC ommioInstallIOProcW;
	FARPROC ommioOpenA;
	FARPROC ommioOpenW;
	FARPROC ommioRead;
	FARPROC ommioRenameA;
	FARPROC ommioRenameW;
	FARPROC ommioSeek;
	FARPROC ommioSendMessage;
	FARPROC ommioSetBuffer;
	FARPROC ommioSetInfo;
	FARPROC ommioStringToFOURCCA;
	FARPROC ommioStringToFOURCCW;
	FARPROC ommioWrite;
	FARPROC ommsystemGetVersion;
	FARPROC omod32Message;
	FARPROC omxd32Message;
	FARPROC osndPlaySoundA;
	FARPROC osndPlaySoundW;
	FARPROC otid32Message;
	FARPROC otimeBeginPeriod;
	FARPROC otimeEndPeriod;
	FARPROC otimeGetDevCaps;
	FARPROC otimeGetSystemTime;
	FARPROC otimeGetTime;
	FARPROC otimeKillEvent;
	FARPROC otimeSetEvent;
	FARPROC owaveInAddBuffer;
	FARPROC owaveInClose;
	FARPROC owaveInGetDevCapsA;
	FARPROC owaveInGetDevCapsW;
	FARPROC owaveInGetErrorTextA;
	FARPROC owaveInGetErrorTextW;
	FARPROC owaveInGetID;
	FARPROC owaveInGetNumDevs;
	FARPROC owaveInGetPosition;
	FARPROC owaveInMessage;
	FARPROC owaveInOpen;
	FARPROC owaveInPrepareHeader;
	FARPROC owaveInReset;
	FARPROC owaveInStart;
	FARPROC owaveInStop;
	FARPROC owaveInUnprepareHeader;
	FARPROC owaveOutBreakLoop;
	FARPROC owaveOutClose;
	FARPROC owaveOutGetDevCapsA;
	FARPROC owaveOutGetDevCapsW;
	FARPROC owaveOutGetErrorTextA;
	FARPROC owaveOutGetErrorTextW;
	FARPROC owaveOutGetID;
	FARPROC owaveOutGetNumDevs;
	FARPROC owaveOutGetPitch;
	FARPROC owaveOutGetPlaybackRate;
	FARPROC owaveOutGetPosition;
	FARPROC owaveOutGetVolume;
	FARPROC owaveOutMessage;
	FARPROC owaveOutOpen;
	FARPROC owaveOutPause;
	FARPROC owaveOutPrepareHeader;
	FARPROC owaveOutReset;
	FARPROC owaveOutRestart;
	FARPROC owaveOutSetPitch;
	FARPROC owaveOutSetPlaybackRate;
	FARPROC owaveOutSetVolume;
	FARPROC owaveOutUnprepareHeader;
	FARPROC owaveOutWrite;
	FARPROC owid32Message;
	FARPROC owinmmDbgOut;
	FARPROC owinmmSetDebugLevel;
	FARPROC owod32Message;
} winmm;

extern "C" {
	void fCloseDriver() { __asm { jmp winmm.oCloseDriver } }
	void fDefDriverProc() { __asm { jmp winmm.oDefDriverProc } }
    void fDriverCallback() { __asm { jmp winmm.oDriverCallback } }
    void fDrvGetModuleHandle() { __asm { jmp winmm.oDrvGetModuleHandle } }
    void fGetDriverModuleHandle() { __asm { jmp winmm.oGetDriverModuleHandle } }
    void fMigrateAllDrivers() { __asm { jmp winmm.oMigrateAllDrivers } }
    void fMigrateSoundEvents() { __asm { jmp winmm.oMigrateSoundEvents } }
    void fNotifyCallbackData() { __asm { jmp winmm.oNotifyCallbackData } }
    void fOpenDriver() { __asm { jmp winmm.oOpenDriver } }
    void fPlaySound() { __asm { jmp winmm.oPlaySound } }
    void fPlaySoundA() { __asm { jmp winmm.oPlaySoundA } }
    void fPlaySoundW() { __asm { jmp winmm.oPlaySoundW } }
    void fSendDriverMessage() { __asm { jmp winmm.oSendDriverMessage } }
    void fWOW32DriverCallback() { __asm { jmp winmm.oWOW32DriverCallback } }
    void fWOW32ResolveMultiMediaHandle() { __asm { jmp winmm.oWOW32ResolveMultiMediaHandle } }
    void fWOWAppExit() { __asm { jmp winmm.oWOWAppExit } }
    void fWinmmLogoff() { __asm { jmp winmm.oWinmmLogoff } }
    void fWinmmLogon() { __asm { jmp winmm.oWinmmLogon } }
    void f_gfxLogoff() { __asm { jmp winmm.o_gfxLogoff } }
    void f_gfxLogon() { __asm { jmp winmm.o_gfxLogon } }
    void faux32Message() { __asm { jmp winmm.oaux32Message } }
    void fauxGetDevCapsA() { __asm { jmp winmm.oauxGetDevCapsA } }
    void fauxGetDevCapsW() { __asm { jmp winmm.oauxGetDevCapsW } }
    void fauxGetNumDevs() { __asm { jmp winmm.oauxGetNumDevs } }
    void fauxGetVolume() { __asm { jmp winmm.oauxGetVolume } }
    void fauxOutMessage() { __asm { jmp winmm.oauxOutMessage } }
    void fauxSetVolume() { __asm { jmp winmm.oauxSetVolume } }
    void fgfxAddGfx() { __asm { jmp winmm.ogfxAddGfx } }
    void fgfxBatchChange() { __asm { jmp winmm.ogfxBatchChange } }
    void fgfxCreateGfxFactoriesList() { __asm { jmp winmm.ogfxCreateGfxFactoriesList } }
    void fgfxCreateZoneFactoriesList() { __asm { jmp winmm.ogfxCreateZoneFactoriesList } }
    void fgfxDestroyDeviceInterfaceList() { __asm { jmp winmm.ogfxDestroyDeviceInterfaceList } }
    void fgfxEnumerateGfxs() { __asm { jmp winmm.ogfxEnumerateGfxs } }
    void fgfxModifyGfx() { __asm { jmp winmm.ogfxModifyGfx } }
    void fgfxOpenGfx() { __asm { jmp winmm.ogfxOpenGfx } }
    void fgfxRemoveGfx() { __asm { jmp winmm.ogfxRemoveGfx } }
    void fjoy32Message() { __asm { jmp winmm.ojoy32Message } }
    void fjoyConfigChanged() { __asm { jmp winmm.ojoyConfigChanged } }
    void fjoyGetDevCapsA() { __asm { jmp winmm.ojoyGetDevCapsA } }
    void fjoyGetDevCapsW() { __asm { jmp winmm.ojoyGetDevCapsW } }
    void fjoyGetNumDevs() { __asm { jmp winmm.ojoyGetNumDevs } }
    void fjoyGetPos() { __asm { jmp winmm.ojoyGetPos } }
    void fjoyGetPosEx() { __asm { jmp winmm.ojoyGetPosEx } }
    void fjoyGetThreshold() { __asm { jmp winmm.ojoyGetThreshold } }
    void fjoyReleaseCapture() { __asm { jmp winmm.ojoyReleaseCapture } }
    void fjoySetCapture() { __asm { jmp winmm.ojoySetCapture } }
    void fjoySetThreshold() { __asm { jmp winmm.ojoySetThreshold } }
    void fmci32Message() { __asm { jmp winmm.omci32Message } }
    void fmciDriverNotify() { __asm { jmp winmm.omciDriverNotify } }
    void fmciDriverYield() { __asm { jmp winmm.omciDriverYield } }
    void fmciExecute() { __asm { jmp winmm.omciExecute } }
    void fmciFreeCommandResource() { __asm { jmp winmm.omciFreeCommandResource } }
    void fmciGetCreatorTask() { __asm { jmp winmm.omciGetCreatorTask } }
    void fmciGetDeviceIDA() { __asm { jmp winmm.omciGetDeviceIDA } }
    void fmciGetDeviceIDFromElementIDA() { __asm { jmp winmm.omciGetDeviceIDFromElementIDA } }
    void fmciGetDeviceIDFromElementIDW() { __asm { jmp winmm.omciGetDeviceIDFromElementIDW } }
    void fmciGetDeviceIDW() { __asm { jmp winmm.omciGetDeviceIDW } }
    void fmciGetDriverData() { __asm { jmp winmm.omciGetDriverData } }
    void fmciGetErrorStringA() { __asm { jmp winmm.omciGetErrorStringA } }
    void fmciGetErrorStringW() { __asm { jmp winmm.omciGetErrorStringW } }
    void fmciGetYieldProc() { __asm { jmp winmm.omciGetYieldProc } }
    void fmciLoadCommandResource() { __asm { jmp winmm.omciLoadCommandResource } }
    void fmciSendCommandA() { __asm { jmp winmm.omciSendCommandA } }
    void fmciSendCommandW() { __asm { jmp winmm.omciSendCommandW } }
    void fmciSendStringA() { __asm { jmp winmm.omciSendStringA } }
    void fmciSendStringW() { __asm { jmp winmm.omciSendStringW } }
    void fmciSetDriverData() { __asm { jmp winmm.omciSetDriverData } }
    void fmciSetYieldProc() { __asm { jmp winmm.omciSetYieldProc } }
    void fmid32Message() { __asm { jmp winmm.omid32Message } }
    void fmidiConnect() { __asm { jmp winmm.omidiConnect } }
    void fmidiDisconnect() { __asm { jmp winmm.omidiDisconnect } }
    void fmidiInAddBuffer() { __asm { jmp winmm.omidiInAddBuffer } }
    void fmidiInClose() { __asm { jmp winmm.omidiInClose } }
    void fmidiInGetDevCapsA() { __asm { jmp winmm.omidiInGetDevCapsA } }
    void fmidiInGetDevCapsW() { __asm { jmp winmm.omidiInGetDevCapsW } }
    void fmidiInGetErrorTextA() { __asm { jmp winmm.omidiInGetErrorTextA } }
    void fmidiInGetErrorTextW() { __asm { jmp winmm.omidiInGetErrorTextW } }
    void fmidiInGetID() { __asm { jmp winmm.omidiInGetID } }
    void fmidiInGetNumDevs() { __asm { jmp winmm.omidiInGetNumDevs } }
    void fmidiInMessage() { __asm { jmp winmm.omidiInMessage } }
    void fmidiInOpen() { __asm { jmp winmm.omidiInOpen } }
    void fmidiInPrepareHeader() { __asm { jmp winmm.omidiInPrepareHeader } }
    void fmidiInReset() { __asm { jmp winmm.omidiInReset } }
    void fmidiInStart() { __asm { jmp winmm.omidiInStart } }
    void fmidiInStop() { __asm { jmp winmm.omidiInStop } }
    void fmidiInUnprepareHeader() { __asm { jmp winmm.omidiInUnprepareHeader } }
    void fmidiOutCacheDrumPatches() { __asm { jmp winmm.omidiOutCacheDrumPatches } }
    void fmidiOutCachePatches() { __asm { jmp winmm.omidiOutCachePatches } }
    void fmidiOutClose() { __asm { jmp winmm.omidiOutClose } }
    void fmidiOutGetDevCapsA() { __asm { jmp winmm.omidiOutGetDevCapsA } }
    void fmidiOutGetDevCapsW() { __asm { jmp winmm.omidiOutGetDevCapsW } }
    void fmidiOutGetErrorTextA() { __asm { jmp winmm.omidiOutGetErrorTextA } }
    void fmidiOutGetErrorTextW() { __asm { jmp winmm.omidiOutGetErrorTextW } }
    void fmidiOutGetID() { __asm { jmp winmm.omidiOutGetID } }
    void fmidiOutGetNumDevs() { __asm { jmp winmm.omidiOutGetNumDevs } }
    void fmidiOutGetVolume() { __asm { jmp winmm.omidiOutGetVolume } }
    void fmidiOutLongMsg() { __asm { jmp winmm.omidiOutLongMsg } }
    void fmidiOutMessage() { __asm { jmp winmm.omidiOutMessage } }
    void fmidiOutOpen() { __asm { jmp winmm.omidiOutOpen } }
    void fmidiOutPrepareHeader() { __asm { jmp winmm.omidiOutPrepareHeader } }
    void fmidiOutReset() { __asm { jmp winmm.omidiOutReset } }
    void fmidiOutSetVolume() { __asm { jmp winmm.omidiOutSetVolume } }
    void fmidiOutShortMsg() { __asm { jmp winmm.omidiOutShortMsg } }
    void fmidiOutUnprepareHeader() { __asm { jmp winmm.omidiOutUnprepareHeader } }
    void fmidiStreamClose() { __asm { jmp winmm.omidiStreamClose } }
    void fmidiStreamOpen() { __asm { jmp winmm.omidiStreamOpen } }
    void fmidiStreamOut() { __asm { jmp winmm.omidiStreamOut } }
    void fmidiStreamPause() { __asm { jmp winmm.omidiStreamPause } }
    void fmidiStreamPosition() { __asm { jmp winmm.omidiStreamPosition } }
    void fmidiStreamProperty() { __asm { jmp winmm.omidiStreamProperty } }
    void fmidiStreamRestart() { __asm { jmp winmm.omidiStreamRestart } }
    void fmidiStreamStop() { __asm { jmp winmm.omidiStreamStop } }
    void fmixerClose() { __asm { jmp winmm.omixerClose } }
    void fmixerGetControlDetailsA() { __asm { jmp winmm.omixerGetControlDetailsA } }
    void fmixerGetControlDetailsW() { __asm { jmp winmm.omixerGetControlDetailsW } }
    void fmixerGetDevCapsA() { __asm { jmp winmm.omixerGetDevCapsA } }
    void fmixerGetDevCapsW() { __asm { jmp winmm.omixerGetDevCapsW } }
    void fmixerGetID() { __asm { jmp winmm.omixerGetID } }
    void fmixerGetLineControlsA() { __asm { jmp winmm.omixerGetLineControlsA } }
    void fmixerGetLineControlsW() { __asm { jmp winmm.omixerGetLineControlsW } }
    void fmixerGetLineInfoA() { __asm { jmp winmm.omixerGetLineInfoA } }
    void fmixerGetLineInfoW() { __asm { jmp winmm.omixerGetLineInfoW } }
    void fmixerGetNumDevs() { __asm { jmp winmm.omixerGetNumDevs } }
    void fmixerMessage() { __asm { jmp winmm.omixerMessage } }
    void fmixerOpen() { __asm { jmp winmm.omixerOpen } }
    void fmixerSetControlDetails() { __asm { jmp winmm.omixerSetControlDetails } }
    void fmmDrvInstall() { __asm { jmp winmm.ommDrvInstall } }
    void fmmGetCurrentTask() { __asm { jmp winmm.ommGetCurrentTask } }
    void fmmTaskBlock() { __asm { jmp winmm.ommTaskBlock } }
    void fmmTaskCreate() { __asm { jmp winmm.ommTaskCreate } }
    void fmmTaskSignal() { __asm { jmp winmm.ommTaskSignal } }
    void fmmTaskYield() { __asm { jmp winmm.ommTaskYield } }
    void fmmioAdvance() { __asm { jmp winmm.ommioAdvance } }
    void fmmioAscend() { __asm { jmp winmm.ommioAscend } }
    void fmmioClose() { __asm { jmp winmm.ommioClose } }
    void fmmioCreateChunk() { __asm { jmp winmm.ommioCreateChunk } }
    void fmmioDescend() { __asm { jmp winmm.ommioDescend } }
    void fmmioFlush() { __asm { jmp winmm.ommioFlush } }
    void fmmioGetInfo() { __asm { jmp winmm.ommioGetInfo } }
    void fmmioInstallIOProcA() { __asm { jmp winmm.ommioInstallIOProcA } }
    void fmmioInstallIOProcW() { __asm { jmp winmm.ommioInstallIOProcW } }
    void fmmioOpenA() { __asm { jmp winmm.ommioOpenA } }
    void fmmioOpenW() { __asm { jmp winmm.ommioOpenW } }
    void fmmioRead() { __asm { jmp winmm.ommioRead } }
    void fmmioRenameA() { __asm { jmp winmm.ommioRenameA } }
    void fmmioRenameW() { __asm { jmp winmm.ommioRenameW } }
    void fmmioSeek() { __asm { jmp winmm.ommioSeek } }
    void fmmioSendMessage() { __asm { jmp winmm.ommioSendMessage } }
    void fmmioSetBuffer() { __asm { jmp winmm.ommioSetBuffer } }
    void fmmioSetInfo() { __asm { jmp winmm.ommioSetInfo } }
    void fmmioStringToFOURCCA() { __asm { jmp winmm.ommioStringToFOURCCA } }
    void fmmioStringToFOURCCW() { __asm { jmp winmm.ommioStringToFOURCCW } }
    void fmmioWrite() { __asm { jmp winmm.ommioWrite } }
    void fmmsystemGetVersion() { __asm { jmp winmm.ommsystemGetVersion } }
    void fmod32Message() { __asm { jmp winmm.omod32Message } }
    void fmxd32Message() { __asm { jmp winmm.omxd32Message } }
    void fsndPlaySoundA() { __asm { jmp winmm.osndPlaySoundA } }
    void fsndPlaySoundW() { __asm { jmp winmm.osndPlaySoundW } }
    void ftid32Message() { __asm { jmp winmm.otid32Message } }
    void ftimeBeginPeriod() { __asm { jmp winmm.otimeBeginPeriod } }
    void ftimeEndPeriod() { __asm { jmp winmm.otimeEndPeriod } }
    void ftimeGetDevCaps() { __asm { jmp winmm.otimeGetDevCaps } }
    void ftimeGetSystemTime() { __asm { jmp winmm.otimeGetSystemTime } }
    void ftimeGetTime() { __asm { jmp winmm.otimeGetTime } }
    void ftimeKillEvent() { __asm { jmp winmm.otimeKillEvent } }
    void ftimeSetEvent() { __asm { jmp winmm.otimeSetEvent } }
    void fwaveInAddBuffer() { __asm { jmp winmm.owaveInAddBuffer } }
    void fwaveInClose() { __asm { jmp winmm.owaveInClose } }
    void fwaveInGetDevCapsA() { __asm { jmp winmm.owaveInGetDevCapsA } }
    void fwaveInGetDevCapsW() { __asm { jmp winmm.owaveInGetDevCapsW } }
    void fwaveInGetErrorTextA() { __asm { jmp winmm.owaveInGetErrorTextA } }
    void fwaveInGetErrorTextW() { __asm { jmp winmm.owaveInGetErrorTextW } }
    void fwaveInGetID() { __asm { jmp winmm.owaveInGetID } }
    void fwaveInGetNumDevs() { __asm { jmp winmm.owaveInGetNumDevs } }
    void fwaveInGetPosition() { __asm { jmp winmm.owaveInGetPosition } }
    void fwaveInMessage() { __asm { jmp winmm.owaveInMessage } }
    void fwaveInOpen() { __asm { jmp winmm.owaveInOpen } }
    void fwaveInPrepareHeader() { __asm { jmp winmm.owaveInPrepareHeader } }
    void fwaveInReset() { __asm { jmp winmm.owaveInReset } }
    void fwaveInStart() { __asm { jmp winmm.owaveInStart } }
    void fwaveInStop() { __asm { jmp winmm.owaveInStop } }
    void fwaveInUnprepareHeader() { __asm { jmp winmm.owaveInUnprepareHeader } }
    void fwaveOutBreakLoop() { __asm { jmp winmm.owaveOutBreakLoop } }
    void fwaveOutClose() { __asm { jmp winmm.owaveOutClose } }
    void fwaveOutGetDevCapsA() { __asm { jmp winmm.owaveOutGetDevCapsA } }
    void fwaveOutGetDevCapsW() { __asm { jmp winmm.owaveOutGetDevCapsW } }
    void fwaveOutGetErrorTextA() { __asm { jmp winmm.owaveOutGetErrorTextA } }
    void fwaveOutGetErrorTextW() { __asm { jmp winmm.owaveOutGetErrorTextW } }
    void fwaveOutGetID() { __asm { jmp winmm.owaveOutGetID } }
    void fwaveOutGetNumDevs() { __asm { jmp winmm.owaveOutGetNumDevs } }
    void fwaveOutGetPitch() { __asm { jmp winmm.owaveOutGetPitch } }
    void fwaveOutGetPlaybackRate() { __asm { jmp winmm.owaveOutGetPlaybackRate } }
    void fwaveOutGetPosition() { __asm { jmp winmm.owaveOutGetPosition } }
    void fwaveOutGetVolume() { __asm { jmp winmm.owaveOutGetVolume } }
    void fwaveOutMessage() { __asm { jmp winmm.owaveOutMessage } }
    void fwaveOutOpen() { __asm { jmp winmm.owaveOutOpen } }
    void fwaveOutPause() { __asm { jmp winmm.owaveOutPause } }
    void fwaveOutPrepareHeader() { __asm { jmp winmm.owaveOutPrepareHeader } }
    void fwaveOutReset() { __asm { jmp winmm.owaveOutReset } }
    void fwaveOutRestart() { __asm { jmp winmm.owaveOutRestart } }
    void fwaveOutSetPitch() { __asm { jmp winmm.owaveOutSetPitch } }
    void fwaveOutSetPlaybackRate() { __asm { jmp winmm.owaveOutSetPlaybackRate } }
    void fwaveOutSetVolume() { __asm { jmp winmm.owaveOutSetVolume } }
    void fwaveOutUnprepareHeader() { __asm { jmp winmm.owaveOutUnprepareHeader } }
    void fwaveOutWrite() { __asm { jmp winmm.owaveOutWrite } }
    void fwid32Message() { __asm { jmp winmm.owid32Message } }
    void fwinmmDbgOut() { __asm { jmp winmm.owinmmDbgOut } }
    void fwinmmSetDebugLevel() { __asm { jmp winmm.owinmmSetDebugLevel } }
    void fwod32Message() { __asm { jmp winmm.owod32Message } }
}

void SetupWinmmFunctions() {
	char path[MAX_PATH];
	GetWindowsDirectoryA(path, sizeof(path));

	strcat(path, "\\System32\\winmm.dll");
	winmm.dll = LoadLibraryA(path);

	winmm.oCloseDriver = GetProcAddress(winmm.dll, "CloseDriver");
	winmm.oDefDriverProc = GetProcAddress(winmm.dll, "DefDriverProc");
	winmm.oDriverCallback = GetProcAddress(winmm.dll, "DriverCallback");
	winmm.oDrvGetModuleHandle = GetProcAddress(winmm.dll, "DrvGetModuleHandle");
	winmm.oGetDriverModuleHandle = GetProcAddress(winmm.dll, "GetDriverModuleHandle");
	winmm.oMigrateAllDrivers = GetProcAddress(winmm.dll, "MigrateAllDrivers");
	winmm.oMigrateSoundEvents = GetProcAddress(winmm.dll, "MigrateSoundEvents");
	winmm.oNotifyCallbackData = GetProcAddress(winmm.dll, "NotifyCallbackData");
	winmm.oOpenDriver = GetProcAddress(winmm.dll, "OpenDriver");
	winmm.oPlaySound = GetProcAddress(winmm.dll, "PlaySound");
	winmm.oPlaySoundA = GetProcAddress(winmm.dll, "PlaySoundA");
	winmm.oPlaySoundW = GetProcAddress(winmm.dll, "PlaySoundW");
	winmm.oSendDriverMessage = GetProcAddress(winmm.dll, "SendDriverMessage");
	winmm.oWOW32DriverCallback = GetProcAddress(winmm.dll, "WOW32DriverCallback");
	winmm.oWOW32ResolveMultiMediaHandle = GetProcAddress(winmm.dll, "WOW32ResolveMultiMediaHandle");
	winmm.oWOWAppExit = GetProcAddress(winmm.dll, "WOWAppExit");
	winmm.oWinmmLogoff = GetProcAddress(winmm.dll, "WinmmLogoff");
	winmm.oWinmmLogon = GetProcAddress(winmm.dll, "WinmmLogon");
	winmm.o_gfxLogoff = GetProcAddress(winmm.dll, "_gfxLogoff");
	winmm.o_gfxLogon = GetProcAddress(winmm.dll, "_gfxLogon");
	winmm.oaux32Message = GetProcAddress(winmm.dll, "aux32Message");
	winmm.oauxGetDevCapsA = GetProcAddress(winmm.dll, "auxGetDevCapsA");
	winmm.oauxGetDevCapsW = GetProcAddress(winmm.dll, "auxGetDevCapsW");
	winmm.oauxGetNumDevs = GetProcAddress(winmm.dll, "auxGetNumDevs");
	winmm.oauxGetVolume = GetProcAddress(winmm.dll, "auxGetVolume");
	winmm.oauxOutMessage = GetProcAddress(winmm.dll, "auxOutMessage");
	winmm.oauxSetVolume = GetProcAddress(winmm.dll, "auxSetVolume");
	winmm.ogfxAddGfx = GetProcAddress(winmm.dll, "gfxAddGfx");
	winmm.ogfxBatchChange = GetProcAddress(winmm.dll, "gfxBatchChange");
	winmm.ogfxCreateGfxFactoriesList = GetProcAddress(winmm.dll, "gfxCreateGfxFactoriesList");
	winmm.ogfxCreateZoneFactoriesList = GetProcAddress(winmm.dll, "gfxCreateZoneFactoriesList");
	winmm.ogfxDestroyDeviceInterfaceList = GetProcAddress(winmm.dll, "gfxDestroyDeviceInterfaceList");
	winmm.ogfxEnumerateGfxs = GetProcAddress(winmm.dll, "gfxEnumerateGfxs");
	winmm.ogfxModifyGfx = GetProcAddress(winmm.dll, "gfxModifyGfx");
	winmm.ogfxOpenGfx = GetProcAddress(winmm.dll, "gfxOpenGfx");
	winmm.ogfxRemoveGfx = GetProcAddress(winmm.dll, "gfxRemoveGfx");
	winmm.ojoy32Message = GetProcAddress(winmm.dll, "joy32Message");
	winmm.ojoyConfigChanged = GetProcAddress(winmm.dll, "joyConfigChanged");
	winmm.ojoyGetDevCapsA = GetProcAddress(winmm.dll, "joyGetDevCapsA");
	winmm.ojoyGetDevCapsW = GetProcAddress(winmm.dll, "joyGetDevCapsW");
	winmm.ojoyGetNumDevs = GetProcAddress(winmm.dll, "joyGetNumDevs");
	winmm.ojoyGetPos = GetProcAddress(winmm.dll, "joyGetPos");
	winmm.ojoyGetPosEx = GetProcAddress(winmm.dll, "joyGetPosEx");
	winmm.ojoyGetThreshold = GetProcAddress(winmm.dll, "joyGetThreshold");
	winmm.ojoyReleaseCapture = GetProcAddress(winmm.dll, "joyReleaseCapture");
	winmm.ojoySetCapture = GetProcAddress(winmm.dll, "joySetCapture");
	winmm.ojoySetThreshold = GetProcAddress(winmm.dll, "joySetThreshold");
	winmm.omci32Message = GetProcAddress(winmm.dll, "mci32Message");
	winmm.omciDriverNotify = GetProcAddress(winmm.dll, "mciDriverNotify");
	winmm.omciDriverYield = GetProcAddress(winmm.dll, "mciDriverYield");
	winmm.omciExecute = GetProcAddress(winmm.dll, "mciExecute");
	winmm.omciFreeCommandResource = GetProcAddress(winmm.dll, "mciFreeCommandResource");
	winmm.omciGetCreatorTask = GetProcAddress(winmm.dll, "mciGetCreatorTask");
	winmm.omciGetDeviceIDA = GetProcAddress(winmm.dll, "mciGetDeviceIDA");
	winmm.omciGetDeviceIDFromElementIDA = GetProcAddress(winmm.dll, "mciGetDeviceIDFromElementIDA");
	winmm.omciGetDeviceIDFromElementIDW = GetProcAddress(winmm.dll, "mciGetDeviceIDFromElementIDW");
	winmm.omciGetDeviceIDW = GetProcAddress(winmm.dll, "mciGetDeviceIDW");
	winmm.omciGetDriverData = GetProcAddress(winmm.dll, "mciGetDriverData");
	winmm.omciGetErrorStringA = GetProcAddress(winmm.dll, "mciGetErrorStringA");
	winmm.omciGetErrorStringW = GetProcAddress(winmm.dll, "mciGetErrorStringW");
	winmm.omciGetYieldProc = GetProcAddress(winmm.dll, "mciGetYieldProc");
	winmm.omciLoadCommandResource = GetProcAddress(winmm.dll, "mciLoadCommandResource");
	winmm.omciSendCommandA = GetProcAddress(winmm.dll, "mciSendCommandA");
	winmm.omciSendCommandW = GetProcAddress(winmm.dll, "mciSendCommandW");
	winmm.omciSendStringA = GetProcAddress(winmm.dll, "mciSendStringA");
	winmm.omciSendStringW = GetProcAddress(winmm.dll, "mciSendStringW");
	winmm.omciSetDriverData = GetProcAddress(winmm.dll, "mciSetDriverData");
	winmm.omciSetYieldProc = GetProcAddress(winmm.dll, "mciSetYieldProc");
	winmm.omid32Message = GetProcAddress(winmm.dll, "mid32Message");
	winmm.omidiConnect = GetProcAddress(winmm.dll, "midiConnect");
	winmm.omidiDisconnect = GetProcAddress(winmm.dll, "midiDisconnect");
	winmm.omidiInAddBuffer = GetProcAddress(winmm.dll, "midiInAddBuffer");
	winmm.omidiInClose = GetProcAddress(winmm.dll, "midiInClose");
	winmm.omidiInGetDevCapsA = GetProcAddress(winmm.dll, "midiInGetDevCapsA");
	winmm.omidiInGetDevCapsW = GetProcAddress(winmm.dll, "midiInGetDevCapsW");
	winmm.omidiInGetErrorTextA = GetProcAddress(winmm.dll, "midiInGetErrorTextA");
	winmm.omidiInGetErrorTextW = GetProcAddress(winmm.dll, "midiInGetErrorTextW");
	winmm.omidiInGetID = GetProcAddress(winmm.dll, "midiInGetID");
	winmm.omidiInGetNumDevs = GetProcAddress(winmm.dll, "midiInGetNumDevs");
	winmm.omidiInMessage = GetProcAddress(winmm.dll, "midiInMessage");
	winmm.omidiInOpen = GetProcAddress(winmm.dll, "midiInOpen");
	winmm.omidiInPrepareHeader = GetProcAddress(winmm.dll, "midiInPrepareHeader");
	winmm.omidiInReset = GetProcAddress(winmm.dll, "midiInReset");
	winmm.omidiInStart = GetProcAddress(winmm.dll, "midiInStart");
	winmm.omidiInStop = GetProcAddress(winmm.dll, "midiInStop");
	winmm.omidiInUnprepareHeader = GetProcAddress(winmm.dll, "midiInUnprepareHeader");
	winmm.omidiOutCacheDrumPatches = GetProcAddress(winmm.dll, "midiOutCacheDrumPatches");
	winmm.omidiOutCachePatches = GetProcAddress(winmm.dll, "midiOutCachePatches");
	winmm.omidiOutClose = GetProcAddress(winmm.dll, "midiOutClose");
	winmm.omidiOutGetDevCapsA = GetProcAddress(winmm.dll, "midiOutGetDevCapsA");
	winmm.omidiOutGetDevCapsW = GetProcAddress(winmm.dll, "midiOutGetDevCapsW");
	winmm.omidiOutGetErrorTextA = GetProcAddress(winmm.dll, "midiOutGetErrorTextA");
	winmm.omidiOutGetErrorTextW = GetProcAddress(winmm.dll, "midiOutGetErrorTextW");
	winmm.omidiOutGetID = GetProcAddress(winmm.dll, "midiOutGetID");
	winmm.omidiOutGetNumDevs = GetProcAddress(winmm.dll, "midiOutGetNumDevs");
	winmm.omidiOutGetVolume = GetProcAddress(winmm.dll, "midiOutGetVolume");
	winmm.omidiOutLongMsg = GetProcAddress(winmm.dll, "midiOutLongMsg");
	winmm.omidiOutMessage = GetProcAddress(winmm.dll, "midiOutMessage");
	winmm.omidiOutOpen = GetProcAddress(winmm.dll, "midiOutOpen");
	winmm.omidiOutPrepareHeader = GetProcAddress(winmm.dll, "midiOutPrepareHeader");
	winmm.omidiOutReset = GetProcAddress(winmm.dll, "midiOutReset");
	winmm.omidiOutSetVolume = GetProcAddress(winmm.dll, "midiOutSetVolume");
	winmm.omidiOutShortMsg = GetProcAddress(winmm.dll, "midiOutShortMsg");
	winmm.omidiOutUnprepareHeader = GetProcAddress(winmm.dll, "midiOutUnprepareHeader");
	winmm.omidiStreamClose = GetProcAddress(winmm.dll, "midiStreamClose");
	winmm.omidiStreamOpen = GetProcAddress(winmm.dll, "midiStreamOpen");
	winmm.omidiStreamOut = GetProcAddress(winmm.dll, "midiStreamOut");
	winmm.omidiStreamPause = GetProcAddress(winmm.dll, "midiStreamPause");
	winmm.omidiStreamPosition = GetProcAddress(winmm.dll, "midiStreamPosition");
	winmm.omidiStreamProperty = GetProcAddress(winmm.dll, "midiStreamProperty");
	winmm.omidiStreamRestart = GetProcAddress(winmm.dll, "midiStreamRestart");
	winmm.omidiStreamStop = GetProcAddress(winmm.dll, "midiStreamStop");
	winmm.omixerClose = GetProcAddress(winmm.dll, "mixerClose");
	winmm.omixerGetControlDetailsA = GetProcAddress(winmm.dll, "mixerGetControlDetailsA");
	winmm.omixerGetControlDetailsW = GetProcAddress(winmm.dll, "mixerGetControlDetailsW");
	winmm.omixerGetDevCapsA = GetProcAddress(winmm.dll, "mixerGetDevCapsA");
	winmm.omixerGetDevCapsW = GetProcAddress(winmm.dll, "mixerGetDevCapsW");
	winmm.omixerGetID = GetProcAddress(winmm.dll, "mixerGetID");
	winmm.omixerGetLineControlsA = GetProcAddress(winmm.dll, "mixerGetLineControlsA");
	winmm.omixerGetLineControlsW = GetProcAddress(winmm.dll, "mixerGetLineControlsW");
	winmm.omixerGetLineInfoA = GetProcAddress(winmm.dll, "mixerGetLineInfoA");
	winmm.omixerGetLineInfoW = GetProcAddress(winmm.dll, "mixerGetLineInfoW");
	winmm.omixerGetNumDevs = GetProcAddress(winmm.dll, "mixerGetNumDevs");
	winmm.omixerMessage = GetProcAddress(winmm.dll, "mixerMessage");
	winmm.omixerOpen = GetProcAddress(winmm.dll, "mixerOpen");
	winmm.omixerSetControlDetails = GetProcAddress(winmm.dll, "mixerSetControlDetails");
	winmm.ommDrvInstall = GetProcAddress(winmm.dll, "mmDrvInstall");
	winmm.ommGetCurrentTask = GetProcAddress(winmm.dll, "mmGetCurrentTask");
	winmm.ommTaskBlock = GetProcAddress(winmm.dll, "mmTaskBlock");
	winmm.ommTaskCreate = GetProcAddress(winmm.dll, "mmTaskCreate");
	winmm.ommTaskSignal = GetProcAddress(winmm.dll, "mmTaskSignal");
	winmm.ommTaskYield = GetProcAddress(winmm.dll, "mmTaskYield");
	winmm.ommioAdvance = GetProcAddress(winmm.dll, "mmioAdvance");
	winmm.ommioAscend = GetProcAddress(winmm.dll, "mmioAscend");
	winmm.ommioClose = GetProcAddress(winmm.dll, "mmioClose");
	winmm.ommioCreateChunk = GetProcAddress(winmm.dll, "mmioCreateChunk");
	winmm.ommioDescend = GetProcAddress(winmm.dll, "mmioDescend");
	winmm.ommioFlush = GetProcAddress(winmm.dll, "mmioFlush");
	winmm.ommioGetInfo = GetProcAddress(winmm.dll, "mmioGetInfo");
	winmm.ommioInstallIOProcA = GetProcAddress(winmm.dll, "mmioInstallIOProcA");
	winmm.ommioInstallIOProcW = GetProcAddress(winmm.dll, "mmioInstallIOProcW");
	winmm.ommioOpenA = GetProcAddress(winmm.dll, "mmioOpenA");
	winmm.ommioOpenW = GetProcAddress(winmm.dll, "mmioOpenW");
	winmm.ommioRead = GetProcAddress(winmm.dll, "mmioRead");
	winmm.ommioRenameA = GetProcAddress(winmm.dll, "mmioRenameA");
	winmm.ommioRenameW = GetProcAddress(winmm.dll, "mmioRenameW");
	winmm.ommioSeek = GetProcAddress(winmm.dll, "mmioSeek");
	winmm.ommioSendMessage = GetProcAddress(winmm.dll, "mmioSendMessage");
	winmm.ommioSetBuffer = GetProcAddress(winmm.dll, "mmioSetBuffer");
	winmm.ommioSetInfo = GetProcAddress(winmm.dll, "mmioSetInfo");
	winmm.ommioStringToFOURCCA = GetProcAddress(winmm.dll, "mmioStringToFOURCCA");
	winmm.ommioStringToFOURCCW = GetProcAddress(winmm.dll, "mmioStringToFOURCCW");
	winmm.ommioWrite = GetProcAddress(winmm.dll, "mmioWrite");
	winmm.ommsystemGetVersion = GetProcAddress(winmm.dll, "mmsystemGetVersion");
	winmm.omod32Message = GetProcAddress(winmm.dll, "mod32Message");
	winmm.omxd32Message = GetProcAddress(winmm.dll, "mxd32Message");
	winmm.osndPlaySoundA = GetProcAddress(winmm.dll, "sndPlaySoundA");
	winmm.osndPlaySoundW = GetProcAddress(winmm.dll, "sndPlaySoundW");
	winmm.otid32Message = GetProcAddress(winmm.dll, "tid32Message");
	winmm.otimeBeginPeriod = GetProcAddress(winmm.dll, "timeBeginPeriod");
	winmm.otimeEndPeriod = GetProcAddress(winmm.dll, "timeEndPeriod");
	winmm.otimeGetDevCaps = GetProcAddress(winmm.dll, "timeGetDevCaps");
	winmm.otimeGetSystemTime = GetProcAddress(winmm.dll, "timeGetSystemTime");
	winmm.otimeGetTime = GetProcAddress(winmm.dll, "timeGetTime");
	winmm.otimeKillEvent = GetProcAddress(winmm.dll, "timeKillEvent");
	winmm.otimeSetEvent = GetProcAddress(winmm.dll, "timeSetEvent");
	winmm.owaveInAddBuffer = GetProcAddress(winmm.dll, "waveInAddBuffer");
	winmm.owaveInClose = GetProcAddress(winmm.dll, "waveInClose");
	winmm.owaveInGetDevCapsA = GetProcAddress(winmm.dll, "waveInGetDevCapsA");
	winmm.owaveInGetDevCapsW = GetProcAddress(winmm.dll, "waveInGetDevCapsW");
	winmm.owaveInGetErrorTextA = GetProcAddress(winmm.dll, "waveInGetErrorTextA");
	winmm.owaveInGetErrorTextW = GetProcAddress(winmm.dll, "waveInGetErrorTextW");
	winmm.owaveInGetID = GetProcAddress(winmm.dll, "waveInGetID");
	winmm.owaveInGetNumDevs = GetProcAddress(winmm.dll, "waveInGetNumDevs");
	winmm.owaveInGetPosition = GetProcAddress(winmm.dll, "waveInGetPosition");
	winmm.owaveInMessage = GetProcAddress(winmm.dll, "waveInMessage");
	winmm.owaveInOpen = GetProcAddress(winmm.dll, "waveInOpen");
	winmm.owaveInPrepareHeader = GetProcAddress(winmm.dll, "waveInPrepareHeader");
	winmm.owaveInReset = GetProcAddress(winmm.dll, "waveInReset");
	winmm.owaveInStart = GetProcAddress(winmm.dll, "waveInStart");
	winmm.owaveInStop = GetProcAddress(winmm.dll, "waveInStop");
	winmm.owaveInUnprepareHeader = GetProcAddress(winmm.dll, "waveInUnprepareHeader");
	winmm.owaveOutBreakLoop = GetProcAddress(winmm.dll, "waveOutBreakLoop");
	winmm.owaveOutClose = GetProcAddress(winmm.dll, "waveOutClose");
	winmm.owaveOutGetDevCapsA = GetProcAddress(winmm.dll, "waveOutGetDevCapsA");
	winmm.owaveOutGetDevCapsW = GetProcAddress(winmm.dll, "waveOutGetDevCapsW");
	winmm.owaveOutGetErrorTextA = GetProcAddress(winmm.dll, "waveOutGetErrorTextA");
	winmm.owaveOutGetErrorTextW = GetProcAddress(winmm.dll, "waveOutGetErrorTextW");
	winmm.owaveOutGetID = GetProcAddress(winmm.dll, "waveOutGetID");
	winmm.owaveOutGetNumDevs = GetProcAddress(winmm.dll, "waveOutGetNumDevs");
	winmm.owaveOutGetPitch = GetProcAddress(winmm.dll, "waveOutGetPitch");
	winmm.owaveOutGetPlaybackRate = GetProcAddress(winmm.dll, "waveOutGetPlaybackRate");
	winmm.owaveOutGetPosition = GetProcAddress(winmm.dll, "waveOutGetPosition");
	winmm.owaveOutGetVolume = GetProcAddress(winmm.dll, "waveOutGetVolume");
	winmm.owaveOutMessage = GetProcAddress(winmm.dll, "waveOutMessage");
	winmm.owaveOutOpen = GetProcAddress(winmm.dll, "waveOutOpen");
	winmm.owaveOutPause = GetProcAddress(winmm.dll, "waveOutPause");
	winmm.owaveOutPrepareHeader = GetProcAddress(winmm.dll, "waveOutPrepareHeader");
	winmm.owaveOutReset = GetProcAddress(winmm.dll, "waveOutReset");
	winmm.owaveOutRestart = GetProcAddress(winmm.dll, "waveOutRestart");
	winmm.owaveOutSetPitch = GetProcAddress(winmm.dll, "waveOutSetPitch");
	winmm.owaveOutSetPlaybackRate = GetProcAddress(winmm.dll, "waveOutSetPlaybackRate");
	winmm.owaveOutSetVolume = GetProcAddress(winmm.dll, "waveOutSetVolume");
	winmm.owaveOutUnprepareHeader = GetProcAddress(winmm.dll, "waveOutUnprepareHeader");
	winmm.owaveOutWrite = GetProcAddress(winmm.dll, "waveOutWrite");
	winmm.owid32Message = GetProcAddress(winmm.dll, "wid32Message");
	winmm.owinmmDbgOut = GetProcAddress(winmm.dll, "winmmDbgOut");
	winmm.owinmmSetDebugLevel = GetProcAddress(winmm.dll, "winmmSetDebugLevel");
	winmm.owod32Message = GetProcAddress(winmm.dll, "wod32Message");
}