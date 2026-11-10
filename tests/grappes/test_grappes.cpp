// test_grappes.cpp — T-144 (proposition Neo6502AigleDorEngine) : f_read lit les grappes contiguës en un seul
// disk_read (au plus FF_TRINITY_RUN secteurs). make test-grappes
//
// Le FatFs du firmware (firmware/lib/fatfs, son ffconf.h : FF_FS_TINY) sur une image faite par mkfs.fat et tenue
// en mémoire. Un fichier contigu (A, 80 Ko) et un fichier fragmenté (B, écrit une grappe sur deux en alternance
// avec C). Pour chaque lecture : octets comparés au motif attendu, et nombre / taille des disk_read relevés.
// Vérifie : données exactes partout (positions alignées ou non, tailles diverses, fin de fichier) ; sur A, une
// lecture alignée de 64 000 o se fait en un disk_read de 125 secteurs, 80 Ko en 128 + 32 ; sur B, aucun disk_read
// ne dépasse une grappe ; un secteur modifié encore dans la fenêtre (non écrit) est bien relu.
// Contre-essai (make test-grappes-ancien) : ff.c d'avant T-144, qui doit ÉCHOUER aux tailles de disk_read.
// Usage : test_grappes IMAGE (mkfs.fat -C IMAGE ...)
// Auteur : bmarty <bmarty@mailo.com>

#include <cstdio>
#include <cstring>
#include <vector>
extern "C" {
#include "ff.h"
#include "diskio.h"
}

static int echecs = 0;
static void verifie(bool ok, const char *quoi) {
	printf("%s : %s\n", ok ? "OK   " : "ÉCHEC", quoi);
	if (!ok) echecs++;
}

// --- disque en mémoire, avec relevé des lectures -------------------------------------------------------------

static std::vector<unsigned char> image;
static unsigned lectures = 0,maxCompte = 0;

