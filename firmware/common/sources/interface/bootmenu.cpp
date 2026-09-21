// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      bootmenu.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      19th September 2026
//      Purpose :   Trinity boot menu (see bootmenu.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static char bootNames[BOOT_MAX][BOOT_NAME_MAX+1];
static uint8_t bootCount = 0;
static int8_t bootChoice = -1;                                                  // -1 none/NeoDOS, 0.. index in bootNames

static bool _BOOTEndsWith(const char *s,const char *suffix) {
    size_t l = strlen(s),m = strlen(suffix);
    if (l < m) return false;
    for (size_t i = 0;i < m;i++) if (tolower(s[l-m+i]) != suffix[i]) return false;
    return true;
}

void BOOTSelect(void) {
    bootCount = 0;bootChoice = -1;
    if (FIOOpenDir(BOOT_DIR) != 0) return;                                      // No boot directory : no menu
    std::string name;uint32_t size;uint8_t attribs;
    while (bootCount < BOOT_MAX && FIOReadDir(name,&size,&attribs) == 0) {
        if (name.size() == 0 || name[0] == '.' || name.size() > BOOT_NAME_MAX) continue;
        if (!_BOOTEndsWith(name.c_str(),".neo") && !_BOOTEndsWith(name.c_str(),".bin")) continue;
        strcpy(bootNames[bootCount++],name.c_str());
    }
    FIOCloseDir();
    if (bootCount == 0) return;
    int8_t autoChoice = -1;                                                     // boot/auto.txt : name of the entry to start at once
    {
        uint8_t exists = 0;
        if (FIOExistsFile(BOOT_DIR "/" BOOT_AUTO,&exists) == 0 && exists) {
            uint8_t *buf = cpuMemory + 0xFE00;                                  // Scratch below the kernel, before the 6502 runs
            memset(buf,0,BOOT_NAME_MAX + 2);
            if (FISOpenFileHandle(0,BOOT_DIR "/" BOOT_AUTO,FIOMODE_RDONLY) == 0) {
                uint16_t n = BOOT_NAME_MAX + 1;
                FISReadFileHandle(0,0xFE00,&n);
                FISCloseFileHandle(0);
                buf[BOOT_NAME_MAX + 1] = 0;
                for (uint8_t *q = buf;*q;q++) if (*q == '\r' || *q == '\n' || *q == ' ') { *q = 0;break; }
                for (int i = 0;i < bootCount;i++) if (strcasecmp((char *)buf,bootNames[i]) == 0) autoChoice = i;
            }
            memset(buf,0,BOOT_NAME_MAX + 2);
        }
    }
    if (autoChoice >= 0) {                                                      // Auto : start it unless Escape within 1 s
        CONWriteString("Boot : auto %s (Esc = menu, 3 s)\r",bootNames[autoChoice]);
        bootChoice = autoChoice;
        uint32_t end = TMRRead() + BOOT_AUTO_TIMEOUT;
        bool menu = false;
        while ((int32_t)(end - TMRRead()) > 0) {
            KBDSync();
            if (KBDGetKey() == 27) { menu = true;break; }
        }
        if (!menu) { CONWriteString("-> %s\r",bootNames[bootChoice]);return; }
        bootChoice = -1;
    }
    CONWriteString("Boot : 1 NeoDOS");
    for (int i = 0;i < bootCount;i++) CONWriteString("  %d %s",i + 2,bootNames[i]);
    CONWriteString("\r");
    uint32_t end = TMRRead() + BOOT_TIMEOUT;
    while ((int32_t)(end - TMRRead()) > 0) {
        KBDSync();
        uint8_t key = KBDGetKey();
        if (key == 13) break;                                                   // Enter : default
        if (key >= '1' && key < '2' + bootCount) { bootChoice = key - '2';break; }   // '1' = NeoDOS (-1)
    }
    CONWriteString("-> %s\r",bootChoice < 0 ? "NeoDOS" : bootNames[bootChoice]);
}

// Load the boot choice at the first 1,3 after reset : a .bin goes to $800 (NeoBASIC image, boot/neobasic.bin), a .neo
// through the normal loader (JMP exec written at $FF08 by FIOReadFile, the kernel's jmp (0) goes there).
bool BOOTLoadChoice(void) {
    if (bootChoice < 0) return false;
    std::string path = std::string(BOOT_DIR) + "/" + bootNames[bootChoice];
    bool isBin = _BOOTEndsWith(bootNames[bootChoice],".bin");
    bootChoice = -1;                                                            // Once only : later 1,3 go back to NeoDOS
    if (isBin) {
        if (FIOReadFileBasic(path,0x0800) != 0) return false;                    // NeoBASIC load address
        cpuMemory[0] = 0x00;cpuMemory[1] = 0x08;
        return true;
    }
    uint8_t *cmd = cpuMemory + DEFAULT_PORT;
    if (FIOReadFile(path,0xFFFF,cmd) != 0 || cmd[8] != 0x4C) {                 // Unreadable or no exec address : back to NeoDOS
        CONWriteString("Boot : %s not started (load error or no exec address), NeoDOS\r",path.c_str());
        return false;
    }
    cpuMemory[0] = (DEFAULT_PORT + 8) & 0xFF;cpuMemory[1] = (DEFAULT_PORT + 8) >> 8;   // jmp (0) -> JMP exec at $FF08
    return true;
}
