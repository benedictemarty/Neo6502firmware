// test_noms.cpp — noms de fichiers en Latin-1 (T-130, demande D-03 de NeoWin) : make test-noms
//
// 1. FNMToStorage / FNMFromStorage (filenames.cpp) : Latin-1 <-> UTF-8, '?' hors de Latin-1.
// 2. Le FatFs du firmware (firmware/lib/fatfs, son ffconf.h) sur une image FAT faite par mkfs.fat et
//    tenue en mémoire : « md Été » puis « cd été » (casse ignorée), cwd, lecture du répertoire,
//    fichier dans le répertoire, et les octets écrits sur le disque (nom long UTF-16 « Été », pour qu'un PC
//    lise le même nom ; nom 8.3 « ETE » : la table de majuscules CP850 de FatFs, TBL_CT850, retire les accents).
// Contre-essai (make test-noms-ancien) : FatFs avec l'ancien ffconf.h (CP437, FF_LFN_UNICODE 0) et sans
// conversion, comme jusqu'à la 0.16.85 : le test doit échouer à « cd été ».
// Auteur : bmarty <bmarty@mailo.com>

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include "interface/filenames.h"
extern "C" {
#include "ff.h"
#include "diskio.h"
}

#ifdef NOMS_ANCIEN
static std::string ToSto(const std::string& s) { return s; }
static std::string FromSto(const char *s) { return s; }
#else
static std::string ToSto(const std::string& s) { return FNMToStorage(s); }
static std::string FromSto(const char *s) { return FNMFromStorage(s); }
#endif

static int echecs = 0;
static void verifie(bool ok, const char *quoi) {
	printf("%s : %s\n", ok ? "OK   " : "ÉCHEC", quoi);
	if (!ok) echecs++;
}

// --- disque en mémoire --------------------------------------------------------------------------

static std::vector<unsigned char> image;

extern "C" {
DSTATUS disk_initialize(BYTE) { return 0; }
DSTATUS disk_status(BYTE) { return 0; }
DRESULT disk_read(BYTE, BYTE *buff, LBA_t sector, UINT count) {
	if ((sector + count) * 512 > image.size()) return RES_PARERR;
	memcpy(buff, &image[sector * 512], count * 512);
	return RES_OK;
}
DRESULT disk_write(BYTE, const BYTE *buff, LBA_t sector, UINT count) {
	if ((sector + count) * 512 > image.size()) return RES_PARERR;
	memcpy(&image[sector * 512], buff, count * 512);
	return RES_OK;
}
DRESULT disk_ioctl(BYTE, BYTE cmd, void *buff) {
	if (cmd == GET_SECTOR_COUNT) { *(LBA_t *)buff = image.size() / 512; return RES_OK; }
	return cmd == CTRL_SYNC ? RES_OK : RES_PARERR;
}
DWORD get_fattime(void) { return ((DWORD)(2026 - 1980) << 25) | (10 << 21) | (12 << 16); }
}

static bool contient(const std::string& octets) {
	std::vector<unsigned char> o(octets.begin(), octets.end());						// char signé : comparer en non signé
	return std::search(image.begin(), image.end(), o.begin(), o.end()) != image.end();
}

// --- 1. conversion ---------------------------------------------------------------------------------

