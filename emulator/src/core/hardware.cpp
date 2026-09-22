// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		hardware.c
//		Purpose:	Hardware Emulation
//		Created:	22nd November 2023
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include "gfx.h"
#include "sys_processor.h"
#include <time.h>
#include <string.h>
#include "sys_debug_system.h"
#include "hardware.h"
#include <stdio.h>
#include "common.h"
#include "interface/kbdcodes.h"
#include "sys/stat.h"
#include "sys/types.h"
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <vector>
#include "serial_emu.h"

#define DEFAULT_STORAGE "storage"

static const SerialInterface* serialInterface = NULL;
static FILE* fileHandles[FIO_NUM_FILES];
static int frameCount = 0;
static std::filesystem::path storagePath = DEFAULT_STORAGE;
static std::filesystem::path currentPath = storagePath;

// *******************************************************************************************************************************
//
//												Storage path management
//
// *******************************************************************************************************************************

// bmarty F-102 : volumes. Volume 0 is the storage directory ; volumes 1..3 are the
// sibling directories "<storage>1".."<storage>3" when they exist (paths "n:..." select them).

static int currentVolume = 0;
static std::filesystem::path volumeCurrentPath[FIO_MAX_VOLUMES];

static std::filesystem::path volumeRoot(int volume) {
	if (volume == 0) return storagePath;
	return std::filesystem::path(storagePath.string() + std::to_string(volume));
}

static bool volumePresent(int volume) {
	return volume >= 0 && volume < FIO_MAX_VOLUMES && std::filesystem::is_directory(volumeRoot(volume));
}

static std::filesystem::path &volumeCwd(int volume) {  								// Trinity : lazily set to the root (HWSetDefaultPath
	if (volumeCurrentPath[volume].empty()) volumeCurrentPath[volume] = volumeRoot(volume);   // is not called without "path:")
	return volumeCurrentPath[volume];
}

void HWSetDefaultPath(const char *defaultPath) {
	storagePath = defaultPath;
	currentPath = defaultPath;
	currentVolume = 0;
	for (int i = 0;i < FIO_MAX_VOLUMES;i++) volumeCurrentPath[i] = volumeRoot(i);
}

static int pathVolume(const std::string& path) {										// volume addressed by a path
	if (path.size() >= 2 && path[1] == ':' && isdigit((unsigned char)path[0])) return path[0] - '0';
	return currentVolume;
}

static std::string getAbspath(const std::string& path) {
	std::filesystem::path newPath;
	int volume = currentVolume;
	std::string rest = path;
	if (rest.size() >= 2 && rest[1] == ':' && isdigit((unsigned char)rest[0])) {	// "n:" prefix (FatFs style)
		volume = rest[0] - '0';
		rest = rest.substr(2);
		if (!volumePresent(volume)) return (volumeRoot(volume) / "?").string();	// -> not found
	}
	std::filesystem::path root = volumeRoot(volume);
	std::filesystem::path cwd = (volume == currentVolume) ? currentPath : volumeCwd(volume);
	if (!rest.empty() && (rest[0] == '/'))
		newPath = root / rest.substr(1);
	else
		newPath = cwd / rest;
	return newPath.string();
}

uint8_t FISGetVolumeInfo(uint8_t volume, std::string& name, uint8_t* attribs) {
	if (!volumePresent(volume)) return FIOERROR_INVALID_DRIVE;
	name = "HOST" + std::to_string(volume);
	*attribs = FIOVOL_PRESENT;
	return FIOERROR_OK;
}

uint8_t FISSelectVolume(uint8_t volume) {
	if (!volumePresent(volume)) return FIOERROR_INVALID_DRIVE;
	volumeCurrentPath[currentVolume] = currentPath;									// each volume keeps its cwd
	currentVolume = volume;
	currentPath = volumeCwd(volume);
	return FIOERROR_OK;
}

