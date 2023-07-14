/* stuff for receiving commands etc */

/* (c) copyright fenugrec 2016
 * GPLv3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FLASH.h"
#include "SH7052/IODEF.h"
#include "VEHICLE_SELECTOR.h"
#include "stypes.h"

#include <string.h>	//memcpy


#include "cmd_parser.h"

#include "eep_funcs.h"
#include "iso_cmds.h"
#include "errcodes.h"
#include "crc.h"
#include "SERIAL.h"
#include "WDT.h"

#define MAX_INTERBYTE	10	//ms between bytes that causes a disconnect

/* concatenate the ReadECUID positive response byte
 * in front of the version string
 */
const u8 npk_ver_string[] = SID_RECUID_PRC;

/* low-level error code, to give more detail about errors than the SID 7F NRC can provide,
 * without requiring the error string list of nisprog to be updated.
 */
u8 lasterr = 0;

/* generic buffer to construct responses. Saves a lot of stack vs
 * each function declaring its buffer as a local var : gcc tends to inline everything
 * but not combine /overlap each buffer.
 * We just need to make sure the comms functions (iso_sendpkt, tx_7F etc) use their own
 * private buffers. */
u8 txbuf[256];


void set_lasterr(u8 err) {
	lasterr = err;
}

/* sign-extend 24bit number to 32bits,
 * i.e. FF8000 => FFFF8000 etc
 * data stored as big (sh) endian
 */
u32 reconst_24(const u8 *data) {
	u32 tmp;
	tmp = (data[0] << 16) | (data[1] << 8) | data[2];
	if (data[0] & 0x80) {
		//sign-extend to cover RAM
		tmp |= 0xFF << 24;
	}
	return tmp;
}

enum iso_prc { ISO_PRC_ERROR, ISO_PRC_NEEDMORE, ISO_PRC_DONE };
/** Add newly-received byte to msg;
 *
 * @return ISO_PRC_ERROR if bad header, bad checksum, or overrun (caller's fault)
 *	ISO_PRC_NEEDMORE if ok but msg not complete
 *	ISO_PRC_DONE when msg complete + good checksum
 *
 * Note : the *msg->hi, ->di, ->hdrlen, ->datalen memberes must be set to 0 before parsing a new message
 */

enum iso_prc iso_parserx(struct iso14230_msg *msg, u8 newbyte) {
	u8 dl;

	// 1) new msg ?
	if (msg->hi == 0) {
		msg->hdrlen = 1;	//at least 1 byte (FMT)

		//parse FMT byte
		if ((newbyte & 0xC0) == 0x40) {
			//CARB mode, not supported
			return ISO_PRC_ERROR;
		}
		if (newbyte & 0x80) {
			//addresses supplied
			msg->hdrlen += 2;
		}

		dl = newbyte & 0x3f;
		if (dl == 0) {
			/* Additional length byte present */
			msg->hdrlen += 1;
		} else {
			/* len-in-fmt : we can set length already */
			msg->datalen = dl;
		}
	}

	// 2) add to header if required
	if (msg->hi != msg->hdrlen) {
		msg->hdr[msg->hi] = newbyte;
		msg->hi += 1;
		// fetch LEN byte if applicable
		if ((msg->datalen == 0) && (msg->hi == msg->hdrlen)) {
			msg->datalen = newbyte;
		}
		return ISO_PRC_NEEDMORE;
	}

	// ) here, header is complete. Add to data
	msg->data[msg->di] = newbyte;
	msg->di += 1;

	// +1 because we need checksum byte too
	if (msg->di != (msg->datalen + 1)) {
		return ISO_PRC_NEEDMORE;
	}

	// ) data now complete. valide cks
	u8 cks = cks_u8(msg->hdr, msg->hdrlen);
	cks += cks_u8(msg->data, msg->datalen);
	if (cks == msg->data[msg->datalen]) {
		return ISO_PRC_DONE;
	}
	return ISO_PRC_ERROR;
}


