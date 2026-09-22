// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      keyboard.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//									Angel Sancho
//      Date :      21st November 2023
//      Reviewed :  No
//      Purpose :   Converts keyboard events to a queue and key state array.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/kbdcodes.h"

#define MAX_QUEUE_SIZE (64) 													// Max size of keyboard queue.
#define MAX_FKEY_SIZE (48)   													// Max length of function key.
#define QUEUE_MASK (MAX_QUEUE_SIZE-1)

//
//		Bit patterns for the key states. These represent the key codes (see kbdcodes.h)
//		at 8 states per byte, so keycode 0 is byte 0 bit 0, keycode 7 is byte 0 bit 7
//		keycode 8 is byte 1 bit 0 etc.
//
static uint8_t keyboardState[KBD_MAX_KEYCODE+1];
static uint8_t keyboardModifiers;
//
//		Queue of ASCII keycode presses.
//
static uint8_t queue[MAX_QUEUE_SIZE+1];
static volatile uint8_t queueHead = 0;
static volatile uint8_t queueTail = 0;

static uint8_t currentASCII = 0,currentKeyCode = 0; 							// Current key pressed.
static uint32_t nextRepeat = 9999;  											// Time of next repeat.

static uint8_t KBDMapToASCII(uint8_t keyCode,uint8_t modifiers);
static uint8_t KBDDefaultASCIIKeys(uint8_t keyCode,uint8_t isShift);
static uint8_t KBDDefaultControlKeys(uint8_t keyCode,uint8_t isShift);
static void KBDFunctionKey(uint8_t funcNum,uint8_t modifiers);

// ***************************************************************************************
//
//					Handle a key event. Note keyCode = 0xFF is reset
//
// ***************************************************************************************

void KBDEvent(uint8_t isDown,uint8_t keyCode,uint8_t modifiers) {

	if (isDown && keyCode == KEY_ESC) {   										// Pressed ESC
		cpuMemory[controlPort+3] |= 0x80;  										// Set that flag.
	}

	if (keyCode == 0xFF) { 														// Reset request
		queueHead = 0; 															// Empty keyboard queue
		queueTail = 0; 															// Empty keyboard queue
		for (unsigned int i = 0;i < sizeof(keyboardState);i++) {   				// No keys down.
			keyboardState[i] = 0; 
		}
		return;
	}

	if (keyCode != 0 && keyCode < KBD_MAX_KEYCODE) { 							// Legitimate keycode.
		if (isDown) {
			keyboardState[keyCode] = 0xFF; 										// Set down flag.
			keyboardModifiers = modifiers;										// Copy modifiers
			if (keyCode >= KEY_F1 && keyCode < KEY_F1+10) {  					// Function key : hotkey text (2,4)
				KBDFunctionKey(keyCode - KEY_F1 + 1,modifiers);  				// on key DOWN only (KBDMapToASCII is
			}  																	// also called on key up for the event)
			uint8_t ascii = KBDMapToASCII(keyCode,modifiers);  					// What key ?
			if (ascii != 0) {
				currentASCII = ascii;  											// Remember code and time.
				currentKeyCode = keyCode;
				KBDInsertQueue(ascii);  										// Push in the queue
				nextRepeat = TMRRead()+KBD_REPEAT_START;
			}
			EVTPostKey(EVT_KEYDOWN,ascii,keyCode,modifiers);  					// F-42 : event manager
		} else {
			EVTPostKey(EVT_KEYUP,KBDMapToASCII(keyCode,keyboardModifiers),keyCode,keyboardModifiers);
			keyboardState[keyCode] = 0x00; 										// Clear flag
			keyboardModifiers = 0x00;											// Clear Modifiers
			if (keyCode == currentKeyCode) currentASCII = 0; 					// Autorepeat off, key released.
		}
	}
}

// ***************************************************************************************
//
//								Handle Repeat
//
// ***************************************************************************************