uint8_t FISGetCurrentVolume(uint8_t* volume) {
	*volume = currentVolume;
	return FIOERROR_OK;
}

static uint8_t getAttributes(const std::string& filename) {
	using std::filesystem::perms;
	std::filesystem::file_status status = std::filesystem::status(filename);

	return ((status.type() == std::filesystem::file_type::directory) ? FIOATTR_DIR : 0) |
		((status.permissions() & perms::owner_write) == perms::none ? FIOATTR_READONLY : 0);
}

static uint8_t convertError(const std::error_code& errcode) {
	static const std::vector<std::pair<std::errc, FIOErrno>> errorsList = {
		{ std::errc::no_such_device, FIOERROR_INVALID_NAME },
		{ std::errc::no_such_file_or_directory, FIOERROR_NO_FILE },		// bmarty : as FatFs FR_NO_FILE on the board
		{ std::errc::file_exists, FIOERROR_EXIST },
		{ std::errc::permission_denied, FIOERROR_DENIED },
		{ std::errc::is_a_directory, FIOERROR_DENIED },
		{ std::errc::not_a_directory, FIOERROR_NO_PATH },
		{ std::errc::no_space_on_device, FIOERROR_DENIED },
		{ std::errc::filename_too_long, FIOERROR_INVALID_PARAMETER },
		{ std::errc::file_too_large, FIOERROR_DENIED },
		{ std::errc::directory_not_empty, FIOERROR_DENIED },
	};

	if (!errcode) {
		return FIOERROR_OK;
	}

	for (const auto& it : errorsList) {
		if (errcode == it.first) {
			return it.second;
		}
	}
	return FIOERROR_UNKNOWN;
}

static uint8_t convertError(int e) {
	const std::error_code ec = { e, std::system_category() };
	return convertError(ec);
}


// *******************************************************************************************************************************
//
//												Reset Hardware
//
// *******************************************************************************************************************************

void HWReset(void) {
	std::filesystem::create_directories(currentPath);
	MSEEnableMouse();
}

// *******************************************************************************************************************************
//
//											 Frame Sync any hardware
//
// *******************************************************************************************************************************

void KBDLockLEDUpdate(uint8_t locks) { (void)locks; }  							// T-40 : no keyboard LEDs here
void HWClockSet(const CLOCK_TIME *t) { (void)t; }                                 // T-18 (F-14) : no RTC to program here

// T-26 : settings "flash" = storage/settings.flash
static uint8_t settingsImage[SETTINGS_SIZE];
static bool settingsLoaded = false;
const uint8_t *HWSettingsStorage(void) {
	if (!settingsLoaded) {
		memset(settingsImage,0xFF,SETTINGS_SIZE);
		FILE *f = fopen((storagePath / "settings.flash").c_str(),"rb");
		if (f != NULL) { size_t n = fread(settingsImage,1,SETTINGS_SIZE,f);(void)n;fclose(f); }
		settingsLoaded = true;
	}
	return settingsImage;
}
uint8_t HWSettingsWrite(const uint8_t *data) {
	memcpy(settingsImage,data,SETTINGS_SIZE);settingsLoaded = true;
	FILE *f = fopen((storagePath / "settings.flash").c_str(),"wb");
	if (f == NULL) return 1;
	fwrite(settingsImage,1,SETTINGS_SIZE,f);fclose(f);
	return 0;
}