/* Command state machine */
enum t_cmdsm {
	CM_IDLE,		//not initted, only accepts the "startComm" request
	CM_READY,		//initted, accepts all commands

} cmstate;

/* flash state machine */
enum t_flashsm {
	FL_IDLE,
	FL_READY,	//after doing init.
} flashstate;

/* initialize command parser state machine;
 * updates SCI settings : 62500 bps
 * beware the FER error flag, it disables further RX. So when changing BRR, if the host sends a byte
 * FER will be set, etc.
 */

void cmd_init(u8 brrdiv) {
	cmstate = CM_IDLE;
	flashstate = FL_IDLE;
    TARGET_SCI.SCR.BYTE &= 0xCF;	//disable TX + RX
    TARGET_SCI.BRR = brrdiv;		// speed = (div + 1) * 625k
    TARGET_SCI.SSR.BYTE &= 0x87;	//clear RDRF + error flags
    TARGET_SCI.SCR.BYTE |= 0x30;	//enable TX+RX , no RX interrupts for now
	return;
}

void cmd_startcomm(void) {
	// KW : noaddr;  len-in-fmt or lenbyte
	static const u8 startcomm_resp[3] = {0xC1, 0x67, 0x8F};
	iso_sendpkt(startcomm_resp, 3);
	flashstate = FL_IDLE;
}

/* dump command processor, called from cmd_loop.
 * args[0] : address space (0: EEPROM, 1: ROM)
 * args[1,2] : # of 32-byte blocks
 * args[3,4] : (address / 32)
 *
 * EEPROM addresses are interpreted as the flattened memory, i.e. 93C66 set as 256 * 16bit will
 * actually be read as a 512 * 8bit array, so block #0 is bytes 0 to 31 == words 0 to 15.
 *
 * ex.: "00 00 02 00 01" dumps 64 bytes @ EEPROM 0x20 (== address 0x10 in 93C66)
 * ex.: "01 80 00 00 00" dumps 1MB of ROM@ 0x0
 *
 */
void cmd_dump(struct iso14230_msg *msg) {
	u32 addr;
	u32 len;
	u8 space;
	u8 *args = &msg->data[1];	//skip SID byte

	if (msg->datalen != 6) {
		tx_7F(SID_DUMP, ISO_NRC_SFNS_IF);
		return;
	}

	space = args[0];
	len = 32 * ((args[1] << 8) | args[2]);
	addr = 32 * ((args[3] << 8) | args[4]);
	switch (space) {
	case SID_DUMP_EEPROM:
		/* dump eeprom stuff */
		addr /= 2;	/* modify address to fit with eeprom 256*16bit org */
		len &= ~1;	/* align to 16bits */
		while (len) {
			u16 pbuf[17];
			u8 *pstart;	//start of ISO packet
			u16 *ebuf=&pbuf[1];	//cheat : form an ISO packet with the pos resp code in pbuf[0]

			int pktlen;
			int ecur;

			pstart = (u8 *)(pbuf) + 1;
			*pstart = SID_DUMP + 0x40;

			pktlen = len;
			if (pktlen > 32) pktlen = 32;

			for (ecur = 0; ecur < (pktlen / 2); ecur += 1) {
				eep_read16((uint8_t) addr + ecur, (uint16_t *)&ebuf[ecur]);
			}
			iso_sendpkt(pstart, pktlen + 1);

			len -= pktlen;
			addr += (pktlen / 2);	//work in eeprom addresses
		}
		break;
	case SID_DUMP_ROM:
		/* dump from ROM */
		txbuf[0] = SID_DUMP + 0x40;
		while (len) {
			int pktlen;
			pktlen = len;
			if (pktlen > 32) pktlen = 32;
			memcpy(&txbuf[1], (void *) addr, pktlen);
			iso_sendpkt(txbuf, pktlen + 1);
			len -= pktlen;
			addr += pktlen;
		}
		break;
	default:
		tx_7F(SID_DUMP, ISO_NRC_SFNS_IF);
		break;
	}	//switch (space)

	return;
}