void __time_critical_func(KBDCheckTimer)(void) {
	if (currentASCII != 0) {  													// Key pressed
		if (TMRRead() >= nextRepeat) {  										// Time up ?
			KBDInsertQueue(currentASCII);  										// Put in queue
			nextRepeat = TMRRead()+KBD_REPEAT_AFTER; 							// Quicker repeat after first time.
			EVTPostKey(EVT_AUTOKEY,currentASCII,currentKeyCode,keyboardModifiers);  // F-42
		}
	}

//	int ascii = getchar_timeout_us(0);
//	if (ascii != PICO_ERROR_TIMEOUT) {
//		KBDInsertQueue(ascii);
//	}
}

// ***************************************************************************************
//
//								Access the keyboard state
//
// ***************************************************************************************

uint8_t *KBDGetStateArray(void) {
	return keyboardState;
}

uint8_t KBDGetModifiers(void) {
	return keyboardModifiers;
};

// ***************************************************************************************
//
//						Insert ASCII character into keyboard queue
//
// ***************************************************************************************

void KBDInsertQueue(uint8_t ascii) {
	uint8_t next = (queueTail + 1) & QUEUE_MASK;
    if (next == queueHead) return;        // full
    queue[queueTail] = ascii;
    queueTail = next;
}

// ***************************************************************************************
//
//							  Key available in ASCII queue
//
// ***************************************************************************************

bool KBDIsKeyAvailable(void) {
	return (queueHead != queueTail);
}

// ***************************************************************************************
//
//						Dequeue first key, or 0 if no key available
//
// ***************************************************************************************

uint8_t KBDGetKey(void) {
	if (queueHead == queueTail) return 0;            // empty
    uint8_t key = queue[queueHead];
    queueHead = (queueHead + 1) & QUEUE_MASK;
    return key;
}

// ***************************************************************************************
//
//							Map scancode and modifier to ASCII
//
// ***************************************************************************************

static uint8_t KBDMapToASCII(uint8_t keyCode,uint8_t modifiers) {
	uint8_t ascii = 0;
	uint8_t isShift = (modifiers & KEY_SHIFT) != 0;
	uint8_t isControl = (modifiers & KEY_CONTROL) != 0;

	if (keyCode >= KEY_A && keyCode < KEY_A+26) { 								// Handle alphabet.
		ascii = keyCode - KEY_A + 'A';  										// Make ASCII
		if (!isShift) ascii += 'a'-'A'; 										// Handle shift
		if (isControl) ascii &= 0x1F; 											// Handle control
	}

	if (ascii == 0 && keyCode >= KEY_1 && keyCode < KEY_1+10) { 				// Handle numbers - slightly mangled.
		ascii = (keyCode == KEY_1+9) ? '0' : keyCode - KEY_1 + '1'; 			// because the order on the keyboard isn't 0-9!
		if (isShift) ascii = ")!@#$%^&*("[ascii - '0']; 						// Standard US mapping. of the top row
	}

	if (ascii == 0) {															// This maps all the other ASCII keys.
		ascii = KBDDefaultASCIIKeys(keyCode,modifiers); 						
	}

	if (ascii == 0) {															// This maps all the control keys
		ascii = KBDDefaultControlKeys(keyCode,modifiers); 						
	}

	return LOCLocaleMapping(ascii,keyCode,modifiers); 							// Special mapping for locales.
}

// ***************************************************************************************
//
//						Work out ASCII keys (including space)
//
// ***************************************************************************************

#define KEY(code,normal,shifted) code,normal,shifted

static const uint8_t defaultShift[] = {
	KEY(KEY_MINUS,'-','_'),			KEY(KEY_EQUAL,'=','+'), 		KEY(KEY_LEFTBRACE,'[','{'), 
	KEY(KEY_RIGHTBRACE,']','}'), 	KEY(KEY_BACKSLASH,'\\','|'), 	KEY(KEY_HASHTILDE,'#','~'),
	KEY(KEY_SEMICOLON,';',':'), 	KEY(KEY_APOSTROPHE,'\'','"'), 	KEY(KEY_GRAVE,'`','~'),
	KEY(KEY_COMMA,',','<'), 		KEY(KEY_DOT,'.','>'), 			KEY(KEY_SLASH,'/','?'),
	KEY(KEY_SPACE,' ',' '),			KEY(KEY_102ND,'\\','|'),
	0	
};

