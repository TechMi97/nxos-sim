/* Driver for the NXT's LCD display.
 *
 * This driver contains a basic SPI driver to talk to the UltraChip
 * 1601 LCD controller, as well as a higher level API implementing the
 * UC1601's commandset.
 *
 * Note that the SPI driver is not suitable as a general-purpose SPI
 * driver: the MISO pin (Master-In Slave-Out) is instead wired to the
 * UC1601's CD input (used to select whether the transferred data is
 * control commands or display data). Thus, the SPI driver here takes
 * manual control of the MISO pin, and drives it depending on the type
 * of data being transferred.
 *
 * This also means that you can only write to the UC1601, not read
 * back from it. This is not too much of a problem, as we can just
 * introduce a little delay in the places where we really need it.
 */

#include "at91sam7s256.h"

#include "mytypes.h"
#include "interrupts.h"
#include "systick.h"
#include "aic.h"
#include "lcd.h"

/* Internal command bytes implementing part of the basic commandset of
 * the UC1601.
 */
#define SET_COLUMN_ADDR0(addr) (0x00 | (addr & 0xF))
#define SET_COLUMN_ADDR1(addr) (0x10 | ((addr >> 4) & 0xF))
#define SET_MULTIPLEX_RATE(rate) (0x20 | (rate & 3))
#define SET_SCROLL_LINE(sl) (0x40 | (sl & 0x3F))
#define SET_PAGE_ADDR(page) (0xB0 | (page & 0xF))
#define SET_BIAS_POT0() (0x81)
#define SET_BIAS_POT1(pot) (pot & 0xFF)
#define SET_RAM_ADDR_CONTROL(auto_wrap, page_first, neg_inc, write_only_inc) \
           (0x88 | (auto_wrap << 0) | (page_first << 1) |                    \
            (neg_inc << 2) | (write_only_inc << 3))
#define SET_ALL_PIXELS_ON(on) (0xA4 | (on & 1))
#define SET_INVERSE_DISPLAY(on) (0xA6 | (on & 1))
#define ENABLE(on) (0xAE | (on & 1))
#define SET_MAP_CONTROL(mx, my) (0xC0 | (mx << 1) | (my << 2))
#define RESET() (0xE2)
#define SET_BIAS_RATIO(bias) (0xE8 | (bias & 3))


/*
 * SPI controller driver.
 */
typedef enum spi_mode {
  COMMAND,
  DATA
} spi_mode;

static volatile struct {
  /* TRUE if the SPI driver is configured for sending commands, FALSE
   * if it's configured for sending video data. */
  spi_mode mode;

  /* TRUE if the LCD controller is currently the selected chip. */
  bool selected;

  /* State used by the display update code to manage the DMA
   * transfer. */
  U8 *data;
  U8 page;
  bool send_padding;
} spi_state = { DATA, FALSE, NULL, 0, FALSE };


/*
 * Set the selection state of the LCD controller
 */
static void spi_set_select(bool selected) {
  if (spi_state.selected == selected)
    return;

  spi_state.selected = selected;

  if (selected) {
    /* Flip the chip-select line... Because the uc1601 has a bug and
       needs to be pinged a little more directly? */
    *AT91C_PIOA_SODR = AT91C_PA10_NPCS2;
    *AT91C_PIOA_CODR = AT91C_PA10_NPCS2;
  } else {
    /* Set the chip select line high to deselect the uc1601. */
    *AT91C_PIOA_SODR = AT91C_PA10_NPCS2;
  }
}

/*
 * Set the data transmission mode.
 */
static void spi_set_tx_mode(spi_mode mode) {
  if (spi_state.mode == mode) {
    return;
  } else {
    /* If there is a mode switch, we need to let the SPI controller
     * drain all data first, to avoid spurious writes of the wrong
     * type.
     */
    while(!(*AT91C_SPI_SR & AT91C_SPI_TXEMPTY));
  }

  spi_state.mode = mode;

  if (mode == COMMAND) {
    *AT91C_PIOA_CODR = AT91C_PA12_MISO;
  } else {
    *AT91C_PIOA_SODR = AT91C_PA12_MISO;
  }
}


/*
 * Send a command byte to the LCD controller.
 */
static void spi_write_command_byte(U8 command) {
  spi_set_select(TRUE);
  spi_set_tx_mode(COMMAND);

  /* Send the command byte and wait for a reply. */
  *AT91C_SPI_TDR = command;

  /* Wait for the transmission to complete. */
  while (!(*AT91C_SPI_SR & AT91C_SPI_TXEMPTY));
}


