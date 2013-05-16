/**
 * @file Cosa/IOStream/Driver/PCD8544.hh
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2013, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * This file is part of the Arduino Che Cosa project.
 */

#ifndef __COSA_IOSTREAM_DRIVER_ST7565P_HH__
#define __COSA_IOSTREAM_DRIVER_ST7565P_HH__

#include "Cosa/Pins.hh"
#include "Cosa/Board.hh"
#include "Cosa/IOStream.hh"
#include "Cosa/Canvas/Font.hh"
#include "Cosa/Canvas/Font/System5x7.hh"

/**
 * ST7565P 64x128 pixels matrix LCD controller/driver, device driver 
 * for IOStream access. Binding to trace, etc. Support natural text scroll,
 * cursor, and handling of special characters such as form-feed, back-
 * space and new-line. Graphics should be performed with OffScreen
 * Canvas and copied to the display with draw_bitmap().
 *
 * @section See Also
 * For further details see Sitronix 65x132 Dot Matrix LCD Controller/
 * Driver, Ver 1.3, 2004 May 18.
 */
class ST7565P : public IOStream::Device {
protected:
  /**
   * Instruction set (table 16, pp. 52)
   */
  enum {
    DISPLAY_OFF = 0xAE,		  // Turn display off
    DISPLAY_ON = 0xAF,		  // Turn display on
    SET_DISPLAY_START = 0x40,	  // Set start line address
    DISPLAY_START_MASK = 0x3f,	  // - line address mask
    SET_Y_ADDR = 0xB0,		  // Set page address
    Y_ADDR_MASK = 0x0f,		  // - page address mask
    SET_X_ADDR = 0x10,		  // Set column address (2x4 bits) 
    X_ADDR_MASK = 0x0f,		  // - colum address mask
    ADC_NORMAL = 0xA0,		  // Set normal address correspondence
    ADC_REVERSE = 0xA1,		  // Set reverse address correspondence
    DISPLAY_NORMAL = 0xA6,	  // Normal display mode
    DISPLAY_REVERSE = 0xA7,	  // Reverse display mode
    DISPLAY_64X128_POINTS = 0xA4, // Display normal
    DISPLAY_65X132_POINTS = 0xA5, // Display all points
    LCD_BIAS_9 = 0xA2,		  // Voltage ratio 1/9 bias
    LCD_BIAS_7 = 0xA3,		  // Voltage ratio 1/7 bias
    X_ADDR_INC = 0xE0,		  // Column address increment
    X_ADDR_CLEAR = 0xEE,	  // Clear read/modify/write
    INTERNAL_RESET = 0xE2,	  // Internal reset
    COM_OUTPUT_NORMAL = 0xC0,	  // Normal output scan direction
    COM_OUTPUT_REVERSE = 0xC8,	  // - reverse direction
    SET_POWER_CONTROL = 0x28,	  // Select internal power supply mode
    POWER_MASK = 0x07,		  // - operation mode mask
    SET_RESISTOR_RATIO = 0x20,	  // Select internal resistor ratio
    RESISTOR_MASK = 0x07,	  // - resistor ratio mask
    SET_CONSTRAST = 0x81,	  // Set output voltage volume register
    CONSTRAST_MASK = 0x3f,	  // - electronic volume mask
    INDICATOR_OFF = 0xAC,	  // Static indicator off
    INDICATOR_ON = 0xAD,	  // - on
    FLASHING_OFF = 0x00,	  // Set indicator flashing mode off
    FLASHING_ON = 0x01,		  // - on
    SET_BOOSTER_RATIO = 0xF8,	  // Set booster ratio
    BOOSTER_RATIO_234X = 0,	  // - 2x, 3x, 4x
    BOOSTER_RATIO_5X = 1,	  // - 5x
    BOOSTER_RATIO_6X = 3,	  // - 6x
    NOP = 0xE3,			  // Non-operation
    SCRIPT_PAUSE = 0xF0,	  // Init script pause (ms)
    SCRIPT_END = 0xFF		  // Init script end
  } __attribute__((packed));

  // Initialization script to reduce memory footprint
  static const uint8_t script[] PROGMEM;

  // Display pins and state
  OutputPin m_si;		  // Serial input
  OutputPin m_scl;		  // Serial clock input
  OutputPin m_dc;		  // Data(1) or command(0)
  OutputPin m_cs;		  // Chip select (active low)
  uint8_t m_x;			  // Cursor x (0..WIDTH-1)
  uint8_t m_y;			  // Cursor y (0..LINES-1)
  uint8_t m_line;		  // Display start line
  uint8_t m_mode;		  // Text mode (inverted)
  Font* m_font;			  // Font

  /**
   * Set display address for next data block.
   * @param[in] x position (0..WIDTH-1).
   * @param[in] y position (0..LINES-1).
   */
  void set(uint8_t x, uint8_t y);
  