/* SID 34 : prepare for reflashing */
void cmd_flash_init(void) {
	u8 errval;

	if (!platf_flash_init(&errval)) {
		tx_7F(SID_FLREQ, errval);
		return;
	}

	txbuf[0] = (SID_FLREQ + 0x40);
	iso_sendpkt(txbuf, 1);
	flashstate = FL_READY;
	return;
}

/* compare given CRC with calculated value.
 * data is the first byte after SID_CONF_CKS1
 */
int cmd_romcrc(const u8 *data) {
	unsigned idx;
	// <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
	u16 chunkno = (*(data+0) << 8) | *(data+1);
	for (idx = 0; idx < ROMCRC_NUMCHUNKS; idx++) {
		u16 crc;
		data += 2;
		u16 test_crc = (*(data+0) << 8) | *(data+1);
		u32 start = chunkno * ROMCRC_CHUNKSIZE;
		crc = crc16((const u8 *)start, ROMCRC_CHUNKSIZE);
		if (crc != test_crc) {
			return -1;
		}
		chunkno += 1;
	}

	return 0;
}

/* handle low-level reflash commands */
void cmd_flash_utils(struct iso14230_msg *msg) {
	u8 subcommand;
	u32 tmp;

	u32 rv = ISO_NRC_GR;

	if (flashstate != FL_READY) {
		rv = ISO_NRC_CNCORSE;
		goto exit_bad;
	}

	if (msg->datalen <= 1) {
		rv = ISO_NRC_SFNS_IF;
		goto exit_bad;
	}

	subcommand = msg->data[1];

	switch(subcommand) {
	case SIDFL_EB:
		//format : <SID_FLASH> <SIDFL_EB> <BLOCKNO>
		if (msg->datalen != 3) {
			rv = ISO_NRC_SFNS_IF;
			goto exit_bad;
		}
		rv = platf_flash_eb(msg->data[2]);
		if (rv) {
			rv = (rv & 0xFF) | 0x80;	//make sure it's a valid extented NRC
			goto exit_bad;
		}
		break;
	case SIDFL_WB:
		//format : <SID_FLASH> <SIDFL_WB> <A2> <A1> <A0> <D0>...<D127> <CRC>
		if (msg->datalen != (SIDFL_WB_DLEN + 6)) {
			rv = ISO_NRC_SFNS_IF;
			goto exit_bad;
		}

		if (cks_add8(&msg->data[2], (SIDFL_WB_DLEN + 3)) != msg->data[SIDFL_WB_DLEN + 5]) {
			rv = SID_CONF_CKS1_BADCKS;	//crcerror
			goto exit_bad;
		}

		tmp = (msg->data[2] << 16) | (msg->data[3] << 8) | msg->data[4];
		rv = platf_flash_wb(tmp, (u32) &msg->data[5], SIDFL_WB_DLEN);
		if (rv) {
			rv = (rv & 0xFF) | 0x80;	//make sure it's a valid extented NRC
			goto exit_bad;
		}
		break;
	case SIDFL_UNPROTECT:
		//format : <SID_FLASH> <SIDFL_UNPROTECT> <~SIDFL_UNPROTECT>
		if (msg->datalen != 3) {
			rv = ISO_NRC_SFNS_IF;
			goto exit_bad;
		}
		if (msg->data[2] != (u8) ~SIDFL_UNPROTECT) {
			rv = ISO_NRC_IK;	//InvalidKey
			goto exit_bad;
		}

		platf_flash_unprotect();
		break;
	default:
		rv = ISO_NRC_SFNS_IF;
		goto exit_bad;
		break;
	}

	txbuf[0] = SID_FLASH + 0x40;
	iso_sendpkt(txbuf, 1);	//positive resp
	return;

exit_bad:
	tx_7F(SID_FLASH, rv);
	return;
}


