#include "base/at91sam7s256.h"

#include "base/mytypes.h"

#include "base/drivers/systick.h"
#include "base/drivers/uart.h"
#include "base/drivers/bt.h"


/* to remove : */
#ifdef UART_DEBUG
#include "base/display.h"
#include "base/drivers/usb.h"
#include "base/util.h"
#define CMDS_BUFSIZE 128
#define USB_SEND(txt) usb_send((U8*)txt, strlen(txt))
#else
#define CMDS_BUFSIZE 0
#define USB_SEND(txt)
#endif


#define BT_RST_PIN AT91C_PIO_PA11
#define BT_ARM_CMD_PIN AT91C_PIO_PA27
/* BT_BC4_CMD is connected to the channel 4 of the Analog to Digital converter */
#define BT_CS_PIN   AT91C_PIO_PA31


typedef enum {
  /* AMR7 -> BC4 */
  BT_MSG_BEGIN_INQUIRY = 0x00,
  BT_MSG_CANCEL_INQUIRY = 0x01,
  BT_MSG_CONNECT = 0x02,
  BT_MSG_OPEN_PORT = 0x03,
  BT_MSG_LOOKUP_NAME = 0x04,
  BT_MSG_ADD_DEVICE = 0x05,
  BT_MSG_REMOVE_DEVICE = 0x06,
  BT_MSG_DUMP_LIST = 0x07,
  BT_MSG_CLOSE_CONNECTION = 0x08,
  BT_MSG_ACCEPT_CONNECTION = 0x09,
  BT_MSG_PIN_CODE = 0x0A,
  BT_MSG_OPEN_STREAM = 0x0B,
  BT_MSG_START_HEART = 0x0C,
  BT_MSG_SET_DISCOVERABLE = 0x1C,
  BT_MSG_CLOSE_PORT = 0x1D,
  BT_MSG_SET_FRIENDLY_NAME = 0x21,
  BT_MSG_GET_LINK_QUALITY = 0x23,
  BT_MSG_SET_FACTORY_SETTINGS = 0x25,
  BT_MSG_GET_LOCAL_ADDR = 0x26,
  BT_MSG_GET_FRIENDLY_NAME = 0x29,
  BT_MSG_GET_DISCOVERABLE = 0x2A,
  BT_MSG_GET_PORT_OPEN = 0x2B,
  BT_MSG_GET_VERSION = 0x2F,
  BT_MSG_GET_BRICK_STATUS_BYTE = 0x33,
  BT_MSG_SET_BRICK_STATUS_BYTE = 0x34,
  BT_MSG_GET_OPERATING_MODE = 0x35,
  BT_MSG_SET_OPERATING_MODE = 0x36,
  BT_MSG_GET_CONNECTION_STATUS = 0x38,
  BT_MSG_GOTO_DFU_MODE = 0x3A,

  /* BC4 -> ARM7 */
  BT_MSG_HEARTBEAT = 0x0D,
  BT_MSG_INQUIRY_RUNNING = 0x0E,
  BT_MSG_INQUIRY_RESULT = 0x0F,
  BT_MSG_INQUIRY_STOPPED = 0x10,
  BT_MSG_LOOKUP_NAME_RESULT = 0x11,
  BT_MSG_LOOKUP_NAME_FAILURE = 0x12,
  BT_MSG_CONNECT_RESULT = 0x13,
  BT_MSG_RESET_INDICATION = 0x14,
  BT_MSG_REQUEST_PIN_CODE = 0x15,
  BT_MSG_REQUEST_CONNECTION = 0x16,
  BT_MSG_LIST_RESULT = 0x17,
  BT_MSG_LIST_ITEM = 0x18,
  BT_MSG_LIST_DUMP_STOPPED = 0x19,
  BT_MSG_CLOSE_CONNECTION_RESULT = 0x1A,
  BT_MSG_PORT_OPEN_RESULT = 0x1B,
  BT_MSG_CLOSE_PORT_RESULT = 0x1E,
  BT_MSG_PIN_CODE_ACK = 0x1F,
  BT_MSG_SET_DISCOVERABLE_ACK = 0x20,
  BT_MSG_SET_FRIENDLY_NAME_ACK = 0x22,
  BT_MSG_LINK_QUALITY_RESULT = 0x24,
  BT_MSG_SET_FACTORY_SETTINGS_ACK = 0x26,
  BT_MSG_GET_LOCAL_ADDR_RESULT = 0x28,
  BT_MSG_GET_FRIENDLY_NAME_RESULT = 0x2C,
  BT_MSG_GET_DISCOVERABLE_RESULT = 0x2D,
  BT_MSG_GET_PORT_OPEN_RESULT = 0x2E,
  BT_MSG_GET_VERSION_RESULT = 0X30,
  BT_MSG_GET_BRICK_STATUS_BYTE_RESULT = 0x31,
  BT_MSG_SET_BRICK_STATUS_BYTE_RESULT = 0x32,
  BT_MSG_OPERATING_MODE_RESULT = 0x37,
  BT_MSG_CONNECTION_STATUS_RESULT = 0x39,
} bt_msg_t;


