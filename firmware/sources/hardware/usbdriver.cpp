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

// T-40 (withdrawn in 0.10.12) : lighting the Caps/Num/Scroll Lock LEDs means sending a HID output
// report, which is a control transfer. Issued from tuh_hid_mount_cb and from the report callback —
// that is, from inside tuh_task — it wrecked the USB host stack on the board : from 0.10.5 on, every
// program died shortly after start (legdiag, NeoLegacy). Found by bissection with bmarty, 2026-09-23.
// This is the ground T-31 describes : on the RP2040 TinyUSB shares endpoint EPX between the MSC bulk
// transfers and the HID interrupt ones. The lock state itself stays (keyboard.cpp) : Function 2,23
// reads it and T-41 needs it to read the keys. Only the LEDs are gone, to be done again safely
// (request queued and sent from the main loop, outside any callback).

void KBDLockLEDUpdate(uint8_t locks) {
    (void)locks;                                                                // No HID output report : see above
}

static void usbProcessReport(uint8_t const *report) {

    for (int i = 0;i < KBD_MAX_KEYCODE;i++) lastReport[i] = -lastReport[i];     // So if -ve was present last time.
    for (int i = 2;i < 8;i++) {                                                 // Scan the key press array.        
        uint8_t key = report[i];                                            // Raw HID code : the keypad and the
                                                                                // lock keys are handled by KBDEvent
        // if (key == KEY_102ND) key = KEY_BACKSLASH;                           // (T-41). Non US /| mapped.

        if ((report[0] & REBOOT_KEYS) == REBOOT_KEYS) {                         // Ctrl+Alt+AltGr
            ResetSystem();
        }
        
        if (key != 0 && key < KBD_MAX_KEYCODE) {                                // If key is down, and not too high.
            if (lastReport[key] == 0) KBDEvent(1,key,report[0]);                // It wasn't down before so key press.
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
