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

    printf(fails ? "test-snd : %d echec(s)\n" : "test-snd : OK\n",fails);
    return fails ? 1 : 0;
}