/*
 * Send data bytes to the LCD controller.
 */
void spi_write_data(const U8 *data, U32 len) {
  spi_set_select(TRUE);
  spi_set_tx_mode(DATA);

  while (len) {
    /* Send the command byte and wait for a reply. */
    *AT91C_SPI_TDR = *data;
    data++;
    len--;

    /* Wait for the transmission to complete. */
    while (!(*AT91C_SPI_SR & AT91C_SPI_TXEMPTY)); /* TODO: Maybe just
                                                     wait for TDRE?
                                                     Would double data
                                                     rate... */
  }
}


/* Interrupt routine for handling DMA screen refreshing. */
void spi_isr() {
  if (spi_state.page == 0 && !spi_state.send_padding) {
    /* If this is the first page of data we're transferring this
     * refresh, we need to reset the video RAM pointers to zero.
     *
     * Once that is configured, we'll be pushing data only for the
     * rest of the screen refresh cycle, so we switch to data TX mode
     * once here.
     */
    spi_write_command_byte(SET_COLUMN_ADDR0(0));
    spi_write_command_byte(SET_COLUMN_ADDR1(0));
    spi_write_command_byte(SET_PAGE_ADDR(0));
    spi_set_tx_mode(DATA);
  } else if (spi_state.page == 7 && spi_state.send_padding) {
    /* If we would be sending the padding for the last page (useless),
     * just inhibit the DMA interrupt source and end the refresh
     * cycle.
     */
    *AT91C_SPI_IDR = AT91C_SPI_ENDTX;
    return;
  }

  if (!spi_state.send_padding) {
    /* We are at the start of a page, so we need to send 100 bytes of
     * pixel data to display. We also set the state for the next
     * interrupt, which is to send end-of-page padding.
     */
    spi_state.send_padding = TRUE;
    *AT91C_SPI_TNPR = (U32)spi_state.data;
    *AT91C_SPI_TNCR = 100;
  } else {
    /* 100 bytes of displayable data have been transferred. We now
     * have to send 32 more bytes to get to the end of the page and
     * wrap around. We also set up the state for the next interrupt,
     * which is to send the visible part of the next page of data.
     *
     * Given that this data is off-screen, we just resend the last 32
     * bytes of the 100 we just transferred.
     */
    spi_state.page += 1;
    spi_state.data += 100;
    spi_state.send_padding = FALSE;
    *AT91C_SPI_TNPR = (U32)(spi_state.data - 32);
    *AT91C_SPI_TNCR = 32;
  }
}


static void spi_init() {
  interrupts_disable();

  /* Enable power to the SPI and PIO controllers. */
  *AT91C_PMC_PCER = (1 << AT91C_ID_SPI) | (1 << AT91C_ID_PIOA);

  /* Configure the PIO controller: Hand the MOSI (Master Out, Slave
   * In) and SPI clock pins over to the SPI controller, but keep MISO
   * (Master In, Slave Out) and PA10 (Chip Select in this case) and
   * configure them for output.
   *
   * The initial configuration is data mode (sending video data) and
   * the LCD controller chip not selected.
   */
  *AT91C_PIOA_PDR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;
  *AT91C_PIOA_ASR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;

  *AT91C_PIOA_PER = AT91C_PA12_MISO | AT91C_PA10_NPCS2;
  *AT91C_PIOA_OER = AT91C_PA12_MISO | AT91C_PA10_NPCS2;
  *AT91C_PIOA_SODR = AT91C_PA12_MISO | AT91C_PA10_NPCS2;

  /* Disable all SPI interrupts, then configure the SPI controller in
   * master mode, with the chip select locked to chip 0 (UC1601 LCD
   * controller), communication at 2MHz, 8 bits per transfer and an
   * inactive-high clock signal.
   */
  *AT91C_SPI_CR = AT91C_SPI_SWRST;
  *AT91C_SPI_CR = AT91C_SPI_SPIEN;
  *AT91C_SPI_IDR = ~0;
  *AT91C_SPI_MR = (6 << 24) | AT91C_SPI_MSTR;
  AT91C_SPI_CSR[0] = ((0x18 << 24) | (0x18 << 16) | (0x18 << 8) |
                      AT91C_SPI_BITS_8 | AT91C_SPI_CPOL);


  /* Install an interrupt handler for the SPI controller, and enable
   * DMA transfers for SPI data. All SPI-related interrupt sources are
   * inhibited, so it won't bother us until we're ready.
   */
  aic_install_isr(AT91C_ID_SPI, AIC_INT_LEVEL_LOW, spi_isr);
  *AT91C_SPI_PTCR = AT91C_PDC_TXTEN;

  interrupts_enable();
}


