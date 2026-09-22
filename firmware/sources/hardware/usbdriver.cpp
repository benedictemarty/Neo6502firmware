// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      usbdriver.cpp
//      Authors :   Tsvetan Usunov (Olimex)
//                  Paul Robson (paul@robsons.org.uk)
//                  Sascha Schneider
//                  Angel Sancho
//      Date :      20th November 2023
//      Reviewed :  No
//      Purpose :   USB interface and HID->Event mapper.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "tusb.h"
#include "interface/kbdcodes.h"
#include "interface/mouse.h"

#include "GamepadController.h"

#include <cstdint>

// ***************************************************************************************
//
//                          Process USB HID Keyboard Report
//
//                  This converts it to a series of up/down key events
//
// ***************************************************************************************

static short lastReport[KBD_MAX_KEYCODE] = { 0 };                               // state at last HID report.

// T-40 : lock keys and their LEDs. The upstream never sent the HID output report, so Caps/Num/Scroll
// Lock never lit up. State kept here, sent to every keyboard interface on change (bmarty, board 2026-09-22).
#define HID_KEY_CAPSLOCK   0x39
#define HID_KEY_SCROLLLOCK 0x47
#define HID_KEY_NUMLOCK    0x53
#define HID_LED_NUM        0x01
#define HID_LED_CAPS       0x02
#define HID_LED_SCROLL     0x04

static uint8_t kbdLeds = 0;                                                     // Bits : num, caps, scroll
static uint8_t kbdLedDev = 0xFF,kbdLedInst = 0;                                 // Keyboard interface to talk to

static void usbSendLeds(void) {
    if (kbdLedDev == 0xFF) return;
    static uint8_t leds;                                                        // Must outlive the call (async transfer)
    leds = kbdLeds;
    tuh_hid_set_report(kbdLedDev,kbdLedInst,0,HID_REPORT_TYPE_OUTPUT,&leds,1);
}

static void usbLockKey(uint8_t key) {                                           // Toggle on key down
    if (key == HID_KEY_CAPSLOCK) kbdLeds ^= HID_LED_CAPS;
    else if (key == HID_KEY_NUMLOCK) kbdLeds ^= HID_LED_NUM;
    else if (key == HID_KEY_SCROLLLOCK) kbdLeds ^= HID_LED_SCROLL;
    else return;
    usbSendLeds();
}

uint8_t KBDGetLocks(void) { return kbdLeds; }                                   // 2,23

static void usbProcessReport(uint8_t const *report) {

    for (int i = 0;i < KBD_MAX_KEYCODE;i++) lastReport[i] = -lastReport[i];     // So if -ve was present last time.
    for (int i = 2;i < 8;i++) {                                                 // Scan the key press array.        
        uint8_t key = report[i];
        if (key >= KEY_KP1 && key < KEY_KP1+10) {                               // Numeric keypad numbers will work.
            key = key - KEY_KP1 + KEY_1;
        }
        // if (key == KEY_102ND) key = KEY_BACKSLASH;                           // Non US /| mapped.

        if ((report[0] & REBOOT_KEYS) == REBOOT_KEYS) {                         // Ctrl+Alt+AltGr
            ResetSystem();
        }
        
        if (key != 0 && key < KBD_MAX_KEYCODE) {                                // If key is down, and not too high.
            if (lastReport[key] == 0) { KBDEvent(1,key,report[0]);usbLockKey(report[i]); }   // Press (T-40 : lock keys drive the LEDs)
            lastReport[key] = 1;                                                // Flag it as now being down.
        }
    } 

    for (int i = 0;i < KBD_MAX_KEYCODE;i++) {                                   // Any remaining -ve keys are up actions.
        if (lastReport[i] < 0) {
            KBDEvent(0,i,0);                                                    // Flag going up.
            lastReport[i] = 0;                                                  // Mark as now up
        }
    }
}

static void usbProcessMouseReport(uint8_t const *report, uint16_t len) {
    if(len < 3) return;
    MSEOffsetPosition(report[1], report[2]);
    MSEUpdateButtonState(report[0]);

    if(len < 4) return;
    MSEUpdateScrollWheel(report[3]);
}

// ***************************************************************************************
//
//                              USB Callback functions
//
// ***************************************************************************************

static GamepadController gamepad_controller;

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    USBNoteEvent();                                                             // T-32 : enumeration barrier
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    switch(tuh_hid_interface_protocol(dev_addr, instance)) {

    case HID_ITF_PROTOCOL_KEYBOARD:
        KBDSetPresent(true);                                                    // Boot menu waits for it (T-28)
        kbdLedDev = dev_addr;kbdLedInst = instance;                             // T-40 : where to send the LED report
        usbSendLeds();
        CONWriteString("USB keyboard found\r");
        break;

    case HID_ITF_PROTOCOL_MOUSE:
        MSEEnableMouse();
        break;

    case HID_ITF_PROTOCOL_NONE:
        if (KBDMediaClaim(dev_addr,instance,desc_report,desc_len)) {            // T-39 : consumer / system control interface of a
            CONWriteString("USB media keys found\r");                           // keyboard (e.g. 1A2C:0B2A) — not a gamepad
            break;
        }
        gamepad_controller.add(vid, pid, dev_addr, instance, desc_report, desc_len);
        break;
    }
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    USBNoteEvent();                                                             // T-32
    KBDMediaRelease(dev_addr,instance);                                         // T-39
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

    switch(tuh_hid_interface_protocol(dev_addr, instance)) {
    case HID_ITF_PROTOCOL_KEYBOARD:
        usbProcessReport(report);
        break;

    case HID_ITF_PROTOCOL_MOUSE:
        usbProcessMouseReport(report, len);
        break;

    case HID_ITF_PROTOCOL_NONE:
        if (KBDMediaReport(dev_addr,instance,report,len)) break;                // T-39 : media / system keys
        gamepad_controller.update(dev_addr, instance, report, len);
        break;
    }
    tuh_hid_receive_report(dev_addr, instance);
}

// ***************************************************************************************
//
//                               Keyboard initialisation
//
// ***************************************************************************************


void KBDInitialise(void) {
    for (int i = 0;i < KBD_MAX_KEYCODE;i++) lastReport[i] = 0;                  // No keys currently known
    tusb_init();
}

// ***************************************************************************************
//
//                            Gamepad Controller information
//
// ***************************************************************************************

uint8_t GMPGetControllerCount(void) {
    return gamepad_controller.getCount();
}

uint32_t GMPReadDigitalController(uint8_t index) {
    return gamepad_controller.readDigital(index);
}

// ***************************************************************************************
//
//                                  Keyboard polling
//
// ***************************************************************************************

// T-30 : after a flash write (interrupts off on both cores, ~50-150 ms) the USB host controller has missed its
// interrupts : pump the stack hard for a moment so TinyUSB catches up (endpoints resumed, reports flowing again).
void HWUSBRecover(void) {
    uint32_t end = TMRRead() + 30;                                              // 300 ms
    while ((int32_t)(end - TMRRead()) > 0) tuh_task_ext(0,false);
}

void __time_critical_func(KBDSync)(void) {
    if (tuh_task_event_ready()) {
      tuh_task_ext(0, false);
    }
    
    KBDCheckTimer();
    
}

// ***************************************************************************************
//
//      Date        Revision
//      ====        ========
//		 13-03-26     Optimized tuh_task call
//
// ***************************************************************************************
