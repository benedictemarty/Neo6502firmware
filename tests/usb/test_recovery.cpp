// T-168 : nouvelle détection d'une clé qui ne répond plus (msc_recovery.cpp) sur PC, pile USB remplacée par des
// bouchons : volume gardé, clé revenue, clé muette, autre clé à sa place, hub qui ne répond pas, clé hors hub.
#include <stdio.h>
#include <string>
#include <vector>
#include "tusb.h"
#include "host/hub.h"
#include "msc_recovery.h"

static std::string trace;
static uint32_t now;
static int failures;
static uint8_t hubOfKey;
static bool replugRefused,configured;
static uint16_t vid[8],pid[8];
static uint32_t blocks[8];

struct Event { uint32_t at;char kind;uint8_t addr; };		// 'r' retrait, 'm' montage, 'c' autre clé
static std::vector<Event> events;

uint32_t time_us_32(void) { return now; }
extern "C" bool tuh_bus_info_get(uint8_t,tuh_bus_info_t *b) { b->rhport = 0;b->hub_addr = hubOfKey;b->hub_port = 2;b->speed = 0;return true; }
extern "C" bool tuh_vid_pid_get(uint8_t a,uint16_t *v,uint16_t *p) { *v = vid[a];*p = pid[a];return true; }
extern "C" uint32_t tuh_msc_get_block_count(uint8_t a,uint8_t) { return blocks[a]; }
extern "C" bool tuh_msc_abort(uint8_t,uint8_t *i,uint8_t *in,uint8_t *out) { trace += "abandon ";*i = 0;*in = 0x81;*out = 0x02;return configured; }
extern "C" bool tuh_trinity_hub_replug(uint8_t h,uint8_t p) { trace += "port" + std::to_string(h) + "." + std::to_string(p) + " ";return !replugRefused; }
void MSCRecoveryLost(uint8_t a) { trace += "démonté" + std::to_string(a) + " "; }
void MSCRecoveryWait(void) {
	now += 100;
	while (!events.empty() && now >= events.front().at) {
		Event e = events.front();events.erase(events.begin());
		if (e.kind == 'c') blocks[e.addr] = 55;  								// Another key takes this address
		else if (e.kind == 'r') trace += MSCRecoveryHoldsMount(e.addr) ? "gardé " : "retrait-normal ";
		else trace += MSCRecoveryReturned(e.addr) ? "revenu " : "montage-normal ";
	}
}

static void reset(void) {
	trace.clear();events.clear();hubOfKey = 6;replugRefused = false;configured = true;
	for (int i = 0;i < 8;i++) { vid[i] = 0x090c;pid[i] = 0x1000;blocks[i] = 1000000; }
}

static void check(const char *name,bool expected,const std::string &expectedTrace) {
	uint32_t t0 = now;
	for (auto &e : events) e.at += t0;
	bool r = MSCRecoverKey(1);
	bool ok = (r == expected && trace == expectedTrace && now - t0 < 3100000);
	//	Hors reprise, retraits et montages suivent leur cours normal
	ok = ok && !MSCRecoveryHoldsMount(1) && !MSCRecoveryReturned(1);
	printf("%-36s : %s\n",name,ok ? "OK" : "ECHEC");
	if (!ok) { printf("   rendu %d, %u us\n   obtenu  : %s\n   attendu : %s\n",r,(unsigned)(now - t0),trace.c_str(),expectedTrace.c_str());failures++; }
}

#define DEBUT "abandon port6.2 "

int main(void) {
	reset();events = { {20000,'r',1},{600000,'m',1} };
	check("clé revenue, volume gardé",true,DEBUT "gardé revenu ");
	reset();events = { {20000,'r',1} };
	check("clé retirée et pas revenue",false,DEBUT "gardé démonté1 ");
	reset();
	check("hub sans réponse : rien démonté",false,DEBUT);
	reset();events = { {20000,'r',1},{300000,'c',1},{600000,'m',1} };
	check("autre clé à la même adresse",false,DEBUT "gardé démonté1 montage-normal ");
	reset();events = { {20000,'r',1},{600000,'m',2} };blocks[2] = 55;
	check("autre clé à une autre adresse",false,DEBUT "gardé montage-normal démonté1 ");
	reset();events = { {20000,'r',1},{600000,'m',3} };
	check("même clé à une autre adresse",false,DEBUT "gardé démonté1 montage-normal ");
	reset();events = { {10000,'m',2},{20000,'r',1},{600000,'m',1} };
	check("autre clé montée avant le retrait",true,DEBUT "montage-normal gardé revenu ");
	reset();events = { {20000,'r',2},{40000,'r',1},{600000,'m',1} };
	check("retrait d'une autre clé",true,DEBUT "retrait-normal gardé revenu ");
	reset();replugRefused = true;
	check("hub qui refuse",false,DEBUT);
	reset();hubOfKey = 0;
	check("clé hors d'un hub",false,"");
	reset();configured = false;
	check("clé non configurée",false,"abandon ");
	printf("test_recovery : %s (%d échec(s))\n",failures ? "ECHEC" : "OK",failures);
	return failures != 0;
}