  /**
   * Write given data to display according to mode.
   * Chip select and/or Command/Data pin asserted.
   * @param[in] data to fill write to device.
   */
  void write(uint8_t data)
  {
    m_si.write(data, m_scl);
  }

  /**
   * Fill display with given data.
   * @param[in] data to fill with.
   * @param[in] count number of bytes to fill.
   */
  void fill(uint8_t data, uint16_t count) 
  {
    inverted(m_cs) {
      for (uint16_t i = 0; i < count; i++) 
	write(data);
    }
  }

public:
  enum DisplayMode {
    NORMAL_DISPLAY_MODE,
    REVERSE_DISPLAY_MODE
  } __attribute__((packed));
  enum TextMode {
    NORMAL_TEXT_MODE = 0x00,
    INVERTED_TEXT_MODE = 0xff
  } __attribute__((packed));
  static const uint8_t WIDTH = 128;
  static const uint8_t HEIGHT = 64;
  static const uint8_t LINES = 8;

  /**
   * Construct display device driver with given pins and font.
   * Defaults are digital pin for Arduino/Tiny.
   * @param[in] si screen data pin (default D0/D6).
   * @param[in] scl screen clock pin (default D1/D7). 
   * @param[in] dc data/command control pin (default D2/D8).
   * @param[in] cs screen chip enable pin (default D3/D9).
   */
#if defined(__ARDUINO_TINY__)
  ST7565P(Board::DigitalPin si = Board::D0, 
	  Board::DigitalPin scl = Board::D1, 
	  Board::DigitalPin dc = Board::D2, 
	  Board::DigitalPin cs = Board::D3,
	  Font* font = &system5x7) :
#else
  ST7565P(Board::DigitalPin si = Board::D6, 
	  Board::DigitalPin scl = Board::D7, 
	  Board::DigitalPin dc = Board::D8, 
	  Board::DigitalPin cs = Board::D9,
	  Font* font = &system5x7) :
#endif
    IOStream::Device(),
    m_si(si, 0),
    m_scl(scl, 0),
    m_dc(dc, 1),
    m_cs(cs, 1),
    m_x(0),
    m_y(0),
    m_line(0),
    m_mode(0),
    m_font(font)
  {}

  /**
   * Start interaction with display.
   * @param[in] level contrast.
   * @return true(1) if successful otherwise false(0)
   */
  bool begin(uint8_t level = 0x08);

  /**
   * Stop sequence of interaction with device.
   * @return true(1) if successful otherwise false(0)
   */
  bool end();

  /**
   * Set display mode. 
   * @param[in] mode new display mode.
   */
  void set_display_mode(DisplayMode mode);

  /**
   * Set display contrast (0..63).
   * @param[in] contrast level.
   */
  void set_display_contrast(uint8_t level);

  /**
   * Get current cursor position.
   * @param[out] x pixel position (0..WIDTH-1).
   * @param[out] y line position (0..LINES-1).
   */
  void get_cursor(uint8_t& x, uint8_t& y)
  {
    x = m_x;
    y = m_y;
  }

  /**
   * Set cursor to given position.
   * @param[in] x pixel position (0..WIDTH-1).
   * @param[in] y line position (0..LINES-1).
   */
  void set_cursor(uint8_t x, uint8_t y);

  /**
   * Set text mode. Return previous text mode.
   * @param[in] mode new text mode.
   * @return previous text mode.
   */
  TextMode set_text_mode(TextMode mode)
  {
    TextMode previous = (TextMode) m_mode;
    m_mode = mode;
    return (previous);
  }

  /**
   * Get current text font. 
   * @return font setting.
   */
  Font* get_text_font()
  {
    return (m_font);
  }

  /**
   * Set text font. Returns previous setting.
   * @param[in] font.
   * @return previous font setting.
   */
  Font* set_text_font(Font* font)
  {
    Font* previous = m_font;
    m_font = font;
    return (previous);
  }

  /**
   * Draw icon in the current mode. The icon must be stored in program
   * memory with width, height and data.
   * @param[in] bp
   */
  void draw_icon(const uint8_t* bp);

  /**
   * Draw bitmap in the current mode. 
   * @param[in] bp
   */
  void draw_bitmap(uint8_t* bp, uint8_t width, uint8_t height);

  /**
   * Draw a bar at the current position with the given width.
   * The bar is filled from left to right proportional to the
   * given procent (0..100).
   * @param[in] procent filled from left to right.
   * @param[in] width of bar.
   * @param[in] pattern of filled section of bar.
   */
  void draw_bar(uint8_t procent, uint8_t width, uint8_t pattern = 0x55);

  /**
   * @override
   * Write character to display. Handles carriage-return-line-feed, back-
   * space and form-feed. Returns character or EOF on error.
   * @param[in] c character to write.
   * @return character written or EOF(-1).
   */
  virtual int putchar(char c);
};

#endif
