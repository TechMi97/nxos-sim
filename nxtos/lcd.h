#ifndef __NXTOS_LCD_H__
#define __NXTOS_LCD_H__

/* Width and height of the LCD display. */
#define LCD_WIDTH 100 /* pixels */
#define LCD_HEIGHT 8 /* bytes, so 64 pixels. */

void lcd_init();
void lcd_display_data(U8 *display_buffer);
void lcd_shutdown();
void lcd_test();

#endif