// T-17 : bank "flash" = storage/banks.flash (BANK_COUNT * BANK_SIZE bytes, $FF when absent), persistent like the board's flash.
static uint8_t *bankImage = NULL;
static void bankLoad(void) {
	if (bankImage != NULL) return;
	bankImage = (uint8_t *)malloc(BANK_COUNT * BANK_SIZE);
	memset(bankImage,0xFF,BANK_COUNT * BANK_SIZE);
	FILE *f = fopen((storagePath / "banks.flash").c_str(),"rb");
	if (f != NULL) { size_t n = fread(bankImage,1,BANK_COUNT * BANK_SIZE,f);(void)n;fclose(f); }
}
const uint8_t *HWBankStorage(uint8_t bank) {
	if (bank >= BANK_COUNT) return NULL;
	bankLoad();
	return bankImage + bank * BANK_SIZE;
}
uint8_t HWBankWrite(uint8_t bank,const uint8_t *data) {
	if (bank >= BANK_COUNT) return 1;
	bankLoad();
	memcpy(bankImage + bank * BANK_SIZE,data,BANK_SIZE);
	FILE *f = fopen((storagePath / "banks.flash").c_str(),"wb");
	if (f == NULL) return 1;
	fwrite(bankImage,1,BANK_COUNT * BANK_SIZE,f);fclose(f);
	return 0;
}

void HWSync(void) {
	TICKProcess();
	frameCount++;
	CONBlinkSync();  															// The emulator does not call DSPSync : blink here.
}

int  RNDGetFrameCount(void) {
	return frameCount;
}

// *******************************************************************************************************************************
//
//												Read 100Hz timer
//
// *******************************************************************************************************************************

uint32_t TMRRead(void) {
	return GFXTimer() / 10;
}

// *******************************************************************************************************************************
//
//											Keyboard Sync/Initialise dummies
//
// *******************************************************************************************************************************

void KBDSync(void) { }
void KBDInitialise(void) { }

// *******************************************************************************************************************************
//
//											   Sound system initialise
//
// *******************************************************************************************************************************

void SNDInitialise(void) {
}

// // *******************************************************************************************************************************
// //
// //											   Sound system set pitch
// //
// // *******************************************************************************************************************************

// void SNDUpdateSoundChannel(uint8_t channel,SOUND_CHANNEL *c) {
// 	if (c->isPlayingNote != 0) {
// 		GFXSetFrequency(c->currentFrequency,1);
// 	} else {
// 		GFXSilence();
// 	}
// }

// *******************************************************************************************************************************
//
//										  Receive SDL Keyboard events
//
// *******************************************************************************************************************************

#include "hid2sdl.h"

void HWQueueKeyboardEvent(int sdlCode,int isDown) {
	int found = -1;
	for (int i = 0;i < SDL2HIDSIZE;i++) {
		if (SDL2HIDMapping[i] == sdlCode) found = i;
	}
	if (found >= 0) {
		int modifier = 0;
		if (GFXIsKeyPressed(GFXKEY_SHIFT)) modifier |= KEY_SHIFT;
		if (GFXIsKeyPressed(GFXKEY_CONTROL)) modifier |= KEY_CONTROL;
		if (GFXIsKeyPressed(GFXKEY_ALT)) modifier |= KEY_ALT;
		if (GFXIsKeyPressed(GFXKEY_ALTGR)) modifier |= KEY_ALTGR;
		KBDEvent(isDown,found,modifier);
	}
}

// *******************************************************************************************************************************
//
//												Dummy debug write
//
// *******************************************************************************************************************************

void FDBWrite(uint8_t c) {
	fputc(c,stderr);
}

// ***************************************************************************************
//
//									Rename file
//
// ***************************************************************************************

uint8_t FISRenameFile(const std::string& oldFilename, const std::string& newFilename) {
	std::string oldAbspath = getAbspath(oldFilename);
	std::string newAbspath = getAbspath(newFilename);
	printf("Renaming %s to %s: ", oldAbspath.c_str(), newAbspath.c_str());
	std::error_code ec;
	std::filesystem::rename(oldAbspath, newAbspath, ec);
	if (ec)
		printf("%s\n", ec.message().c_str());
	else
		printf("OK\n");

	return convertError(ec);
}

// ***************************************************************************************
//
//									 Copy file 
//
// ***************************************************************************************

