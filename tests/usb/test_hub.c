// T-168 : nouvelle détection d'un port (tuh_trinity_hub_replug). hub.c du TinyUSB du firmware (PICO_TINYUSB_PATH),
// compilé sur PC ; la pile USB est remplacée par des bouchons, le hub répond comme le décrit chaque scénario.
#include <stdio.h>
#include <string.h>
#include "host/hub.c"

#define HUB (CFG_TUH_DEVICE_MAX + 1)

static tuh_xfer_t pending;static tusb_control_request_t request;static int waiting;
static char trace[512];static int failures,polls;
#define NOTE(...) snprintf(trace + strlen(trace),sizeof(trace) - strlen(trace),__VA_ARGS__)

bool tuh_control_xfer(tuh_xfer_t *x) {
	pending = *x;request = *x->setup;pending.setup = &request;waiting++;
	NOTE("req(%d,%d,%d) ",request.bRequest,request.wValue,request.wIndex);
	return true;
}
bool usbh_edpt_claim(uint8_t d,uint8_t e) { (void)d;(void)e;return true; }
bool usbh_edpt_release(uint8_t d,uint8_t e) { (void)d;(void)e;return true; }
bool usbh_edpt_xfer_with_callback(uint8_t d,uint8_t e,uint8_t *b,uint16_t n,tuh_xfer_cb_t cb,uintptr_t u) {
	(void)d;(void)e;(void)b;(void)n;(void)cb;(void)u;polls++;return true;
}
uint8_t usbh_get_rhport(uint8_t d) { (void)d;return 0; }
void hcd_event_handler(hcd_event_t const *e,bool isr) {
	(void)isr;NOTE("%s%d.%d ",e->event_id == HCD_EVENT_DEVICE_ATTACH ? "branché" : "retiré",e->connection.hub_addr,e->connection.hub_port);
}

static void answer(xfer_result_t result) {		// Le hub répond à la requête en attente
	tuh_xfer_t x = pending;waiting--;x.result = result;
	if (x.complete_cb) x.complete_cb(&x);
}

//		Le hub signale un changement sur un port : lecture de son état, puis acquittements.
static void change(int port,int connected) {
	get_hub_epbuf(HUB)->status_change[0] = 1 << port;
	hub_xfer_cb(HUB,0x81,XFER_RESULT_SUCCESS,1);
	hub_port_status_response_t s;memset(&s,0,sizeof(s));s.status.connection = connected;s.change.reset = 1;
	memcpy(pending.buffer,&s,sizeof(s));
	answer(XFER_RESULT_SUCCESS);													// GET_STATUS
	while (waiting) answer(XFER_RESULT_SUCCESS);									// CLEAR_FEATURE
}

static void reset(void) {
	trace[0] = 0;waiting = polls = 0;
	hub_interface_t *p = get_hub_itf(HUB);memset(p,0,sizeof(*p));p->bNbrPorts = 4;p->ep_in = 0x81;
}

static void check(const char *name,const char *expected,int expectedPolls) {
	int ok = strcmp(trace,expected) == 0 && polls == expectedPolls;
	printf("%-36s : %s\n",name,ok ? "OK" : "ECHEC");
	if (!ok) { printf("   obtenu  : %s(%d relances)\n   attendu : %s(%d)\n",trace,polls,expected,expectedPolls);failures++; }
}

#define RESET2 "req(3,4,2) "					/* SET_FEATURE PORT_RESET, port 2 */
#define LU2 "req(0,0,2) req(1,20,2) "			/* GET_STATUS, CLEAR_FEATURE C_PORT_RESET */
#define LU3 "req(0,0,3) req(1,20,3) "

int main(void) {
	reset();change(2,1);
	check("reset ordinaire : pas de détection",LU2,1);
	reset();tuh_trinity_hub_replug(HUB,2);answer(XFER_RESULT_SUCCESS);change(2,1);
	check("port à redétecter",RESET2 LU2 "branché6.2 ",0);							// Pas de relance : l'énumération la fera
	reset();change(2,1);
	check("une seule fois",LU2,1);
	reset();tuh_trinity_hub_replug(HUB,2);answer(XFER_RESULT_SUCCESS);change(3,1);change(2,1);
	check("un autre port entre-temps",RESET2 LU3 LU2 "branché6.2 ",1);
	reset();tuh_trinity_hub_replug(HUB,2);answer(XFER_RESULT_FAILED);change(2,1);
	check("reset refusé",RESET2 LU2,1);
	reset();tuh_trinity_hub_replug(HUB,2);answer(XFER_RESULT_SUCCESS);change(2,0);
	check("clé retirée pendant le reset",RESET2 LU2 "retiré6.2 ",1);
	reset();
	bool r = tuh_trinity_hub_replug(1,2) || tuh_trinity_hub_replug(HUB,0) || tuh_trinity_hub_replug(0,0);
	if (r) NOTE("accepté ");
	check("hub ou port invalide","",0);
	printf("test_hub : %s (%d échec(s))\n",failures ? "ECHEC" : "OK",failures);
	return failures != 0;
}
