// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      usb_storage.cpp
//      Author :    Veselin Sladkov
//      Date :      20th November 2023
//      Reviewed :  No
//      Purpose :   USB MSC Storage / FATFS link from Apple/Oric emulators.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include <inttypes.h>
#include "tusb.h"
#include "ff.h"
#include "diskio.h"

//		T-54 : these two used to be indexed by the USB device address itself, but TinyUSB hands
//		out addresses 1..CFG_TUH_DEVICE_MAX (address 0 is the enumeration one), so a key that
//		got the LAST address wrote one slot past the end of both arrays — a stray byte for the
//		busy flag, and a whole FATFS structure (hundreds of bytes) over whatever the linker had
//		put next. On every disk access. With a hub plus keyboard, mouse, modem and key, that
//		address is reached in practice (bmarty's board). Slot = address - 1, and every access
//		is bounds checked. What the overrun hit depended on the binary, which is what made the
//		board's behaviour look random (T-48) and painted red streaks when it landed in
//		graphicsMemory (colour 1 of the default palette is 255,0,77).
static FATFS msc_fatfs_volumes[CFG_TUH_DEVICE_MAX];
static volatile bool msc_volume_busy[CFG_TUH_DEVICE_MAX];
static volatile bool msc_volume_failed[CFG_TUH_DEVICE_MAX];  					// T-34 : SCSI status of the last transfer

static inline int mscSlot(uint8_t dev_addr) {  									// USB address -> array slot, or -1
	int slot = (int)dev_addr - 1;
	return (slot >= 0 && slot < CFG_TUH_DEVICE_MAX) ? slot : -1;
}
// Trinity T-24 : logical drive n (FatFs "n:", API volume n, NeoDOS letter A+n) = n-th key mounted, whatever its
// USB address (the upstream mounted a key at its address : behind a hub the first key was "1:" = B:, no A:).
static uint8_t driveDevice[FF_VOLUMES];                                          // FatFs drive -> USB device address (0 = free)
static uint8_t deviceOf(BYTE pdrv) { return (pdrv < FF_VOLUMES) ? driveDevice[pdrv] : 0; }
static int driveOfDevice(uint8_t dev_addr) { for (int i = 0;i < FF_VOLUMES;i++) if (driveDevice[i] == dev_addr) return i;return -1; }
void FISNoteCurrentVolume(uint8_t volume);                                       // fileimplementation.cpp
static scsi_inquiry_resp_t msc_inquiry_resp;
bool msc_inquiry_complete = false;

// ***************************************************************************************
//
//                                  Storage initialise
//
// ***************************************************************************************

void STOInitialise(void) {
}

// ***************************************************************************************
//
//    Wait for USB to 'settle' ; not quite sure why this is required, time to process 
//    USB Messages ?
//
// ***************************************************************************************

// T-32 : the wait for the bus to settle is now the boot barrier (USBWaitSettled, phase P1) ; this
// only reports what was mounted. The old "wait 2 s for the first key" heuristic is gone.
void STOSynchronise(void) {
    CONWriteString("USB Storage%s\r",msc_inquiry_complete ? "" : " (no key)");
}

// ***************************************************************************************
//
//                              USB Key found, initialised.
//
// ***************************************************************************************

bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data) {
    if (cb_data->csw->status != 0) {
        CONWriteString("MSC SCSI inquiry failed\r");
        return false;
    }

    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    CONWriteString("USB Key found %04x %04x\r",vid,pid);

    int drive = driveOfDevice(dev_addr);                                        // First free logical drive (T-24)
    if (drive < 0) drive = driveOfDevice(0);
    if (drive < 0) { CONWriteString("MSC : no logical drive left\r");return false; }
    driveDevice[drive] = dev_addr;
    USBNoteEvent();                                                             // T-32 : mount finished, bus still busy
    char drive_path[3] = "0:";
    drive_path[0] += drive;
    int slot = mscSlot(dev_addr);
    if (slot < 0) { CONWriteString("MSC : USB address %d out of range\r",dev_addr);return false; }
    FRESULT result = f_mount(&msc_fatfs_volumes[slot], drive_path, 1);
    if (result != FR_OK) {
        driveDevice[drive] = 0;
        CONWriteString("MSC filesystem mount failed, FatFs error ");CONWriteHex(result);CONWrite('\r');
        return false;
    }

    char s[2];
    if (FR_OK != f_getcwd(s, 2)) {                                              // No current drive yet : this key
        f_chdrive(drive_path);
        f_chdir("/");
        FISNoteCurrentVolume(drive);
    }
    CONWriteString("Volume %d: (%c:)\r",drive,'A' + drive);

    msc_inquiry_complete = true;

    return true;
}

// ***************************************************************************************
//
//                              Mount and unmount devices
//
// ***************************************************************************************

void tuh_msc_mount_cb(uint8_t dev_addr) {
    USBNoteEvent();                                                             // T-32 : enumeration barrier
    uint8_t const lun = 0;
    //CONWriteString("MSC mounted, inquiring\r\n");
    tuh_msc_inquiry(dev_addr, lun, &msc_inquiry_resp, inquiry_complete_cb, 0);
}

void tuh_msc_umount_cb(uint8_t dev_addr) {
    USBNoteEvent();                                                             // T-32
    int drive = driveOfDevice(dev_addr);
    if (drive < 0) return;
    char drive_path[3] = "0:";
    drive_path[0] += drive;
    f_unmount(drive_path);
    driveDevice[drive] = 0;
    int slot = mscSlot(dev_addr);  												// T-34 : a transfer in flight will never complete ;
    if (slot >= 0) { msc_volume_failed[slot] = true;msc_volume_busy[slot] = false; }	// failed first, so a wait ended here is an error
}

// ***************************************************************************************
//
//                                  sInterface to FATFS
//
// ***************************************************************************************

bool HWBusServeInWait(void);  													// T-82 : processor_pio.cpp
void HWBusServeBurst(void);

//		T-34 (ADR-0001 § 5) : this waited for ever. A key pulled out, or one that stops
//		answering, froze the whole machine : the 65C02 is stalled for the length of the API
//		call. Now the wait ends when the key is gone (RES_NOTRDY) or after DISK_IO_TIMEOUT_US
//		(RES_ERROR, FatFs reports a disk error). ADR-0001 says 500 ms ; 2 s is used because the
//		worst case of a WRITE on a real key has not been measured yet, and a write cut short can
//		damage the file system : stoDiskWaitMaxUs records the longest wait so the limit can be
//		set from a measurement. A transfer still pending after a timeout keeps its slot busy :
//		the next accesses to that key are refused at once instead of piling up.
#define DISK_IO_TIMEOUT_US  (2000000)
extern volatile uint32_t stoDiskWaitMaxUs,stoDiskTimeouts;  					// debugport.cpp

static DRESULT wait_for_disk_io(uint8_t dev_addr) {                             // By device, slot = address - 1 (T-24, T-54)
    int slot = mscSlot(dev_addr);
    if (slot < 0) return RES_PARERR;
    uint32_t t0 = time_us_32();
    while (msc_volume_busy[slot]) {
        tuh_task();
        if (HWBusServeInWait()) HWBusServeBurst();  							// T-82 : the 65C02 runs meanwhile (3,28)
        if (!tuh_msc_mounted(dev_addr)) {  										// Key gone : its transfer is gone too
            msc_volume_busy[slot] = false;
            return RES_NOTRDY;
        }
        if (time_us_32() - t0 > DISK_IO_TIMEOUT_US) {
            stoDiskTimeouts = stoDiskTimeouts + 1;
            return RES_ERROR;
        }
    }
    uint32_t d = time_us_32() - t0;
    if (d > stoDiskWaitMaxUs) stoDiskWaitMaxUs = d;
    return msc_volume_failed[slot] ? RES_ERROR : RES_OK;
}