static const U8 bt_msg_start_heart[] = {
  0x03, /* length */
  BT_MSG_START_HEART,
  0xFF, /* checksum (high) */
  0xF4  /* checksum (low) */
};


static const U8 bt_msg_get_version[] = {
  0x03, /* length */
  BT_MSG_GET_VERSION,
  0xFF, /* checksum (hi) */
  0xD1  /* checksum (lo) */
};


static const U8 bt_set_discoverable_true[] = {
  0x04, /* length */
  BT_MSG_SET_DISCOVERABLE,
  0x01, /* => true */
  0xFF,
  0xE3
};

static const U8 bt_set_discoverable_false[] = {
  0x04, /* length */
  BT_MSG_SET_DISCOVERABLE, /* set discoverable */
  0x00, /* => true */
  0xFF,
  0xE4
};



typedef enum {
  BT_STATE_WAITING = 0,
  BT_STATE_INQUIRING,
} bt_state_t;


static volatile struct {
  bt_state_t state; /* not used atm */

  /* see systick_get_ms() */
  U32 last_heartbeat;

  /* used for inquiring */

  /* TODO : Figure why there is this InquiryStopped to ignore */
  bool first_stop;
  U8 bt_remote_id;
  bt_device_t bt_remote_device;

#ifdef UART_DEBUG
  /* to remove: */
  U8 cmds[CMDS_BUFSIZE];
  U8 cmds_pos;
#endif

} bt_state = {
  0
};




/* len => checksum included
 */
static U16 bt_get_checksum(U8 *msg, U8 len, bool count_len)
{
  U8 i;
  U16 checksum;

  checksum = 0;

  /* from the second byte of the message
   * to the last byte before the checksum */
  for (i = 0 ; i < len-2 ; i++) {
    checksum += msg[i];
  }

  if (count_len)
    checksum += len;

  checksum = -checksum;

  return checksum;
}


/* len => length excepted, but checksum included
 * two last bytes will be set
 */
static void bt_set_checksum(U8 *msg, U8 len) {
  U16 checksum = bt_get_checksum(msg, len, FALSE);

  msg[len] = ((checksum >> 8) & 0xFF);
  msg[len+1] = checksum & 0xFF;
}


/* len => length excepted, but checksum included */
static bool bt_check_checksum(U8 *msg, U8 len) {

  /* Strangess: Must include the packet length in the checksum ?! */
  /* TODO : Figure this out */
  U16 checksum = bt_get_checksum(msg, len, TRUE);
  U8 hi, lo;

  hi = ((checksum >> 8) & 0xFF);
  lo = checksum & 0xFF;

  return (hi == msg[len-2] && lo == msg[len-1]);
}


static void bt_begin_inquiry(U8 max_devices,
                             U8 timeout,
                             U8 bt_remote_class[4])
{
  int i;
  U8 packet[10];

  packet[0] = 10;      /* length */
  packet[1] = BT_MSG_BEGIN_INQUIRY; /* begin inquiry */
  packet[2] = max_devices;
  packet[3] = 0;       /* timeout (hi) */
  packet[4] = timeout; /* timeout (lo) */

  for (i = 0 ; i < 4 ; i++)
    packet[5+i] = bt_remote_class[i];

  bt_set_checksum(packet+1, 10);

  uart_write(packet, 19);

  while(uart_is_writing()); /* to avoid some stack issues */

  bt_state.state = BT_STATE_INQUIRING;
}

static void bt_reseted()
{
  bt_state.first_stop = FALSE;
  bt_state.state = BT_STATE_WAITING;
}