uint8_t FISCopyFile(const std::string& oldFilename, const std::string& newFilename) {
	std::string oldAbspath = getAbspath(oldFilename);
	std::string newAbspath = getAbspath(newFilename);
	printf("FISCopyFile('%s', '%s') -> ", oldAbspath.c_str(), newAbspath.c_str());
	std::error_code ec;
	std::filesystem::copy(oldAbspath, newAbspath,
		std::filesystem::copy_options::overwrite_existing, ec);
	if (ec)
		printf("%s\n", ec.message().c_str());
	else
		printf("OK\n");

	return convertError(ec);
}

// ***************************************************************************************
//
//									Delete file
//
// ***************************************************************************************

uint8_t FISDeleteFile(const std::string& filename) {
	std::string abspath = getAbspath(filename);
	printf("FISDeleteFile('%s') -> ", abspath.c_str());
	std::error_code ec;
	bool removed = std::filesystem::remove(abspath, ec);
	if (ec || !removed) {
		printf("%s\n", ec.message().c_str());
		return convertError(ec);
	} else {
		printf("OK\n");
		return FIOERROR_OK;
	}
}

// ***************************************************************************************
//
//									Create directory
//
// ***************************************************************************************

uint8_t FISCreateDirectory(const std::string& filename) {
	std::string abspath = getAbspath(filename);
	printf("FISCreateDirectory('%s') -> ", abspath.c_str());
	std::error_code ec;
	std::filesystem::create_directory(abspath, ec);
	if (ec)
		printf("%s\n", ec.message().c_str());
	else
		printf("OK\n");

	return convertError(ec);
}

// ***************************************************************************************
//
//									Change directory
//
// ***************************************************************************************

uint8_t FISChangeDirectory(const std::string& filename) {
	std::string abspath = getAbspath(filename);
	printf("FISChangeDirectory('%s') -> ", abspath.c_str());

	std::error_code ec;
	auto status = std::filesystem::status(abspath, ec);

	if (!ec && (status.type() == std::filesystem::file_type::directory)) {
		printf("OK\n");
		int volume = pathVolume(filename);												// "n:" : that volume's cwd, as FatFs does
		if (volume == currentVolume) currentPath = abspath; else volumeCurrentPath[volume] = abspath;
		return FIOERROR_OK;
	} else {
		return convertError(ec);
	}
}

// ***************************************************************************************
//
//								Read current directory
//
// ***************************************************************************************

uint8_t FISGetCurrentDirectory(char *target,int maxSize) {
	std::string root = volumeRoot(currentVolume).string();							// cwd relative to the volume root
	std::string cwd = currentPath.string();
	std::string rel = (cwd.compare(0, root.size(), root) == 0) ? cwd.substr(root.size()) : cwd;
	if (rel.empty() || rel[0] != '/') rel = "/" + rel;
	if (maxSize <= 0) return FIOERROR_INVALID_PARAMETER;
	strncpy(target, rel.c_str(), maxSize);
	target[maxSize-1] = '\0';
	printf("FISGetCurrentDirectory() -> %s\n", target);
	return FIOERROR_OK;
}

// ***************************************************************************************
//
//									   Stat file
//
// ***************************************************************************************

uint8_t FISStatFile(const std::string& filename, uint32_t* length, uint8_t* attribs) {
	std::string abspath = getAbspath(filename);
	printf("FISStatFile('%s') -> ", abspath.c_str());

	try {
		if (!std::filesystem::is_directory(abspath)) {
			*length = std::filesystem::file_size(abspath);
		}
		*attribs = getAttributes(abspath);
		printf("OK; length=0x%04x; permissions=0x%02x\n", *length, *attribs);
		return FIOERROR_OK;
	} catch (const std::filesystem::filesystem_error& e) {
		printf("%s\n", e.what());
		return convertError(e.code());
	}
}

// ***************************************************************************************
//
//									Set file attributes
//
// ***************************************************************************************