/* ReadMemByAddress */
void cmd_rmba(struct iso14230_msg *msg) {
	//format : <SID_RMBA> <AH> <AM> <AL> <SIZ>
	/* response : <SID + 0x40> <D0>....<Dn> <AH> <AM> <AL> */

	u32 addr;
	int siz;

	if (msg->datalen != 5) goto bad12;
	siz = msg->data[4];

	if ((siz == 0) || (siz > 251)) goto bad12;

	addr = reconst_24(&msg->data[1]);

	txbuf[0] = SID_RMBA + 0x40;
	memcpy(txbuf + 1, (void *) addr, siz);

	siz += 1;
	txbuf[siz++] = msg->data[1];
	txbuf[siz++] = msg->data[2];
	txbuf[siz++] = msg->data[3];

	iso_sendpkt(txbuf, siz);
	return;

bad12:
	tx_7F(SID_RMBA, ISO_NRC_SFNS_IF);
	return;
}


/* WriteMemByAddr - RAM only */
void cmd_wmba(struct iso14230_msg *msg) {
	/* WriteMemByAddress (RAM only !) . format : <SID_WMBA> <AH> <AM> <AL> <SIZ> <DATA> , siz <= 250. */
	/* response : <SID + 0x40> <AH> <AM> <AL> */
	u8 rv = ISO_NRC_SFNS_IF;
	u32 addr;
	u8 siz;
	u8 *src;

	if (msg->datalen < 6) goto badexit;
	siz = msg->data[4];

	if (	(siz == 0) ||
		(siz > 250) ||
		(msg->datalen != (siz + 5))) goto badexit;

	addr = reconst_24(&msg->data[1]);

	// bounds check, restrict to RAM
	if (	(addr < RAM_MIN) ||
		(addr > RAM_MAX)) {
		rv = ISO_NRC_CNDTSA; /* canNotDownloadToSpecifiedAddress */
		goto badexit;
	}

	/* write */
	src = &msg->data[5];
	memcpy((void *) addr, src, siz);

	msg->data[0] = SID_WMBA + 0x40;	//cheat !
	iso_sendpkt(msg->data, 4);
	return;

badexit:
	tx_7F(SID_WMBA, rv);
	return;
}

/* set & configure kernel */
void cmd_conf(struct iso14230_msg *msg) {
	u8 resp[4];
	u32 tmp;

	resp[0] = SID_CONF + 0x40;
	if (msg->datalen < 2) goto bad12;

	switch (msg->data[1]) {
	case SID_CONF_SETSPEED:
		/* set comm speed (BRR divisor reg) : <SID_CONF> <SID_CONF_SETSPEED> <new divisor> */
		iso_sendpkt(resp, 1);
		cmd_init(msg->data[2]);
		sci_rxidle(25);
		return;
		break;
	case SID_CONF_SETEEPR:
		/* set eeprom_read() function address <SID_CONF> <SID_CONF_SETEEPR> <AH> <AM> <AL> */
		if (msg->datalen != 5) goto bad12;
		tmp = (msg->data[2] << 16) | (msg->data[3] << 8) | msg->data[4];
		eep_setptr(tmp);
		iso_sendpkt(resp, 1);
		return;
		break;
	case SID_CONF_CKS1:
		//<SID_CONF> <SID_CONF_CKS1> <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
		if (msg->datalen != 12) {
			goto bad12;
		}
		if (cmd_romcrc(&msg->data[2])) {
			tx_7F(SID_CONF, SID_CONF_CKS1_BADCKS);
			return;
		}
		iso_sendpkt(resp, 1);
		return;
		break;
	case SID_CONF_LASTERR:
		resp[1] = lasterr;
		lasterr = 0;
		iso_sendpkt(resp, 2);
		return;
		break;
#ifdef DIAG_U16READ
	case SID_CONF_R16:
		{
		u16 val;
		//<SID_CONF> <SID_CONF_R16> <A2> <A1> <A0>
		tmp = reconst_24(&msg->data[2]);
		tmp &= ~1;	//clr lower bit of course
		val = *(const u16 *) tmp;
		resp[1] = val >> 8;
		resp[2] = val & 0xFF;
		iso_sendpkt(resp,3);
		return;
		break;
		}
#endif
	default:
		goto bad12;
		break;
	}

bad12:
	tx_7F(SID_CONF, ISO_NRC_SFNS_IF);
	return;
}