static uint8_t KBDDefaultASCIIKeys(uint8_t keyCode,uint8_t isShift) {
	uint8_t ascii = 0;
	uint8_t index = 0;
	while (defaultShift[index] != 0 && defaultShift[index] != keyCode) 			// Look up in table of standard mappings (US)
		index += 3;
	if (defaultShift[index] == keyCode) {  										// found a match.
		ascii = isShift ? defaultShift[index+2] : defaultShift[index+1]; 	
	}
	return ascii;
}

// ***************************************************************************************
//
//						Work out standard controls (include CHR(127))
//
// ***************************************************************************************

static const uint8_t defaultControlKeys[] = {
	KEY_LEFT,CC_LEFT,KEY_RIGHT,CC_RIGHT,KEY_INSERT,CC_INSERT,
	KEY_PAGEDOWN,CC_PAGEDOWN,KEY_END,CC_END,KEY_DELETE,CC_DELETE,
	KEY_TAB,CC_TAB,KEY_ENTER,CC_ENTER,KEY_PAGEUP,CC_PAGEUP,KEY_DOWN,CC_DOWN,
	KEY_HOME,CC_HOME,KEY_UP,CC_UP,KEY_ESC,CC_ESC, 
	KEY_BACKSPACE, CC_BACKSPACE, 0
};	

static uint8_t KBDDefaultControlKeys(uint8_t keyCode,uint8_t isShift) {
	uint8_t index = 0;
	while (defaultControlKeys[index] != 0) {
		if (defaultControlKeys[index] == keyCode) {
			return defaultControlKeys[index+1];
		}
		index += 2;
	} 
	return 0;
}	

// ***************************************************************************************
//
//								Define function keys
//
// ***************************************************************************************

static char functionKeyText[10*(MAX_FKEY_SIZE+1)] = {0};

#define FKEYTEXT(n) (functionKeyText + (n-1)*(MAX_FKEY_SIZE+1))

uint8_t KBDSetFunctionKey(int fKey,const char *keyText) {
	if (strlen(keyText) <= MAX_FKEY_SIZE && fKey >= 1 && fKey <= 10) {
		strcpy(FKEYTEXT(fKey),keyText);
		return 0;
	}
	return 1;
}

// ***************************************************************************************
//
//								Process function keys
//
// ***************************************************************************************

static void KBDFunctionKey(uint8_t funcNum,uint8_t modifiers) {
	const char *txt = FKEYTEXT(funcNum);
	while (*txt != '\0') {
		KBDInsertQueue(*txt++);		
	}
}


// ***************************************************************************************
//
//								   List function keys
//
// ***************************************************************************************

void KBDShowFunctionKeys(void) {
	for (int i = 1;i <= 10;i++) {
		char *k = FKEYTEXT(i);
		if (*k != '\0') {
			CONWriteString("F%-2d : \"", i);
			while (*k != '\0') {
				if (*k == 13) {
					CONWriteString("<Enter>");
				} else {
					CONWrite(*k);
				}
				k++;
			}
			CONWrite(34);CONWrite(13);
		}
	}	
}


// ***************************************************************************************
//
//							Read the keyboard controller
//
// ***************************************************************************************