uint8_t FISSetFileAttributes(const std::string& filename, uint8_t attribs) {
	std::string abspath = getAbspath(filename);
	printf("FISSetFileAttributes('%s') -> ", abspath.c_str());

	try {
		using std::filesystem::perms;
		std::filesystem::permissions(abspath,
			(std::filesystem::perms)((attribs & FIOATTR_READONLY) ? 0444 : 0666),
			std::filesystem::perm_options::replace);
		printf("OK\n");
		return FIOERROR_OK;
	} catch (const std::filesystem::filesystem_error& e) {
		printf("%s\n", e.what());
		return convertError(e.code());
	}
}

// ***************************************************************************************
//
//								Directory enumeration
//
// ***************************************************************************************

static std::filesystem::directory_iterator readDirIterator;

uint8_t FISOpenDir(const std::string& filename) {
	std::string abspath = getAbspath(filename);
	printf("FISOpenDir('%s') -> ", abspath.c_str());
	errno = 0;
	try {
		readDirIterator = std::filesystem::directory_iterator(abspath);
		printf("OK\n");
		return FIOERROR_OK;
	} catch (const std::filesystem::filesystem_error& e) {
		printf("%s\n", e.what());
		return convertError(e.code());
	}
}

uint8_t FISReadDir(std::string& filename, uint32_t* size, uint8_t* attribs) {
	printf("FISReadDir() -> ");

	if (readDirIterator != std::filesystem::directory_iterator()) {
		const auto& de = *readDirIterator++;

		filename = de.path().filename().string();
		*size = de.is_regular_file() ? de.file_size() : 0;
		*attribs = getAttributes(de.path().string());
		printf("OK: '%s', length=0x%04x, attribus=0x%02x\n", filename.c_str(), *size, *attribs);
		return FIOERROR_OK;
	} else {
		printf("Failed\n");
		return FIOERROR_EOF;
	}
}

uint8_t FISCloseDir() {
	readDirIterator = std::filesystem::directory_iterator();
	return FIOERROR_OK;
}

// ***************************************************************************************
//
//								File-handle based functions
//
// ***************************************************************************************

uint8_t FISOpenFileHandle(uint8_t fileno, const std::string& filename, uint8_t mode) {
	std::string abspath = getAbspath(filename);

	if (fileno >= FIO_NUM_FILES)
		return FIOERROR_INVALID_PARAMETER;

	/* Check if already open. */
	if (fileHandles[fileno])
		return FIOERROR_INVALID_PARAMETER;

	static const char* const modes[] = {
		"rb",	// 0: FIOMODE_RDONLY
		"r+b", 	// 1: FIOMODE_WRONLY
		"r+b",	// 2: FIOMODE_RDWR
		"w+b",	// 3: FIOMODE_RDWR_CREATE
	};
	if (mode >= sizeof(modes)/sizeof(*modes))
		return FIOERROR_INVALID_PARAMETER;

	fprintf(stderr, "FISOpenFileHandle(%d, '%s', 0x%02x) -> ", fileno, abspath.c_str(), mode);
	errno = 0;
	fileHandles[fileno] = fopen(abspath.c_str(), modes[mode]);
	fprintf(stderr, "%s\n", strerror(errno));
	return fileHandles[fileno] ? FIOERROR_OK : convertError(errno);
}

static FILE* getF(uint8_t fileno) {
	if (fileno >= FIO_NUM_FILES)
		return NULL;
	return fileHandles[fileno];
}

uint8_t FISCloseFileHandle(uint8_t fileno) {
	fprintf(stderr, "FISCloseFileHandle(%d)\n", fileno);
	if (fileno == 0xff) {
		for (FILE*& f : fileHandles) {
			if (f) {
				fclose(f);
				f = NULL;
			}
		}
		return FIOERROR_OK;
	} else {
		FILE* f = getF(fileno);
		if (!f)
			return FIOERROR_INVALID_PARAMETER;

		int result = fclose(f);
		fileHandles[fileno] = NULL;
		return !result ? FIOERROR_OK : convertError(errno);
	}
}