/* command parser; infinite loop waiting for commands.
 * not sure if it's worth the trouble to make this async,
 * what other tasks could run in background ? reflash shit ?
 *
 * This receives valid iso14230 packets; message splitting is by pkt length
 */
void cmd_loop(void) {
	u8 rxbyte;

	static struct iso14230_msg msg;

	//u32 t_last, t_cur;	//timestamps

	iso_clearmsg(&msg);

	while (1) {
		enum iso_prc prv;

		/* in case of errors (ORER | FER | PER), reset state mach. */
        if (TARGET_SCI.SSR.BYTE & 0x38) {

			cmstate = CM_IDLE;
			flashstate = FL_IDLE;
			iso_clearmsg(&msg);
			sci_rxidle(MAX_INTERBYTE);
			continue;
		}

        if (!TARGET_SCI.SSR.BIT.RDRF) continue;

        rxbyte = TARGET_SCI.RDR;
        TARGET_SCI.SSR.BIT.RDRF = 0;

		//t_cur = get_mclk_ts();	/* XXX TODO : filter out interrupted messages with t>5ms interbyte ? */

		/* got a byte; parse according to state */
		prv = iso_parserx(&msg, rxbyte);

		if (prv == ISO_PRC_NEEDMORE) {
			continue;
		}
		if (prv != ISO_PRC_DONE) {
			iso_clearmsg(&msg);
			sci_rxidle(MAX_INTERBYTE);
			continue;
		}
		/* here, we have a complete iso frame */

		switch (cmstate) {
		case CM_IDLE:
			/* accept only startcomm requests */
			if (msg.data[0] == SID_STARTCOMM) {
				cmd_startcomm();
				cmstate = CM_READY;
			}
			iso_clearmsg(&msg);
			break;

		case CM_READY:
			switch (msg.data[0]) {
			case SID_STARTCOMM:
				cmd_startcomm();
				iso_clearmsg(&msg);
				break;
			case SID_RECUID:
				iso_sendpkt(npk_ver_string, sizeof(npk_ver_string));
				iso_clearmsg(&msg);
				break;
			case SID_CONF:
				cmd_conf(&msg);
				iso_clearmsg(&msg);
				break;
			case SID_RESET:
				/* ECUReset */
				txbuf[0] = msg.data[0] + 0x40;
				iso_sendpkt(txbuf, 1);
				die();
				break;
			case SID_RMBA:
				cmd_rmba(&msg);
				iso_clearmsg(&msg);
				break;
			case SID_WMBA:
				cmd_wmba(&msg);
				iso_clearmsg(&msg);
				break;
			case SID_DUMP:
				cmd_dump(&msg);
				iso_clearmsg(&msg);
				break;
			case SID_FLASH:
				cmd_flash_utils(&msg);
				iso_clearmsg(&msg);
				break;
			case SID_TP:
				txbuf[0] = msg.data[0] + 0x40;
				iso_sendpkt(txbuf, 1);
				iso_clearmsg(&msg);
				break;
			case SID_FLREQ:
				cmd_flash_init();
				iso_clearmsg(&msg);
				break;
			default:
				tx_7F(msg.data[0], ISO_NRC_SNS);
				iso_clearmsg(&msg);
				break;
			}	//switch (SID)
			break;
		default :
			//invalid state, or nothing special to do
			break;
		}	//switch (cmstate)
	}	//while 1

	die();
}