extern "C" {
DSTATUS disk_initialize(BYTE) { return 0; }
DSTATUS disk_status(BYTE) { return 0; }
DRESULT disk_read(BYTE, BYTE *buff, LBA_t sector, UINT count) {
	if ((sector + count) * 512 > image.size()) return RES_PARERR;
	memcpy(buff, &image[sector * 512], count * 512);
	lectures++;if (count > maxCompte) maxCompte = count;
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

static unsigned char motif(int f,unsigned p) { return (unsigned char)((p * 7 + (p >> 8) * 13 + f * 101) & 0xFF); }

static bool ecrit(FIL *fp,int f,unsigned debut,unsigned n) {
	std::vector<unsigned char> b(n);
	for (unsigned i = 0;i < n;i++) b[i] = motif(f,debut + i);
	UINT bw;
	return f_write(fp,b.data(),n,&bw) == FR_OK && bw == n;
}

// Lit n octets à pos ; vrai si les octets sont ceux du motif (avec, éventuellement, une zone modifiée).
static bool lit(const char *nom,int f,unsigned pos,unsigned n,unsigned taille) {
	FIL fil;
	if (f_open(&fil,nom,FA_READ) != FR_OK) return false;
	std::vector<unsigned char> b(n + 16,0xEE);
	UINT br;
	lectures = 0;maxCompte = 0;
	bool ok = f_lseek(&fil,pos) == FR_OK && f_read(&fil,b.data(),n,&br) == FR_OK;
	unsigned attendu = (pos + n > taille) ? taille - pos : n;
	ok = ok && br == attendu;
	for (unsigned i = 0;ok && i < attendu;i++) if (b[i] != motif(f,pos + i)) { printf("   écart à %u\n",pos + i);ok = false; }
	f_close(&fil);
	return ok;
}

int main(int argc, char **argv) {
	if (argc != 2) { fprintf(stderr, "usage : test_grappes IMAGE\n"); return 2; }
	FILE *fi = fopen(argv[1], "rb");
	if (!fi) { perror(argv[1]); return 2; }
	fseek(fi, 0, SEEK_END); image.resize(ftell(fi)); fseek(fi, 0, SEEK_SET);
	if (fread(image.data(), 1, image.size(), fi) != image.size()) return 2;
	fclose(fi);

	FATFS fs;
	verifie(f_mount(&fs, "0:", 1) == FR_OK, "montage de l'image");
	unsigned g = fs.csize * 512;
	printf("        FAT%d, grappes de %u o\n", fs.fs_type == FS_FAT32 ? 32 : fs.fs_type == FS_FAT16 ? 16 : 12, g);

	const unsigned TA = 81920,TB = 20 * g;
	FIL a,b,c;
	verifie(f_open(&a,"A.BIN",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK && ecrit(&a,1,0,TA) && f_close(&a) == FR_OK,
			"A : 80 Ko écrits d'un coup (contigu)");
	bool ok = f_open(&b,"B.BIN",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK && f_open(&c,"C.BIN",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK;
	for (unsigned i = 0;ok && i < 20;i++) ok = ecrit(&b,2,i * g,g) && f_sync(&b) == FR_OK && ecrit(&c,3,i * g,g) && f_sync(&c) == FR_OK;
	verifie(ok && f_close(&b) == FR_OK && f_close(&c) == FR_OK,"B et C : 20 grappes écrites en alternance (B fragmenté)");

	// Données exactes, contigu et fragmenté, positions et tailles variées
	const unsigned pos[] = { 0, 512, 100, g, g - 300, 3 * g + 512, 70000 };
	const unsigned tai[] = { 64000, 512, 513, g, 2 * g + 1, 8192, 20000 };
	ok = true;
	for (unsigned p : pos) for (unsigned t : tai) if (p < TA && !lit("A.BIN",1,p,t,TA)) { printf("   A pos %u taille %u\n",p,t);ok = false; }
	verifie(ok,"A : 49 lectures (positions et tailles variées) : octets exacts");
	ok = true;
	for (unsigned p : pos) for (unsigned t : tai) if (p < TB && !lit("B.BIN",2,p,t,TB)) { printf("   B pos %u taille %u\n",p,t);ok = false; }
	verifie(ok,"B (fragmenté) : mêmes lectures : octets exacts");
	verifie(lit("A.BIN",1,TA - 1000,5000,TA),"A : lecture qui dépasse la fin du fichier, tronquée, octets exacts");

	// Tailles des disk_read
	lit("A.BIN",1,0,64000,TA);
	printf("        A 64 000 o alignés : %u disk_read, le plus gros %u secteurs\n",lectures,maxCompte);
	verifie(maxCompte == 125,"A : 64 000 o alignés lus en un disk_read de 125 secteurs");
	lit("A.BIN",1,0,TA,TA);
	printf("        A 80 Ko alignés : %u disk_read, le plus gros %u secteurs\n",lectures,maxCompte);
	verifie(maxCompte == FF_TRINITY_RUN,"A : 80 Ko alignés, disk_read limité à FF_TRINITY_RUN (128) secteurs");
	lit("A.BIN",1,100,64000,TA);
	printf("        A 64 000 o à 100 : %u disk_read, le plus gros %u secteurs\n",lectures,maxCompte);
	verifie(maxCompte == 124,"A : 64 000 o à la position 100 : 124 secteurs entiers en un disk_read");
	lit("B.BIN",2,0,TB,TB);
	printf("        B %u o : %u disk_read, le plus gros %u secteurs\n",TB,lectures,maxCompte);
	verifie(maxCompte == fs.csize,"B (fragmenté) : aucun disk_read au-delà d'une grappe");

	// Secteur modifié encore dans la fenêtre (FF_FS_TINY) puis lecture qui le couvre
	FIL m;
	ok = f_open(&m,"A.BIN",FA_READ | FA_WRITE) == FR_OK && f_lseek(&m,5000) == FR_OK;
	UINT bw = 0,br = 0;
	ok = ok && f_write(&m,"TRINITY144",10,&bw) == FR_OK && bw == 10 && f_lseek(&m,0) == FR_OK;
	std::vector<unsigned char> buf(64000);
	ok = ok && f_read(&m,buf.data(),64000,&br) == FR_OK && br == 64000 && memcmp(&buf[5000],"TRINITY144",10) == 0
			&& buf[4999] == motif(1,4999) && buf[5010] == motif(1,5010) && buf[63999] == motif(1,63999);
	verifie(ok && f_close(&m) == FR_OK,"A : 10 o écrits à 5000 sans sync, relus dans une lecture de 64 000 o");

	f_unmount("0:");
	printf("test-grappes : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
	return echecs ? 1 : 0;
}
