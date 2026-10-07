// ***************************************************************************************
//
//      Test sur PC du synthétiseur (sndcreator.cpp), T-80 : chaque canal garde son niveau
//      entre deux bascules (un carré, pas un train d'impulsions).
//
// ***************************************************************************************

#include <cstdio>
#include <cstdlib>
#include "common.h"

static int fails = 0;
#define CHECK(c,msg) do { if (!(c)) { printf("ECHEC : %s\n",msg); fails++; } } while (0)

int SNDGetSampleFrequency(void) { return 252000000/32/255; }                   // 30 882 Hz, comme sound.cpp à 252 MHz
uint8_t cpuMemory[65536];                                                       // T-79 : RAM du 6502 (tampon du flux)
uint8_t gfxObjectMemory[0x8000];                                                // T-119 : RAM graphique (8,17)

static void note(int ch,int freq,int vol,int type) {
    SOUND_CHANNEL c = {};
    c.isPlayingNote = true; c.currentFrequency = freq; c.currentVolume = vol; c.currentType = type;
    SNDUpdateSoundChannel(ch,&c);
}

int main(void) {
    int fe = SNDGetSampleFrequency();

    // Carré seul : jamais de zéro, amplitude ±volume, puissance d'un carré plein
    SNDMuteAllChannels(); note(0,440,100,SOUNDTYPE_SQUARE);
    long zeros = 0, power = 0, changes = 0; int prev = 0;
    for (int i = 0;i < fe;i++) {
        int s = SNDGetNextSample();
        if (s == 0) zeros++;
        if (s != 100 && s != -100) { CHECK(false,"carre : echantillon hors de +-100"); break; }
        if (i > 0 && s != prev) changes++;
        power += (long)s*s; prev = s;
    }
    CHECK(zeros == 0,"carre : echantillons nuls entre deux bascules");
    CHECK(power == (long)fe*100*100,"carre : puissance differente d'un carre plein");
    CHECK(changes > 800 && changes < 900,"carre : nombre de bascules par seconde (~2 x 440)");

    // Justesse (accumulateur de phase) : 2 x f bascules par seconde, a 1 pres
    {
        const int freqs[3] = { 440,1000,3000 };
        for (int k = 0;k < 3;k++) {
            SNDMuteAllChannels(); note(0,freqs[k],100,SOUNDTYPE_SQUARE);
            int prev2 = SNDGetNextSample(),toggles = 0;
            for (int i = 1;i < fe;i++) { int v = SNDGetNextSample(); if (v != prev2) toggles++; prev2 = v; }
            char msg[80];snprintf(msg,sizeof(msg),"justesse %d Hz : %d bascules (attendu %d)",freqs[k],toggles,2*freqs[k]);
            CHECK(abs(toggles - 2*freqs[k]) <= 1,msg);
        }
    }

    // Bruit : le niveau tenu entre deux bascules, jamais un zéro forcé
    SNDMuteAllChannels(); note(0,1000,100,SOUNDTYPE_NOISE);
    int held = 0, first = SNDGetNextSample();
    for (int i = 0;i < 10;i++) if (SNDGetNextSample() == first) held++;
    CHECK(held == 10,"bruit : niveau non tenu entre deux bascules");

    // Deux canaux : la CAG existante (3/4 dès le 2e canal) et l'écrêtage à ±127
    SNDMuteAllChannels(); note(0,440,100,SOUNDTYPE_SQUARE); note(1,440,100,SOUNDTYPE_SQUARE);
    SNDGetNextSample();                                                             // la CAG compte les canaux de l'échantillon précédent
    int s2 = SNDGetNextSample();
    CHECK(s2 == 127 || s2 == -127,"deux canaux en phase : 200*3/4 ecrete a +-127");
    note(1,440,40,SOUNDTYPE_SQUARE);
    SNDGetNextSample(); s2 = SNDGetNextSample();
    CHECK(abs(s2) == 105 || abs(s2) == 45,"deux canaux 100 et 40 : (100+-40)*3/4 = 105 ou 45 selon la phase");

    // Silence : canal coupé, sortie nulle
    SOUND_CHANNEL off = {}; SNDUpdateSoundChannel(0,&off); SNDUpdateSoundChannel(1,&off);
    SNDGetNextSample();
    CHECK(SNDGetNextSample() == 0,"silence : sortie non nulle");

    // ---- T-79 : flux PCM ----
    SNDMuteAllChannels();
    uint16_t u;
    CHECK(SNDStreamStart(0x1000,0,22050,127) == 1,"flux : moitie de taille 0 refusee");
    CHECK(SNDStreamStart(0x1000,100,0,127) == 1,"flux : cadence 0 refusee");
    CHECK(SNDStreamStart(0x1000,100,22050,128) == 1,"flux : volume 128 refuse");
    CHECK(SNDStreamStart(0xFE00,0x81,22050,127) == 1,"flux : tampon au-dela de $FF00 refuse");
    CHECK(SNDStreamStatus(&u) == 0,"flux : arrete tant que rien n'est lance");
    CHECK(SNDStreamFilled(0) == 1,"flux : 8,14 refuse flux arrete");

    // Cadence = sortie : un échantillon d'entrée par échantillon de sortie, moitiés de 100
    for (int i = 0;i < 100;i++) { cpuMemory[0x1000+i] = 0x80 + 64;cpuMemory[0x1064+i] = 0x80 - 64; }
    CHECK(SNDStreamStart(0x1000,100,fe,127) == 0,"flux : demarrage");
    SNDGetNextSample();                                                             // la CAG suit d'un echantillon
    int ok = 1;
    for (int i = 1;i < 100;i++) if (SNDGetNextSample() != 64*127/128) ok = 0;
    CHECK(ok,"flux : moitie 0 jouee a +63");
    CHECK(SNDStreamStatus(&u) == 0x81,"flux : moitie 0 rendue apres 100 echantillons");
    ok = 1;
    for (int i = 0;i < 100;i++) if (SNDGetNextSample() != -64*127/128) ok = 0;
    CHECK(ok,"flux : moitie 1 jouee a -63");
    CHECK(SNDStreamStatus(&u) == 0x83 && u == 1,"flux : les deux moitiés rendues, un retard compte");
    ok = 1;
    for (int i = 0;i < 50;i++) if (SNDGetNextSample() != 0) ok = 0;
    CHECK(ok,"flux : silence tant que rien n'est rempli (pas de rejeu)");
    CHECK(SNDStreamFilled(2) == 1,"flux : moitie 2 refusee");
    CHECK(SNDStreamFilled(0) == 0,"flux : moitie 0 marquee pleine");
    CHECK(SNDGetNextSample() == 64*127/128 && SNDStreamStatus(&u) == 0x82,"flux : reprise sur la moitie 0");

    // Cadence moitié de la sortie : chaque échantillon d'entrée dure deux échantillons de sortie
    SNDStreamStop();
    CHECK(SNDStreamStatus(&u) == 0,"flux : 8,12 arrete");
    CHECK(SNDStreamStart(0x1000,100,fe/2,127) == 0,"flux : demarrage a fe/2");
    for (int i = 0;i < 199;i++) SNDGetNextSample();
    CHECK(SNDStreamStatus(&u) == 0x80,"flux fe/2 : moitie 0 pas finie apres 199 echantillons");
    SNDGetNextSample();SNDGetNextSample();
    CHECK(SNDStreamStatus(&u) == 0x81,"flux fe/2 : moitie 0 finie apres 201 echantillons");

    // Interpolation (T-79) : à fe/2, l'échantillon intermédiaire est la moyenne des deux voisins
    SNDStreamStop();
    for (int i = 0;i < 200;i++) cpuMemory[0x1000+i] = (i & 1) ? 0x80 + 100 : 0x80;
    CHECK(SNDStreamStart(0x1000,100,fe/2,127) == 0,"flux : demarrage pour l'interpolation");
    SNDGetNextSample();SNDGetNextSample();                                         // echantillon 0 (0) puis milieu 0-1
    int i2 = SNDGetNextSample(),i3 = SNDGetNextSample();                           // echantillon 1 (100), milieu 1-2 (50)
    CHECK(i2 == 100*127/128 && abs(i3 - 50*127/128) <= 1,"flux : interpolation lineaire entre voisins");

    // Mélange : un carré à 100 + le flux à +63, CAG 3/4 : (100+63)*3/4 = 122 ou (-100+63)*3/4 = -27
    SNDStreamStop();
    for (int i = 0;i < 200;i++) cpuMemory[0x1000+i] = 0x80 + 64;                   // tampon constant (l'essai precedent l'a change)
    CHECK(SNDStreamStart(0x1000,100,fe,127) == 0,"flux : demarrage pour le melange");
    note(0,440,100,SOUNDTYPE_SQUARE);
    SNDGetNextSample();
    int m = SNDGetNextSample();
    CHECK(m == (100+63)*3/4 || m == (-100+63)*3/4,"flux + canal : somme puis CAG 3/4");
    SNDStreamStop();SNDMuteAllChannels();

    // ---- T-119 : flux depuis la RAM graphique (8,17) ----
    CHECK(SNDStreamStartGraphics(0x100,0,fe,127,0) == 1,"flux graphique : longueur 0 refusee");
    CHECK(SNDStreamStartGraphics(0x100,50,0,127,0) == 1,"flux graphique : cadence 0 refusee");
    CHECK(SNDStreamStartGraphics(0x100,50,fe,128,0) == 1,"flux graphique : volume 128 refuse");
    CHECK(SNDStreamStartGraphics(0x7FF0,0x11,fe,127,0) == 1,"flux graphique : au-dela de la RAM graphique refuse");
    CHECK(SNDStreamStartGraphics(0x7FF0,0x10,fe,127,0) == 0,"flux graphique : jusqu'au dernier octet accepte");
    SNDStreamStop();
    for (int i = 0;i < 50;i++) { gfxObjectMemory[0x100+i] = 0x80 + 64;cpuMemory[0x100+i] = 0x80 - 64; }
    CHECK(SNDStreamStartGraphics(0x100,50,fe,127,0) == 0,"flux graphique : demarrage une fois");
    SNDGetNextSample();
    ok = 1;
    for (int i = 1;i < 50;i++) if (SNDGetNextSample() != 64*127/128) ok = 0;
    CHECK(ok,"flux graphique : lit la RAM graphique, pas celle du 6502");
    CHECK(SNDStreamStatus(&u) == 0,"flux graphique : arrete a la fin de l'echantillon");
    CHECK(SNDGetNextSample() == 0 && SNDGetNextSample() == 0,"flux graphique : silence apres la fin");
    CHECK(SNDStreamStartGraphics(0x100,50,fe,127,1) == 0,"flux graphique : demarrage en boucle");
    CHECK(SNDStreamFilled(0) == 1,"flux graphique : 8,14 refuse");
    ok = 1;
    for (int i = 0;i < 500;i++) { int v = SNDGetNextSample(); if (i > 0 && v != 64*127/128) ok = 0; }
    CHECK(ok && SNDStreamStatus(&u) == 0x80 && u == 0,"flux graphique : la boucle rejoue sans fin ni retard");
    for (int i = 0;i < 50;i++) gfxObjectMemory[0x100+i] = (i == 0) ? 0x80 + 100 : 0x80;
    SNDStreamStop();
    CHECK(SNDStreamStartGraphics(0x100,50,fe/2,127,1) == 0,"flux graphique : boucle a fe/2");
    for (int i = 0;i < 98;i++) SNDGetNextSample();
    int l1 = SNDGetNextSample(),l2 = SNDGetNextSample();                           // dernier echantillon (0) puis milieu vers le premier (50)
    CHECK(l1 == 0 && abs(l2 - 50*127/128) <= 1,"flux graphique : la boucle interpole vers le debut");
    SNDStreamStop();
    CHECK(SNDStreamStatus(&u) == 0,"flux graphique : 8,12 arrete");
    // Deux moitiés dans la RAM graphique (8,17 parametre 7 = 2) : comme 8,11, remplies par 3,28 en page $90
    CHECK(SNDStreamStartGraphics(0x100,50,fe,127,3) == 1,"flux graphique : parametre 7 = 3 refuse");
    CHECK(SNDStreamStartGraphics(0x7F00,0x81,fe,127,2) == 1,"flux graphique : deux moitiés au-dela de la RAM graphique refusees");
    CHECK(SNDStreamStartGraphics(0x7F00,0x80,fe,127,2) == 0,"flux graphique : deux moitiés jusqu'au dernier octet acceptees");
    SNDStreamStop();
    for (int i = 0;i < 50;i++) { gfxObjectMemory[0x200+i] = 0x80 + 64;gfxObjectMemory[0x232+i] = 0x80 - 64;cpuMemory[0x200+i] = 0x80; }
    CHECK(SNDStreamStartGraphics(0x200,50,fe,127,2) == 0,"flux graphique : demarrage en deux moitiés");
    SNDGetNextSample();
    ok = 1;
    for (int i = 1;i < 50;i++) if (SNDGetNextSample() != 64*127/128) ok = 0;
    CHECK(ok && SNDStreamStatus(&u) == 0x81,"flux graphique : moitie 0 lue dans la RAM graphique puis rendue");
    ok = 1;
    for (int i = 0;i < 50;i++) if (SNDGetNextSample() != -64*127/128) ok = 0;
    CHECK(ok && SNDStreamStatus(&u) == 0x83 && u == 1,"flux graphique : moitie 1 jouee, les deux rendues, un retard");
    CHECK(SNDGetNextSample() == 0,"flux graphique : silence sans moitie pleine");
    CHECK(SNDStreamFilled(1) == 0 && SNDStreamFilled(0) == 0,"flux graphique : 8,14 accepte en deux moitiés");
    CHECK(SNDGetNextSample() == 64*127/128 && SNDStreamStatus(&u) == 0x80,"flux graphique : reprise sur la moitie 0");
    SNDStreamStop();
    for (int i = 0;i < 200;i++) cpuMemory[0x1000+i] = 0x80 + 64;
    CHECK(SNDStreamStart(0x1000,100,fe,127) == 0,"flux : 8,11 reprend la RAM du 6502 apres 8,17");
    SNDGetNextSample();
    CHECK(SNDGetNextSample() == 64*127/128 && SNDStreamStatus(&u) == 0x80,"flux : 8,11 apres 8,17 rend ses moitiés");
    SNDStreamStop();

    // ---- T-86 : volume général et touches multimédia ----
    SNDMuteAllChannels();note(0,440,100,SOUNDTYPE_SQUARE);
    SNDGetNextSample();
    int full = abs(SNDGetNextSample());
    uint8_t mv;bool mm;
    SNDGetMasterVolume(&mv,&mm);
    CHECK(mv == 16 && !mm && full == 100,"volume general : 16 et son normal au depart");
    CHECK(SNDSetMasterVolume(17,false) == 1,"volume general : 17 refuse");
    SNDSetMasterVolume(8,false);
    CHECK(abs(SNDGetNextSample()) == 50,"volume general 8 : moitie");
    SNDMasterKey(0xE2);
    CHECK(SNDGetNextSample() == 0,"touche Muet : silence");
    SNDMasterKey(0xE9);SNDGetMasterVolume(&mv,&mm);
    CHECK(mv == 10 && !mm && abs(SNDGetNextSample()) == 62,"Volume + : +2 et son retabli");
    for (int k = 0;k < 10;k++) SNDMasterKey(0xEA);
    SNDGetMasterVolume(&mv,&mm);
    CHECK(mv == 0 && SNDGetNextSample() == 0,"Volume - : bute a 0");
    for (int k = 0;k < 10;k++) SNDMasterKey(0xE9);
    SNDGetMasterVolume(&mv,&mm);
    CHECK(mv == 16,"Volume + : bute a 16");
    SNDMasterKey(0xCD);SNDGetMasterVolume(&mv,&mm);
    CHECK(mv == 16 && !mm,"autre touche multimedia : sans effet");
    SNDMuteAllChannels();

    printf(fails ? "test-snd : %d echec(s)\n" : "test-snd : OK\n",fails);
    return fails ? 1 : 0;
}