static bool disk_io_complete(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data) {
    int slot = mscSlot(dev_addr);
    if (slot >= 0) {
        msc_volume_failed[slot] = (cb_data->csw->status != 0);  					// T-34 : a refused command is an error
        msc_volume_busy[slot] = false;
    }
    return true;
}

DSTATUS disk_status(BYTE pdrv) {
    uint8_t dev_addr = deviceOf(pdrv);
    if (dev_addr == 0) return STA_NODISK;
    return tuh_msc_mounted(dev_addr) ? 0 : STA_NODISK;
}

DSTATUS disk_initialize(BYTE pdrv) {
    (void)(pdrv);
    return 0;
}

//		T-74 : what the !f report needs, straight from RAM — no FatFs call, no TinyUSB call,
//		because the report is emitted from DSPSync.
uint8_t __not_in_flash_func(STODebugDrive)(int drive) {
	return (drive >= 0 && drive < FF_VOLUMES) ? driveDevice[drive] : 0;
}

bool __not_in_flash_func(STODebugBusy)(int slot) {
	return (slot >= 0 && slot < CFG_TUH_DEVICE_MAX) ? msc_volume_busy[slot] : false;
}

extern volatile uint32_t stoSectorCount;  										// T-73 : defined in debugport.cpp
extern volatile uint32_t stoDiskReads,stoDiskWaitUs;  							// T-82 : idem

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    uint8_t const dev_addr = deviceOf(pdrv);
    if (dev_addr == 0) return RES_NOTRDY;
    uint8_t const lun = 0;
    int slot = mscSlot(dev_addr);
    if (slot < 0) return RES_PARERR;
    if (msc_volume_busy[slot]) return RES_NOTRDY;  								// T-34 : a timed out transfer still pending
    msc_volume_busy[slot] = true;                                               // Busy flag by device : the completion
    msc_volume_failed[slot] = false;
    stoSectorCount += count;  													// T-73
    if (!tuh_msc_read10(dev_addr, lun, buff, sector, (uint16_t)count, disk_io_complete, 0)) {   // callback only knows dev_addr (T-24)
        msc_volume_busy[slot] = false;  										// T-34 : not queued, nothing will complete
        return RES_ERROR;
    }
    uint32_t t0 = time_us_32();  												// T-82 : time with the transfer in flight
    DRESULT r = wait_for_disk_io(dev_addr);
    stoDiskWaitUs += time_us_32() - t0;stoDiskReads = stoDiskReads + 1;
    return r;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    stoSectorCount += count;  													// T-73
    uint8_t const dev_addr = deviceOf(pdrv);
    if (dev_addr == 0) return RES_NOTRDY;
    uint8_t const lun = 0;
    int slot = mscSlot(dev_addr);
    if (slot < 0) return RES_PARERR;
    if (msc_volume_busy[slot]) return RES_NOTRDY;  								// T-34
    msc_volume_busy[slot] = true;
    msc_volume_failed[slot] = false;
    if (!tuh_msc_write10(dev_addr, lun, buff, sector, (uint16_t)count, disk_io_complete, 0)) {
        msc_volume_busy[slot] = false;
        return RES_ERROR;
    }
    return wait_for_disk_io(dev_addr);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    uint8_t const dev_addr = deviceOf(pdrv);
    if (dev_addr == 0) return RES_NOTRDY;
    uint8_t const lun = 0;
    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *((DWORD *)buff) = (WORD)tuh_msc_get_block_count(dev_addr, lun);
            return RES_OK;
        case GET_SECTOR_SIZE:
            *((WORD *)buff) = (WORD)tuh_msc_get_block_size(dev_addr, lun);
            return RES_OK;
        case GET_BLOCK_SIZE:
            *((DWORD *)buff) = 1;  // 1 sector
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

// ***************************************************************************************
//
//      Date        Revision
//      ====        ========
//
// ***************************************************************************************
