// T-168 : réinitialisation d'une clé USB (msc_recovery.cpp) sur PC, pile USB remplacée par des bouchons : ordre des
// requêtes, remise à zéro des bits de bascule, clé muette, requête refusée, clé retirée, réponse tardive.
#include <stdio.h>
#include <string>
#include "tusb.h"
#include "host/hcd.h"
#include "msc_recovery.h"

static std::string trace;
static uint32_t now;
static int failAt,silentAt,unplugAt,count;
static bool lateAnswer;
static tuh_xfer_t pending;
static bool hasPending;
static tuh_xfer_cb_t oldCb;static tuh_xfer_t oldXfer;

uint32_t time_us_32(void) { return now; }
extern "C" bool tuh_msc_mounted(uint8_t) { return unplugAt == 0 || count < unplugAt; }
extern "C" bool tuh_msc_abort(uint8_t,uint8_t *i,uint8_t *in,uint8_t *out) { trace += "abandon ";*i = 2;*in = 0x81;*out = 0x02;return true; }
extern "C" bool tuh_edpt_abort_xfer(uint8_t,uint8_t ep) { trace += "abandon-ep" + std::to_string(ep) + " ";hasPending = false;return true; }
extern "C" bool hcd_edpt_clear_stall(uint8_t,uint8_t,uint8_t ep) { trace += "bascule" + std::to_string(ep) + " ";return true; }
extern "C" bool tuh_control_xfer(tuh_xfer_t *x) {
	char b[64];
	snprintf(b,sizeof(b),"req(%02x,%d,%d) ",*(uint8_t *)x->setup,x->setup->bRequest,x->setup->wIndex);
	trace += b;count++;
	pending = *x;pending.setup = 0;hasPending = true;
	return true;
}
void MSCRecoveryWait(void) {
	now += 100;
	if (!hasPending) return;
	if (lateAnswer && count == 2 && oldCb) { oldXfer.result = XFER_RESULT_SUCCESS;oldCb(&oldXfer);oldCb = 0;return; }
	if (count == silentAt) return;
	if (count == 1) { oldCb = pending.complete_cb;oldXfer = pending; }
	hasPending = false;
	pending.result = (count == failAt) ? XFER_RESULT_STALLED : XFER_RESULT_SUCCESS;
	pending.complete_cb(&pending);
}

static int failures;
static void play(const char *name,int fail,int silent,int unplug,bool late,bool expected,const char *expectedTrace) {
	trace.clear();count = 0;failAt = fail;silentAt = silent;unplugAt = unplug;lateAnswer = late;hasPending = false;oldCb = 0;
	uint32_t t0 = now;
	bool r = MSCResetRecovery(1);
	bool ok = (r == expected && trace == expectedTrace && now - t0 < 3100000);
	printf("%-34s : %s\n",name,ok ? "OK" : "ECHEC");
	if (!ok) { printf("   rendu %d, %u us\n   obtenu  : %s\n   attendu : %s\n",r,(unsigned)(now - t0),trace.c_str(),expectedTrace);failures++; }
}

#define R1 "req(21,255,2) "  		// Bulk-Only Mass Storage Reset, interface 2
#define R2 "req(02,1,129) "  		// CLEAR_FEATURE HALT, point $81
#define R3 "req(02,1,2) "  			// CLEAR_FEATURE HALT, point $02

int main(void) {
	play("clé qui répond",0,0,0,false,true,"abandon " R1 R2 "bascule129 " R3 "bascule2 ");
	play("reset refusé",1,0,0,false,false,"abandon " R1);
	play("seconde requête refusée",2,0,0,false,false,"abandon " R1 R2);
	play("clé muette",0,1,0,false,false,"abandon " R1 "abandon-ep0 ");
	play("clé retirée",0,2,2,false,false,"abandon " R1 R2);
	play("réponse tardive ignorée",0,2,0,true,false,"abandon " R1 R2 "abandon-ep0 ");
	printf("test_recovery : %s (%d échec(s))\n",failures ? "ECHEC" : "OK",failures);
	return failures != 0;
}