uint8_t KBDKeyboardController(void) {
	uint8_t ck = 0;
	if (keyboardState[4]|keyboardState[80]) ck |= 0x01; 						// A/Left bit 0
	if (keyboardState[7]|keyboardState[79]) ck |= 0x02; 						// D/Right bit 1
	if (keyboardState[26]|keyboardState[82]) ck |= 0x04; 						// W/Up bit 2
	if (keyboardState[22]|keyboardState[81]) ck |= 0x08; 						// S/Down bit 3
	if (keyboardState[18]|keyboardState[29]) ck |= 0x10; 						// O/Z bit 4 [A]
	if (keyboardState[19]|keyboardState[27]) ck |= 0x20; 						// P/X bit 5 [B]
	if (keyboardState[14]|keyboardState[6]) ck |= 0x40; 						// K/C bit 6 [X]
	if (keyboardState[15]|keyboardState[25]) ck |= 0x80; 						// L/V bit 7 [Y]
	
	return ck;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		07-02-24 	Added ability to list function keys
//		18-02-24	Keys WASDOP now default keys, also cursor keys.
//		17-03-24 	Added ZX control keys.
//		25-03-24 	Extended to support ABXY in the basic API structure.
//		13-03-26  Optimized keyboard queue
//	
// ***************************************************************************************

// T-28 : keyboard presence (the board reports the HID mount ; the emulators always have one).
#ifdef PICO
static bool keyboardPresent = false;
#else
static bool keyboardPresent = true;
#endif
void KBDSetPresent(bool present) { keyboardPresent = present; }
bool KBDIsPresent(void) { return keyboardPresent; }

// ***************************************************************************************
//
//		T-39 : media keys of a USB keyboard. Many keyboards (e.g. 1A2C:0B2A) expose a second
//		HID interface with no boot protocol, carrying a Consumer Control collection (usage
//		page $0C : volume, play, browser...) and often a System Control one (sleep, power,
//		wake). It is claimed here — not by the gamepad manager, which only had "no driver
//		found" to say about it. The last key pressed is kept for the 6502 (Function 2,22).
//
// ***************************************************************************************

#define KBD_MEDIA_MAX 4                                                         // Claimed interfaces
static uint16_t mediaKey[KBD_MEDIA_MAX];                                        // Unused : 0
static uint8_t mediaDev[KBD_MEDIA_MAX],mediaInst[KBD_MEDIA_MAX],mediaUsed = 0;
static uint16_t lastMediaKey = 0;                                               // Last consumer usage, cleared when read
static uint8_t lastSystemKey = 0;                                               // Bits : 1 sleep, 2 power, 4 wake

// The descriptor is claimed when it starts a Consumer Control or System Control collection.
bool KBDMediaClaim(uint8_t dev_addr,uint8_t instance,const uint8_t *desc,uint16_t len) {
	bool isMedia = false;
	for (uint16_t i = 0;i + 3 < len;i++) {
		if (desc[i] == 0x05 && desc[i+1] == 0x0C && desc[i+2] == 0x09 && desc[i+3] == 0x01) isMedia = true;   // Consumer Control
		if (desc[i] == 0x05 && desc[i+1] == 0x01 && desc[i+2] == 0x09 && desc[i+3] == 0x80) isMedia = true;   // System Control
	}
	if (!isMedia || mediaUsed >= KBD_MEDIA_MAX) return false;
	mediaDev[mediaUsed] = dev_addr;mediaInst[mediaUsed] = instance;mediaKey[mediaUsed] = 0;
	mediaUsed++;
	return true;
}

void KBDMediaRelease(uint8_t dev_addr,uint8_t instance) {
	for (int i = 0;i < mediaUsed;i++) {
		if (mediaDev[i] == dev_addr && mediaInst[i] == instance) {
			for (int j = i;j + 1 < mediaUsed;j++) { mediaDev[j] = mediaDev[j+1];mediaInst[j] = mediaInst[j+1];mediaKey[j] = mediaKey[j+1]; }
			mediaUsed--;
			return;
		}
	}
}

// Report of a claimed interface : ID 1 = consumer usage (16 bits, 0 = released), ID 2 = system bits.
bool KBDMediaReport(uint8_t dev_addr,uint8_t instance,const uint8_t *report,uint16_t len) {
	bool mine = false;
	for (int i = 0;i < mediaUsed;i++) if (mediaDev[i] == dev_addr && mediaInst[i] == instance) mine = true;
	if (!mine || len < 2) return mine;
	if (report[0] == 1 && len >= 3) {  											// Consumer : keep the press, ignore the release
		uint16_t usage = report[1] | (report[2] << 8);
		if (usage != 0) lastMediaKey = usage;
	} else if (report[0] == 2) {
		if (report[1] != 0) lastSystemKey = report[1];
	}
	return true;
}

void KBDGetMediaKey(uint16_t *usage,uint8_t *system) {  							// 2,22 : reading clears them
	*usage = lastMediaKey;*system = lastSystemKey;
	lastMediaKey = 0;lastSystemKey = 0;
}