uint8_t FISSeekFileHandle(uint8_t fileno, uint32_t offset) {
	printf("FISSeekFileHandle(%d, %u)\n", fileno, offset);
	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	int result = fseek(f, offset, SEEK_SET);
	return (result >= 0) ? FIOERROR_OK : convertError(errno);
}

uint8_t FISTellFileHandle(uint8_t fileno, uint32_t* offset) {

	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	*offset = ftell(f);
	printf("FISTellFileHandle(%d, %d) -> ", fileno, *offset);
	return FIOERROR_OK;
}

uint8_t FISReadFileHandle(uint8_t fileno, uint16_t address, uint16_t* size) {
	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	errno = 0;
	printf("FISReadFileHandle(%d, @0x%x, %d) -> ", fileno, address, *size);
	size_t result;
	if (address != 0xFFFF) {
		result = fread(cpuMemory+address, 1,*size, f);
	} else {
		result = fread(gfxObjectMemory,1,*size,f);
	}
	printf("%d: %s\n", (int)result, (result != *size) ? strerror(errno) : "OK");
	*size = result;

	if ((errno == 0) && (result == 0)) {
		return FIOERROR_EOF;
	}
	return convertError(errno);
}

// T-12 (F-16 of the fork) : read into a host buffer ; bounds checked by the caller.
uint8_t FISReadFileHandleBuffer(uint8_t fileno, uint8_t* dest, uint16_t* size) {
	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	errno = 0;
	printf("FISReadFileHandleBuffer(%d, %d) -> ", fileno, *size);
	size_t result = fread(dest, 1, *size, f);
	printf("%d: %s\n", (int)result, (result != *size) ? strerror(errno) : "OK");
	*size = result;

	if ((errno == 0) && (result == 0)) {
		return FIOERROR_EOF;
	}
	return convertError(errno);
}

uint8_t FISWriteFileHandle(uint8_t fileno, uint16_t address, uint16_t* size) {
	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	printf("FISWriteFileHandle(%d, @0x%x, %d) -> ", fileno, address, *size);
	size_t result = fwrite(cpuMemory+address, *size, 1, f);
	printf("%d: %s\n", (int)result, (result != 1) ? strerror(errno) : "OK");
	//*size = result;

	if (result == 1) {
		return FIOERROR_OK;
	}
	return convertError(errno);
}

uint8_t FISGetSizeFileHandle(uint8_t fileno, uint32_t* size) {
	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	printf("FISGetSizeFileHandle(%d) -> ", fileno);
	errno = 0;
	size_t oldpos = ftell(f);
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, oldpos, SEEK_SET);
	printf("%d: %s\n", *size, strerror(errno));

	return FIOERROR_OK;
}

uint8_t FISSetSizeFileHandle(uint8_t fileno, uint32_t size) {
	FILE* f = getF(fileno);
	if (!f)
		return FIOERROR_INVALID_PARAMETER;

	printf("FISSetSizeFileHandle(%d, %d) -> ", fileno, size);
	errno = 0;
	fflush(f);
	int result = ftruncate(::fileno(f), size);
	printf("%s\n", strerror(errno));

	return !result ? FIOERROR_OK : convertError(errno);
}

// ***************************************************************************************
//
//								Dummy initialise & synchronise
//
// ***************************************************************************************

void STOInitialise(void) {
}

void STOSynchronise(void) {
	CONWriteString("Stored in 'storage' directory\r");
}

// ***************************************************************************************
//
//								Emulated serial functions
//
// ***************************************************************************************

bool SERInitialise(void) {
	return false;
}

bool SERIsByteAvailable(void) {
    return serialInterface ? serialInterface->isByteAvailable() : false;
}

uint8_t SERReadByte(void) {
    return serialInterface ? serialInterface->readByte() : 0;
}

void SERWriteByte(uint8_t b) {
    if (serialInterface) {
        serialInterface->writeByte(b);
    }
}