/* Initialize the LCD controller. */
void lcd_init() {
  int i;
  /* This is the command byte sequence that should be sent to the LCD
   * after a reset.
   */
  const U8 lcd_init_sequence[] = {
    /* LCD power configuration.
     *
     * The LEGO Hardware Developer Kit documentation specifies that the
     * display should be configured with a multiplex rate (MR) of 1/65,
     * and a bias ratio (BR) of 1/9, and a display voltage V(LCD) of 9V.
     *
     * The specified MR and BR both map to simple command writes. V(LCD)
     * however is determined by an equation that takes into account both
     * the BR and the values of the PM (Potentiometer) and TC
     * (Temperature Compensation) configuration parameters.
     *
     * The equation and calculations required are a little too complex
     * to inline here, but the net result is that we should set a PM
     * value of 92. This will result in a smooth voltage gradient, from
     * 9.01V at -20 degrees Celsius to 8.66V at 60 degrees Celsius
     * (close to the maximum operational range of the LCD display).
     */
    SET_MULTIPLEX_RATE(3),
    SET_BIAS_RATIO(3),
    SET_BIAS_POT0(),
    SET_BIAS_POT1(92),

    /* Set the RAM address control, which defines how the data we send
     * to the LCD controller are placed in its internal video RAM.
     *
     * We want the bytes we send to be written in row-major order (line
     * by line), with no automatic wrapping.
     */
    SET_RAM_ADDR_CONTROL(1, 0, 0, 0),

    /* Set the LCD mapping mode, which defines how the data in video
     * RAM is driven to the display. The display on the NXT is mounted
     * upside down, so we want just Y mirroring.
     */
    SET_MAP_CONTROL(0, 1),

    /* Turn the display on. */
    ENABLE(1),
  };

  /* Initialize the SPI controller to enable communication, then wait
   * a little bit for the UC1601 to register the new SPI bus state.
   */
  spi_init();
  systick_wait_ms(20);

  /* Issue a reset command, and wait. Normally here we'd check the
   * UC1601 status register, but as noted at the start of the file, we
   * can't read from the LCD controller due to the board setup.
   */
  spi_write_command_byte(RESET());
  systick_wait_ms(20);

  for (i=0; i<sizeof(lcd_init_sequence); i++)
    spi_write_command_byte(lcd_init_sequence[i]);
}


/* Mirror the given display buffer to the LCD controller. The given
 * buffer must be exactly 100x64 bytes, one full screen of pixels.
 */
void lcd_display_data(U8 *display_buffer) {
  *AT91C_SPI_IDR = AT91C_SPI_ENDTX;

  spi_state.data = display_buffer;
  spi_state.page = 0;
  spi_state.send_padding = FALSE;

  *AT91C_SPI_IER = AT91C_SPI_ENDTX;
/*   int i; */

/*   for (i=0; i<LCD_HEIGHT; i++) { */
/*     spi_write_command_byte(SET_PAGE_ADDR(i)); */
/*     spi_write_command_byte(SET_COLUMN_ADDR0(0)); */
/*     spi_write_command_byte(SET_COLUMN_ADDR1(0)); */

/*     /\* Write a single page (8x100 pixels) of data. *\/ */
/*     spi_write_data(display_buffer, LCD_WIDTH); */
/*     display_buffer += LCD_WIDTH; */
/*   } */
}


/* Shutdown the LCD controller. */
void lcd_shutdown() {
  /* When power to the controller goes out, there is the risk that
   * some capacitors mounted around the controller might damage it
   * when discharging in an uncontrolled fashion. To avoid this, the
   * spec recommends putting the controller into reset mode before
   * shutdown, which activates a drain circuit to empty the board
   * capacitors gracefully.
   */
  *AT91C_SPI_IDR = ~0;
  *AT91C_SPI_PTCR = AT91C_PDC_TXTDIS;
  spi_write_command_byte(RESET());
  systick_wait_ms(20);
}
