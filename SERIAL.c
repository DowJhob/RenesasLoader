#include "SERIAL.h"
#include "CPU_STUFF.h"

#include "SELECTOR-HEADERS/IODEF.h"
#include "VEHICLE_SELECTOR.h"

void init_sci() {
    TARGET_SCI.SCR.BYTE &= 0x2F;	//clear TXIE, RXIE, RE
    TARGET_SCI.SCR.BIT.TE = 1;	//enable TX
    return;
}

void sci_rxidle(unsigned int ms) {
    u32 t0, tc, intv;

    if (ms > MCLK_MAXSPAN) ms = MCLK_MAXSPAN;
    intv = MCLK_GETTS(ms);	//# of ticks for delay

    t0 = get_mclk_ts();
    while (1) {
        tc = get_mclk_ts();
        if ((tc - t0) >= intv) return;

        if (TARGET_SCI.SSR.BYTE & 0x78) {
            /* RDRF | ORER | FER | PER :reset timer */
            t0 = get_mclk_ts();
            TARGET_SCI.SSR.BYTE &= 0x87;	//clear RDRF + error flags
        }
    }
}

void sci_txblock(const uint8_t *buf, uint32_t len) {
    for (; len > 0; len--) {
        while (!TARGET_SCI.SSR.BIT.TDRE) {}	//wait for empty
        TARGET_SCI.TDR = *buf;
        buf++;
        TARGET_SCI.SSR.BIT.TDRE = 0;		//start tx
    }
}

void iso_sendpkt(const uint8_t *buf, int len) {
    u8 hdr[2];
    uint8_t cks;
    if (len <= 0) return;

    if (len > 0xff) len = 0xff;

    TARGET_SCI.SCR.BIT.RE = 0;

    if (len <= 0x3F) {
        hdr[0] = (uint8_t) len;
        sci_txblock(hdr, 1);	//FMT/Len
    } else {
        hdr[0] = 0;
        hdr[1] = (uint8_t) len;
        sci_txblock(hdr, 2);	//Len
    }

    sci_txblock(buf, len);	//Payload

    cks = len;
    cks += cks_u8(buf, len);
    sci_txblock(&cks, 1);	//cks

    //ugly : wait for transmission end; this means re-enabling RX won't pick up a partial byte
    while (!TARGET_SCI.SSR.BIT.TEND) {}

    TARGET_SCI.SCR.BIT.RE = 1;
    return;
}

void tx_7F(u8 sid, u8 nrc) {
    u8 buf[3];
    buf[0]=0x7F;
    buf[1]=sid;
    buf[2]=nrc;
    iso_sendpkt(buf, 3);
}

void iso_clearmsg(iso14230_msg *msg) {
    msg->hdrlen = 0;
    msg->datalen = 0;
    msg->hi = 0;
    msg->di = 0;
}

uint8_t cks_u8(const uint8_t *data, unsigned int len) {
    uint8_t rv=0;

    while (len > 0) {
        len--;
        rv += data[len];
    }
    return rv;
}

u8 cks_add8(u8 *data, unsigned int len) {
    u16 sum = 0;
    for (; len; len--, data++) {
        sum += *data;
        if (sum & 0x100) sum += 1;
        sum = (u8) sum;
    }
    return sum;
}