void SERSetSerialFormat(uint32_t baudRate, uint32_t protocol) {
    // Initialize the interface if it hasn't been set yet
    if (!serialInterface) {
        serialInterface = SerialInterfaceOpen(SER_TCP);
    }
    
    if (serialInterface) {
        serialInterface->setSerialFormat(baudRate, protocol);
    }
}


// ***************************************************************************************
//
//								Dummy GPIO functions
//
// ***************************************************************************************

int UEXTSetGPIODirection(int gpio,int pinType) {
	//printf("Pin %d set direction to %d\n",gpio,pinType);
	return 0;
}

int UEXTSetGPIO(int gpio,bool isOn) {
	printf("Set Pin %d to %d\n",gpio,isOn ? 1 : 0);
	return 0;
}

int UEXTGetGPIO(int gpio,bool *pIsHigh) {
	printf("Read pin %d, value is (not) %d\n",gpio,gpio & 1);
	*pIsHigh = (gpio & 1) != 0;
	return 0;
}

int UEXTGetGPIOAnalogue(int gpio,uint16_t *pLevel) {
	*pLevel = gpio+1000;
	printf("Read Analogue pin %d, value is (not) %d\n",gpio,*pLevel);
	return 0;
}

// ***************************************************************************************
//
//                                     Dummy I2C functions
//
// ***************************************************************************************

int UEXTI2CInitialise(void) {
	printf("I2C Initialise\n");
    return 0;
}

// ***************************************************************************************
//
//                          	Write bytes to I2C device
//
// ***************************************************************************************

