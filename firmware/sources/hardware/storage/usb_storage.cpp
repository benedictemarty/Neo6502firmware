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

static FATFS msc_fatfs_volumes[CFG_TUH_DEVICE_MAX];
static volatile bool msc_volume_busy[CFG_TUH_DEVICE_MAX];
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

void STOSynchronise(void) {
    CONWriteString("USB Storage\r");
    uint16_t timeOut = 2000;
    while (!msc_inquiry_complete && timeOut > 0) {
        KBDSync();
        sleep_us(1000);
        timeOut--;
    }
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
    char drive_path[3] = "0:";
    drive_path[0] += drive;
    FRESULT result = f_mount(&msc_fatfs_volumes[dev_addr], drive_path, 1);
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
    uint8_t const lun = 0;
    //CONWriteString("MSC mounted, inquiring\r\n");
    tuh_msc_inquiry(dev_addr, lun, &msc_inquiry_resp, inquiry_complete_cb, 0);
}

void tuh_msc_umount_cb(uint8_t dev_addr) {
    int drive = driveOfDevice(dev_addr);
    if (drive < 0) return;
    char drive_path[3] = "0:";
    drive_path[0] += drive;
    f_unmount(drive_path);
    driveDevice[drive] = 0;
}

// ***************************************************************************************
//
//                                  sInterface to FATFS
//
// ***************************************************************************************

static void wait_for_disk_io(BYTE pdrv) {
    while (msc_volume_busy[pdrv]) {
        tuh_task();
    }
}

static bool disk_io_complete(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data) {
    (void)cb_data;
    msc_volume_busy[dev_addr] = false;
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

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    uint8_t const dev_addr = deviceOf(pdrv);
    if (dev_addr == 0) return RES_NOTRDY;
    uint8_t const lun = 0;
    msc_volume_busy[pdrv] = true;
    tuh_msc_read10(dev_addr, lun, buff, sector, (uint16_t)count, disk_io_complete, 0);
    wait_for_disk_io(pdrv);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    uint8_t const dev_addr = deviceOf(pdrv);
    if (dev_addr == 0) return RES_NOTRDY;
    uint8_t const lun = 0;
    msc_volume_busy[pdrv] = true;
    tuh_msc_write10(dev_addr, lun, buff, sector, (uint16_t)count, disk_io_complete, 0);
    wait_for_disk_io(pdrv);
    return RES_OK;
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