static void testConversion() {
	verifie(FNMToStorage("\xC9t\xE9") == "\xC3\x89t\xC3\xA9", "Latin-1 -> UTF-8 : « Été » = C3 89 74 C3 A9");
	verifie(FNMFromStorage("\xC3\x89t\xC3\xA9") == "\xC9t\xE9", "UTF-8 -> Latin-1 : « Été » = C9 74 E9");
	bool aller = true;
	for (int c = 1; c < 256; c++) {
		std::string s(1, (char)c);
		if (FNMFromStorage(FNMToStorage(s).c_str()) != s) aller = false;
	}
	verifie(aller, "aller-retour des 255 octets 01-FF");
	verifie(FNMFromStorage("a\xE2\x95\x94z") == "a?z", "« ╔ » (U+2554, hors Latin-1) -> ?");
	verifie(FNMFromStorage("\xF0\x9F\x98\x80!") == "?!", "U+1F600 (4 octets) -> ?");
	verifie(FNMFromStorage("\x80" "a") == "?a", "octet de suite isolé -> ?");
	verifie(FNMFromStorage("\xC3") == "?", "séquence tronquée en fin -> ?");
	verifie(FNMFromStorage("\xC3" "a") == "?a", "séquence tronquée suivie d'ASCII -> ?a");
	verifie(FNMFromStorage("\xC0\xAF") == "?", "forme trop longue (C0 AF) -> ?");
	verifie(FNMFromStorage("\xFF" "b") == "?b", "octet FF -> ?");
}

// --- 2. FatFs du firmware --------------------------------------------------------------------------

static void testFatFs() {
	static FATFS fs;
	verifie(f_mount(&fs, "0:", 1) == FR_OK, "montage de l'image");
	verifie(f_mkdir(ToSto("\xC9t\xE9").c_str()) == FR_OK, "md Été");
	verifie(f_chdir(ToSto("\xE9t\xE9").c_str()) == FR_OK, "cd été (casse Latin-1 ignorée, D-03)");
	char cwd[64] = "";
	f_getcwd(cwd, sizeof(cwd));
	std::string c = FromSto(cwd);
	verifie(c == "0:/\xC9t\xE9" || c == "/\xC9t\xE9", "cwd = /Été");
	FIL f;
	verifie(f_open(&f, ToSto("No\xEBl.txt").c_str(), FA_WRITE | FA_CREATE_ALWAYS) == FR_OK, "création de Noël.txt dans Été");
	f_close(&f);
	f_chdir("/");
	FILINFO fno;
	verifie(f_stat(ToSto("\xC9T\xC9/NO\xCBL.TXT").c_str(), &fno) == FR_OK, "stat ÉTÉ/NOËL.TXT");
	DIR d;
	bool vu = false;
	if (f_opendir(&d, "/") == FR_OK) {
		while (f_readdir(&d, &fno) == FR_OK && fno.fname[0])
			if (FromSto(fno.fname) == "\xC9t\xE9") vu = true;
		f_closedir(&d);
	}
	verifie(vu, "lecture du répertoire : Été");
	verifie(f_rename(ToSto("\xE9t\xE9/no\xEBl.txt").c_str(), ToSto("\xC9t\xE9/\xC0 lire.txt").c_str()) == FR_OK,
			"renommage en « À lire.txt »");
	verifie(f_stat(ToSto("\xC9t\xE9/\xE0 LIRE.TXT").c_str(), &fno) == FR_OK, "stat Été/à LIRE.TXT");
	f_unmount("0:");
	verifie(contient(std::string("\xC9\0t\0\xE9\0", 6)), "disque : nom long « Été » en UTF-16 (C9 00 74 00 E9 00)");
	verifie(contient(std::string("ETE        \x10", 12)), "disque : nom 8.3 « ETE » (table CP850 de FatFs : accents retirés), répertoire");
}

int main(int argc, char **argv) {
	if (argc != 2) { fprintf(stderr, "usage : test_noms IMAGE (mkfs.fat -C IMAGE 1024)\n"); return 2; }
	FILE *fi = fopen(argv[1], "rb");
	if (!fi) { perror(argv[1]); return 2; }
	fseek(fi, 0, SEEK_END); image.resize(ftell(fi)); fseek(fi, 0, SEEK_SET);
	if (fread(image.data(), 1, image.size(), fi) != image.size()) return 2;
	fclose(fi);
	testConversion();
	testFatFs();
	FILE *fo = fopen(argv[1], "wb");										// laissée pour examen (mdir, fsck.fat)
	if (fo) { fwrite(image.data(), 1, image.size(), fo); fclose(fo); }
	printf("test-noms : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
	return echecs ? 1 : 0;
}