// F-14 : PCF8563 real time clock modelled at $51 : registers $00-$0F, the time registers
// ($02-$08, BCD) are taken from the host clock until a program writes them (then they
// follow the host clock offset by the difference, seconds resolution).
static uint8_t rtcReg = 0;  														// Register pointer.
static long rtcOffset = 0;  														// Seconds added to the host clock.
static uint8_t rtcControl[2] = { 0,0 };
static uint8_t rtcBCD(int v) { return ((v / 10) << 4) | (v % 10); }
static int rtcFromBCD(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static void rtcTime(uint8_t *r) {  												// r[0..6] = $02..$08
	time_t now = time(NULL) + rtcOffset;
	struct tm *t = localtime(&now);
	r[0] = rtcBCD(t->tm_sec);r[1] = rtcBCD(t->tm_min);r[2] = rtcBCD(t->tm_hour);
	r[3] = rtcBCD(t->tm_mday);r[4] = t->tm_wday;
	r[5] = rtcBCD(t->tm_mon + 1) | ((t->tm_year < 100) ? 0x80 : 0);r[6] = rtcBCD(t->tm_year % 100);
}

static int rtcWrite(uint8_t *data,size_t size) {
	if (size == 0) return 1;
	rtcReg = data[0] & 0x0F;
	if (size >= 8 && rtcReg == 0x02) {  											// Full time write : keep the offset to the host clock.
		struct tm t;memset(&t,0,sizeof(t));
		t.tm_sec = rtcFromBCD(data[1] & 0x7F);t.tm_min = rtcFromBCD(data[2] & 0x7F);t.tm_hour = rtcFromBCD(data[3] & 0x3F);
		t.tm_mday = rtcFromBCD(data[4] & 0x3F);t.tm_mon = rtcFromBCD(data[6] & 0x1F) - 1;
		t.tm_year = rtcFromBCD(data[7]) + ((data[6] & 0x80) ? 0 : 100);t.tm_isdst = -1;
		rtcOffset = (long)(mktime(&t) - time(NULL));
	} else if (size >= 2 && rtcReg < 2) {
		rtcControl[rtcReg] = data[1];
	}
	return 0;
}

static int rtcRead(uint8_t *data,size_t size) {
	uint8_t r[16];memset(r,0,sizeof(r));
	r[0] = rtcControl[0];r[1] = rtcControl[1];
	rtcTime(r + 2);
	for (size_t i = 0;i < size;i++) data[i] = r[(rtcReg + i) & 0x0F];
	return 0;
}

int UEXTI2CWriteBlock(uint8_t device,uint8_t *data,size_t size) {
	if (device == 0x51) return rtcWrite(data,size);  								// F-14 : PCF8563
	printf("I2C Write to $%02x %d bytes\n",device,(int)size);
	for (int i = 0;i < size;i++) {
		printf(" $%02x",data[i]);
	}
	printf("\n");
	return 0;
}

// ***************************************************************************************
//
//                          	Read bytes from I2C device 
//
// ***************************************************************************************

int UEXTI2CReadBlock(uint8_t device,uint8_t *data,size_t size) {
	if (device == 0x7F) return 1;
	if (device == 0x51) return rtcRead(data,size);  								// F-14 : PCF8563
	printf("I2C Read from $%x %d bytes\n",device,(int)size);
	for (int i = 0;i < size;i++) {
		data[i] = device + 0x12 + i * 3;
		printf(" $%02x",data[i]);
	}
	printf("\n");
    return 0;
}

// ***************************************************************************************
//
//                                     Dummy SPI functions
//
// ***************************************************************************************

int UEXTSPIInitialise(void) {
	printf("SPI Initialise\n");
    return 0;
}

// ***************************************************************************************
//
//                          Write bytes to SPI device
//
// ***************************************************************************************

int UEXTSPIWriteBlock(uint8_t *data,size_t size) {
	printf("SPI Write to %d bytes\n",(int)size);
	for (int i = 0;i < size;i++) {
		printf(" $%02x",data[i]);
	}
	printf("\n");
	return 0;
}

// ***************************************************************************************
//
//                          Read bytes from I2C device
//
// ***************************************************************************************

int UEXTSPIReadBlock(uint8_t *data,size_t size) {
	printf("SPI Read from %d bytes\n",(int)size);
	for (int i = 0;i < size;i++) {
		data[i] = 0x12 + i * 3;
		printf(" $%02x",data[i]);
	}
	printf("\n");
    return 0;
}
// ***************************************************************************************
//
//                          			Hardware reset
//
// ***************************************************************************************

void ResetSystem(void) {
	printf("Hardware reset.\n");
}

// ***************************************************************************************
//
//                          			Dummy Gamepad
//
// ***************************************************************************************

uint8_t GMPGetControllerCount(void) {
	return GFXControllerCount();
}

// ***************************************************************************************
//
//                          		Read controller status
//
// ***************************************************************************************

uint32_t GMPReadDigitalController(uint8_t index) {
	return GFXReadController(index);
}

// ***************************************************************************************
//
//                          		 Handle dispatch warning.
//
// ***************************************************************************************

void DSPWarnHandler(uint8_t group,uint8_t func) {
	fprintf(stderr,"** WARN ** Execute %d.%d not defined.\n",group,func);
}

// ***************************************************************************************
//
//                          Update mouse state - move or buttons
//
// ***************************************************************************************

void HWUpdateMouse(void) {
	int x,y;
	SDL_Rect r;
	int xScale,yScale,xWidth,yWidth;

	Uint32 sbut = SDL_GetMouseState(&x,&y);
	DGBXGetActiveDisplayInfo(&r,&xScale,&yScale,&xWidth,&yWidth);

	if (x >= r.x && y >= r.y && x < r.x+r.w && y < r.y+r.h) {
		x = (x - r.x) / xScale;y = (y - r.y) / yScale;
		int buttons = 0;
		if (sbut & SDL_BUTTON(1)) buttons |= 0x1;
		if (sbut & SDL_BUTTON(3)) buttons |= 0x2;
		if (sbut & SDL_BUTTON(2)) buttons |= 0x4;
		// printf("%x %x %x\n",buttons,x,y);
		MSESetPosition(x & 0xFFFF,y & 0xFFFF);
		MSEUpdateButtonState(buttons & 0xFF);
	}
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
