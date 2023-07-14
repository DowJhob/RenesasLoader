#ifndef SERIAL_H
#define SERIAL_H

/* init SCI to continue comms on K line */

#include "stypes.h"

#define SCI_DEFAULTDIV 9	//default value for BRR reg. Speed (kbps) = (20 * 1000) / (32 * (BRR + 1))


/* make receiving slightly easier maybe */
struct iso14230_msg {
    int	hdrlen;		//expected header length : 1 (len-in-fmt), 2(fmt + len), 3(fmt+addr), 4(fmt+addr+len)
    int	datalen;	//expected data length
    int	hi;		//index in hdr[]
    int	di;		//index in data[]
    u8	hdr[4];
    u8	data[256];	//255 data bytes + checksum
};

extern void init_sci(void);

/** discard RX data until idle for a given time
 * @param idle : purge until interbyte > idle ms
 *
 * blocking, of course. Do not call from ISR
 */
extern void sci_rxidle(unsigned ms);

/** send a whole buffer, blocking. For use by iso_sendpkt() only */
extern void sci_txblock(const uint8_t *buf, uint32_t len);

/** Send a headerless iso14230 packet
 * @param len is clipped to 0xff
 *
 * disables RX during sending to remove halfdup echo. Should be reliable since
 * we re-enable after the stop bit, so K should definitely be back up to '1' again
 *
 * this is blocking
 */
extern void iso_sendpkt(const uint8_t *buf, int len);



/* transmit negative response, 0x7F <SID> <NRC>
 * Blocking
 */
extern void tx_7F(u8 sid, u8 nrc);


extern void iso_clearmsg(struct iso14230_msg *msg);


/** simple 8-bit sum */
extern uint8_t cks_u8(const uint8_t * data, unsigned int len);


/* "one's complement" checksum; if adding causes a carry, add 1 to sum. Slightly better than simple 8bit sum
 */
extern u8 cks_add8(u8 *data, unsigned len);


#endif // SERIAL_H
