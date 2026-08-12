#ifndef TS_L2_DEFS_H
#define TS_L2_DEFS_H

// Basic definitions of TS messaging over SPI.
// Most of commands and constants are defined in generated header file.
// Commands defines and description in YAML file <tassic.git>/doc/user_api/tropic01_L2_api.yml and tropic01_L3_api.yml

// Primary protocol description :
//   https://docs.google.com/document/d/1PCTmTOSdCclg26J7MoLacZzenGn4NipQt7sGttiN2GM/edit#heading=h.dada9j6je591

typedef s32 ts_l2_result_t;
typedef u8  ts_l2_len_t;


#define TS_L2_IDX_HDR  (0) // CMD for input or RESP for output
#define TS_L2_IDX_LEN  (1)
#define TS_L2_IDX_DATA (2)

// message length will be usualy limited by command description, this is protocol maximum
#define TS_L2_MAX_LEN_DATA (252) // 1 byte field LEN, we set the value to fit whole L2 message to 256 bytes
#define TS_L2_LEN_CRC      (2)   // 16bit CRC
#define TS_L2_MAX_LEN_PACKET (TS_L2_IDX_DATA+TS_L2_MAX_LEN_DATA+TS_L2_LEN_CRC) // <HDR><LEN><DATA(LEN)><CRC(2)>

// packet commands (headers)
#define TS_L2_CMD_NONE             (0x00) // invalid header

#define TS_L2_GET_RESP             (0xAA) // NOTE: command handled by HW

// rest of TS_L2_* defs in generated api.h from YAML file

// response status
#define TS_L2_RESP_NONE            (0x00) // invalid value
#define TS_L2_RESP_REQ_OK          (0x01)
#define TS_L2_RESP_CMD_DONE        (0x02)
#define TS_L2_RESP_REQ_CONT        (0x03)
#define TS_L2_RESP_RES_CONT        (0x04)

#define TS_L2_RESP_DISABLED        (0x78)
#define TS_L2_RESP_HSK_ERR         (0x79)
#define TS_L2_RESP_NO_SESSION      (0x7A)
#define TS_L2_RESP_TAG_ERR         (0x7B)
#define TS_L2_RESP_CRC_ERR         (0x7C)
#define TS_L2_RESP_LEN_ERR         (0x7D)
#define TS_L2_RESP_UNKNOWN_REQ     (0x7E)
#define TS_L2_RESP_GEN_ERR         (0x7F)

// special values
#define TS_L2_RESP_NO_RESP         (0xFF) // nothing to respond yet
// NOTE: handled by HW, therefore NO_RESP is exception and there is no CRC (not valid packet)

#endif // ! TS_L2_DEFS_H

