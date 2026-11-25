// T-167 : le résultat d'un transfert raté doit arriver jusqu'à l'application. msc_host.c du TinyUSB du firmware
// (PICO_TINYUSB_PATH), compilé sur PC ; la pile USB est remplacée par des bouchons.
#include <stdio.h>
#include <string.h>
#include "class/msc/msc_host.c"

static int queued,calls,status,failures;
bool usbh_edpt_claim(uint8_t d,uint8_t e) { (void)d;(void)e;return true; }
bool usbh_edpt_release(uint8_t d,uint8_t e) { (void)d;(void)e;return true; }
bool usbh_edpt_xfer_with_callback(uint8_t d,uint8_t e,uint8_t *b,uint16_t n,tuh_xfer_cb_t cb,uintptr_t u) {
	(void)d;(void)e;(void)b;(void)n;(void)cb;(void)u;queued++;return true;
}

static int aborted[4];
bool tuh_edpt_abort_xfer(uint8_t d,uint8_t e) { (void)d;aborted[(e & 1) + ((e & 0x80) >> 6)]++;return true; }

static bool done(uint8_t d,tuh_msc_complete_data_t const *cb) { (void)d;calls++;status = cb->csw->status;return true; }

//		Une lecture d'un secteur : résultats des étapes commande, données, statut ; le CSW laissé en mémoire dit
//		toujours « réussi » (clé satisfaite, ou reste de la commande précédente). attendu < 0 : pas de rappel.
static void play(const char *name,xfer_result_t cmd,xfer_result_t data,xfer_result_t sts,int expected) {
	static uint8_t sector[512];
	msch_interface_t *p = get_itf(1);
	memset(p,0,sizeof(*p));p->configured = p->mounted = true;p->ep_in = 0x81;p->ep_out = 0x01;p->capacity[0].block_size = 512;		// Sinon pas d'étape de données
	memset(get_epbuf(1),0,sizeof(msch_epbuf_t));
	queued = calls = 0;status = -1;
	tuh_msc_read10(1,0,sector,0,1,done,0);
	msch_xfer_cb(1,0x01,cmd,cmd == XFER_RESULT_SUCCESS ? sizeof(msc_cbw_t) : 0);
	if (calls == 0 && queued > 1) msch_xfer_cb(1,0x81,data,data == XFER_RESULT_SUCCESS ? 512 : 0);
	if (calls == 0 && queued > 2) msch_xfer_cb(1,0x81,sts,sts == XFER_RESULT_SUCCESS ? sizeof(msc_csw_t) : 0);
	int ok = (calls == 1 && status == expected && p->stage == MSC_STAGE_IDLE && queued == (cmd == XFER_RESULT_FAILED ? 1 : 3));
	printf("%-28s rappels %d, statut %2d, attendu %d : %s\n",name,calls,status,expected,ok ? "OK" : "ECHEC");
	if (!ok) failures++;
}

int main(void) {
	const xfer_result_t S = XFER_RESULT_SUCCESS,F = XFER_RESULT_FAILED,T = XFER_RESULT_STALLED;
	play("tout réussi",S,S,S,0);
	play("commande ratée",F,S,S,2);		// 2 = phase error : la clé doit être réinitialisée (T-168)
	play("données ratées",S,F,S,2);
	play("statut raté",S,S,F,2);
	play("réussi après un échec",S,S,S,0);
	play("blocage (STALL) en données",S,T,S,0);			// Le CSW de la clé fait foi : comportement d'origine
	//		T-168 : abandon d'une commande en cours ; une fin de transfert arrivée après ne rappelle personne.
	msch_interface_t *p = get_itf(1);
	uint8_t itf = 9,in = 0,out = 0;
	p->itf_num = 3;calls = 0;
	tuh_msc_read10(1,0,0,0,1,done,0);
	bool ok = tuh_msc_abort(1,&itf,&in,&out) && itf == 3 && in == 0x81 && out == 0x01 && p->stage == MSC_STAGE_IDLE
			&& aborted[3] == 1 && aborted[1] == 1;
	msch_xfer_cb(1,0x01,S,sizeof(msc_cbw_t));
	ok = ok && calls == 0 && p->stage == MSC_STAGE_IDLE;
	printf("%-28s : %s\n","abandon d'une commande",ok ? "OK" : "ECHEC");
	if (!ok) failures++;
	printf("test_msc : %s (%d échec(s))\n",failures ? "ECHEC" : "OK",failures);
	return failures != 0;
}