static void bt_uart_callback(U8 *msg, U8 len)
{
  int i;

  /* if it's a break from the nxt */
  if (msg == NULL || len == 0)
    return;

  /* we check first the checksum and ignore the message if the checksum is invalid */
  if (len < 1 || !bt_check_checksum(msg, len))
    return;

#ifdef UART_DEBUG
  if (bt_state.cmds_pos < CMDS_BUFSIZE) {
    bt_state.cmds[bt_state.cmds_pos] = msg[0];
    bt_state.cmds_pos++;
  }
#endif


  if (msg[0] == BT_MSG_HEARTBEAT) {
    bt_state.last_heartbeat = systick_get_ms();
    return;
  }

  if (msg[0] == BT_MSG_INQUIRY_RUNNING) {
    bt_state.state = BT_STATE_INQUIRING;
    return;
  }

  if (msg[0] == BT_MSG_INQUIRY_RESULT) {
    /* we've found a device => we extract the fields */

    for (i = 0 ; i < BT_ADDR_SIZE ; i++)
      bt_state.bt_remote_device.addr[i] = msg[1+i];

    for (i = 0 ; i < BT_NAME_MAX_LNG ; i++)
      bt_state.bt_remote_device.name[i] = msg[1+7+i];
    bt_state.bt_remote_device.name[BT_NAME_MAX_LNG+1] = '\0';

    for (i = 0 ; i < BT_CLASS_SIZE ; i++)
      bt_state.bt_remote_device.class[i] = msg[1+7+16+i];

    bt_state.bt_remote_id++;
    return;
  }

  if (msg[0] == BT_MSG_RESET_INDICATION) {
    bt_reseted();
    return;
  }

  if (msg[0] == BT_MSG_INQUIRY_STOPPED) {
    if (!bt_state.first_stop)
      bt_state.first_stop = TRUE;
    else
      bt_reseted();
    return;
  }

}




void bt_init()
{
  USB_SEND("bt_init()");

  /* we put the ARM CMD pin to 0 => command mode */
  /* and we put the RST PIN to 0 => Reseting */
  *AT91C_PIOA_PER = BT_RST_PIN | BT_ARM_CMD_PIN;
  *AT91C_PIOA_PPUDR = BT_ARM_CMD_PIN;
  *AT91C_PIOA_CODR = BT_ARM_CMD_PIN;
  *AT91C_PIOA_CODR = BT_RST_PIN;
  *AT91C_PIOA_OER = BT_RST_PIN | BT_ARM_CMD_PIN;

  uart_init(bt_uart_callback);

  /* we release the reset pin => 1 */
  *AT91C_PIOA_SODR = BT_RST_PIN;

  systick_wait_ms(1000);

  uart_write(&bt_msg_start_heart, sizeof(bt_msg_start_heart));

  USB_SEND("bt_init() finished");
}



void bt_set_friendly_name(char *name)
{
  int i;
  U8 packet[20] = { 0 };

  packet[0] = 19; /* length */
  packet[1] = BT_MSG_SET_FRIENDLY_NAME;

  for (i = 0 ; i < 16 && name[i] != '\0' ; i++)
    packet[i+2] = name[i];

  bt_set_checksum(packet+1, 19);

  uart_write(packet, 19);

  while(uart_is_writing()); /* to avoid some stack issues */
}


void bt_set_discoverable(bool d)
{
  if (d)
    uart_write(&bt_set_discoverable_true, sizeof(bt_set_discoverable_true));
  else
    uart_write(&bt_set_discoverable_false, sizeof(bt_set_discoverable_false));
}




void bt_inquiry(bt_inquiry_callback_t callback,
                U8 max_devices,
                U8 timeout,
                U8 bt_remote_class[4])
{
  U8 last_id = 0;
  U32 start_time;

  if (timeout < 1 || timeout > 0x30)
    return;

  bt_begin_inquiry(max_devices, timeout, bt_remote_class);

  start_time = systick_get_ms();

  while(bt_state.state == BT_STATE_INQUIRING)
    {
      if (last_id != bt_state.bt_remote_id)
        {
          last_id = bt_state.bt_remote_id;
          callback((bt_device_t *)&bt_state.bt_remote_device);
        }

      if (start_time + (2*timeout*1000) < systick_get_ms()) {
        /* argh, sometimes went wrong */
        break;
      }
    }
}



/* to remove: */
void bt_debug()
{
  display_uint(bt_state.last_heartbeat);
  display_end_line();
  USB_SEND((char *)bt_state.cmds);
}


