#pragma once

#if __linux__
namespace raw_keyboard_device {

}
struct termios orig_termios;

/**
 * @fn disable_raw_mode
 * @brief disables raw mode preventing character echo within the terminal when a
 * key is pressed. See:
 * https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
 */
void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

/**
 * @enum raw_mode_t
 * @brief
 *
 * The ability to exit cleanly in both of these
 * circumstance also exists. Or you may choose to implement another
 * interface for emergency system exit.
 *
 */
enum class raw_mode_t {
  /**
   * @brief The keyboard buffer is read one character at a time. When any key on
   * the keyboard is pressed, the function immediately returns. It does this
   * one at a time. If keyboard keys produce multiple scan codes such as an
   * escape key for recognition, the routine must be called again to gather the
   * rest. The characters typed are not displayed within the output window.
   */
  immediate_no_echo,

  /* @brief This is for when the program works flawlessly. In addition to the
   * keyboard processing above, signaling of special OS keyboard combinations
   * tied to standard keys will be disabled. That is no pressing CTRL c, CTRL Z,
   * etc. to close the program at the prompt. This is most likely a preference
   * for your text application.
   *
   * Turn off CTRL-C and CTRL-Z signals
   * Disable CTRL-S and CTRL-Q
   * Disable CTRL-V
   * Fix CTRL-M
   * Turn off all output processing
   *
   */
  immediate_no_echo_ignore_signals
};

/**
 * @fn enable_raw_mode
 * @brief enables raw mode to prevent character echo within the terminal.
 * See: https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
 */
void enable_raw_mode(raw_mode_t mode) {

  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw_mode);

  struct termios raw = orig_termios;
  switch (mode) {
  case raw_mode_t::immediate_no_echo:
    /**
     * @brief
     * no echo - return immediately
     * Turn off canonical mode - immediate character return
     */

    raw.c_lflag &= ~(ECHO | ICANON);
    break;

  case raw_mode_t::immediate_no_echo_ignore_signals:
    /**
     * Turn off Ctrl-C and Ctrl-Z signals
     * Disable Ctrl-S and Ctrl-Q
     * Disable Ctrl-V
     * Fix Ctrl-M
     * Turn off all output processing
     *
     */
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    break;
  }

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
#endif

/**
 * @enum vkey_t
 * @brief the virtual keycode. The system translates the input from the
 * STDIN_FILENO low level file to these values. There are two separate events
 * within the event class. A character and a virtual key. Programming the
 * virtual key behavior can be provided as a distinct function within the
 * editing objects.
 */
enum class vkey_t : u_int8_t {
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  HOME,
  UP_ARROW,
  PAGE_UP,
  LEFT_ARROW,
  RIGHT_ARROW,
  END,
  DOWN_ARROW,
  PAGE_DOWN,
  INSERT,
  DELETE,
  PRINT_SCREEN,
  PAUSE_BREAK,
  BACKSPACE,
  ENTER,
  TAB
};

/**
 * @fn get_console_size
 * @brief gets the size of the console text window in text rows
 * and columns in monospace font character units.
 * See:
 *  https://stackoverflow.com/questions/23369503/get-size-of-terminal-window-rows-columns
 *   also contains windows information
 *   - Microsoft GetConsoleScreenBufferInfo()
 */
void get_console_size(u_int16_t &rows, u_int16_t &columns) {
#if __linux__

  struct winsize size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
  rows = size.ws_row;
  columns = size.ws_col;

#elif _WIN32 || _WIN64

  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;

#endif
}

char read_raw(u_int8_t ms_wait_return) {}
}
