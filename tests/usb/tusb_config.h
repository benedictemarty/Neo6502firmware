// Configuration minimale de TinyUSB pour test_msc (PC, pile hôte MSC seule)
#pragma once
#define CFG_TUSB_MCU            OPT_MCU_NONE
#define CFG_TUSB_OS             OPT_OS_NONE
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_HOST
#define CFG_TUH_ENABLED         1
#define CFG_TUH_MAX_SPEED       OPT_MODE_FULL_SPEED
#define CFG_TUH_DEVICE_MAX      5
#define CFG_TUH_MSC             1
