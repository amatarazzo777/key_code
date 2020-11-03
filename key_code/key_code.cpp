#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <unordered_map>
#include <iostream>

using namespace std;

#if __linux__

struct termios orig_termios = {};
bool bset_exit = {};
int keyboard_state = {};

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

/*
This directory is for system-local terminfo descriptions. By default,
ncurses will search ${HOME}/.terminfo first, then /etc/terminfo (this
directory), then /lib/terminfo, and last not least /usr/share/terminfo.
*/

/**
 * @fn enable_raw_mode
 * @brief enables raw mode to prevent character echo within the terminal.
 * This function provides several discreet functionalities by parameter
 * settings.
 * @raw_mode_t mode - this is usually a compile setting the implementor would
 * change. Mode for raw with or without signal capture of ui enhancements and
 * other emergency program interruptions from the terminal.
 *
 * A use case might be to keep information from being copied via the CTRL C.
 * Although this is by no means security for an interface as there may be other
 * means.
 *
 * See:
 * https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
 */
void enable_raw_mode(bool wait_for_input = true,
                     raw_mode_t mode = raw_mode_t::immediate_no_echo) {

  if (!bset_exit) {
    atexit(disable_raw_mode);
    tcgetattr(STDIN_FILENO, &orig_termios);
    printf("orig_termios(%x %x)\n", (int)orig_termios.c_cc[VMIN],
           (int)orig_termios.c_cc[VTIME]);
    bset_exit = true;
  }

  struct termios raw = {};
  tcgetattr(STDIN_FILENO, &raw);

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
     * Legacy flags as per
     * https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
     */
#if 0
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);



    termios-p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                                  |INLCR|IGNCR|ICRNL|IXON);
    termios-p->c_oflag &= ~OPOST;
    termios-p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    termios-p->c_cflag &= ~(CSIZE|PARENB);
    termios-p->c_cflag |= CS8;
#endif

    cfmakeraw(&raw);
    break;
  }

  // amount of characters that must be received.
  if (wait_for_input) {
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
  } else {
    raw.c_cc[VMIN] = 0;
    // tenth of second poll time. or wait exit time.
    raw.c_cc[VTIME] = 1;
  }

  // TCSANOW is used to keep keys in buffer there for reading.
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
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
  none,
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
  END,
  UP_ARROW,
  DOWN_ARROW,
  LEFT_ARROW,
  RIGHT_ARROW,
  PAGE_UP, // 19
  PAGE_DOWN,
  INSERT,
  DELETE,
  ESC, // 23
  BACKSPACE,
  ENTER, // 25
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

/**
 * @fn get_keyboard_state
 * @@brief gets the state of the caps lock, num lock and insert mode used during
 * editing.
 */
void get_keyboard_state() {
  int fd = open("/dev/tty0, O_NOCTTY");
  if (fd == -1)
    throw std::runtime_error("Error cannot open /dev/tty0");
}

/**
 * @fn read_raw
 * @param u_int8_t ms_wait_return - amount of time to wait for query.  *
 * @brief Default is that it waits indefinitely for a key to be pressed.
 * If a numeric is supplied within the parameter, the unsigned integer notes the
 * number of
 */
std::size_t read_raw(char *ptr, bool bwait_for_key = true,
                     std::size_t ptr_size = 1) {
  enable_raw_mode(bwait_for_key);
  std::size_t ret = read(STDIN_FILENO, ptr, ptr_size);
  return ret;
}

int main() {
  u_int16_t rows = {};
  u_int16_t columns = {};

  // get the size of the text window
  get_console_size(rows, columns);

  printf("text(%d %d)\n", rows, columns);
  for (auto i = 0; i < columns - 1; i++)
    printf("%c", (i % 10 + '0'));
  printf("*\n");

  std::unordered_map<std::string, vkey_t> virtual_key_map = {
      {"\x1b", vkey_t::ESC},          {"\x1b[OQ", vkey_t::F2},
      {"\x1b[OR", vkey_t::F3},        {"\x1b[OS", vkey_t::F4},
      {"\x1b[15~", vkey_t::F5},       {"\x1b[17~", vkey_t::F6},
      {"\x1b[18~", vkey_t::F7},       {"\x1b[19~", vkey_t::F8},
      {"\x1b[20~", vkey_t::F9},       {"\x1b[H", vkey_t::HOME},
      {"\x1b[F", vkey_t::END},        {"\x1b[A", vkey_t::UP_ARROW},
      {"\x1b[B", vkey_t::DOWN_ARROW}, {"\x1b[C", vkey_t::RIGHT_ARROW},
      {"\x1b[D", vkey_t::LEFT_ARROW}, {"\x1b[5~", vkey_t::PAGE_UP},
      {"\x1b[6~", vkey_t::PAGE_DOWN}, {"\x1b[2~", vkey_t::INSERT},
      {"\x1b[3~", vkey_t::DELETE},    {"\x7f", vkey_t::BACKSPACE},
      {"\x0a", vkey_t::ENTER},        {"\x09", vkey_t::TAB}};

#if 0

  char c = {};
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (iscntrl(c)) {
      printf("%d - 0x%x\n", c, c);
    } else {
      printf("%d - 0x%x ('%c')\n", c, c, c);
    }
  }
#endif

  /*
   * @brief  if a control escape sequence has been received, process the rest of
   * the messages from the keyboard. Read the entire buffer. Once it is
   * interpreted, dispatch the virtual key by the enumeration and discard the
   * raw input. If the input is character information it is dispatched.
   *
   */

  char c = {};
  vkey_t vk = {};

  // this loop will received control messages another way.
  while (read_raw(&c) == 1 && c != 'q') {
    std::string key_sequence = {};

    key_sequence.push_back(c);

    /**
     * @brief  if its an escape code, detection of the actual ESC key is
     * performed by reading the keyboard again with a minimal wait period very
     * low. The read function will return without actually having a
     * character. When it does not have a character at this point, it is a key
     * press from the ESC key. A user input and not an escaped virtual key.
     */
    if (c == '\x1b') {
      char immediate_next = {};
      std::size_t rdret = read_raw(&immediate_next, false);
      printf("%lu)\n", rdret);
      if (rdret == 1) {
        key_sequence.push_back(immediate_next);
        printf("%lu->", rdret);
        /** read the rest of the sequence into the remainder of the signature.
         * the ESC character occupies the first postion. The immediate_next
         * variable occupies the second. The read is therefore started from the
         * next insert position. after this completes, a keyboard signature will
         * most likely be there. This is used as a string to the virtual key map
         * for identification and then dispatch.*/
        char buffer[11] = {};

        read_raw(buffer, false, 10); //-1 room for strcat null.
        key_sequence.append(buffer);
      }
    }

    /** @brief filter the key through the keyboard map. If the item is within
     * the unordered_map then it will produce a vk value from the corresponding
     * entry. Of note, both  multiple escaped sequence character keystrokes and
     * single character keystrokes are processed using this filter. There are a
     * few single character ones that are also labeled as virtual key. ENTER,
     * TAB, BACKSPACE, etc. for preference of style and handling the filter in
     * one place. Common for such filters. */
    auto it = virtual_key_map.find(key_sequence);
    if (it != virtual_key_map.end()) {
      vk = it->second;
      printf("key seq - ");
      for (auto ch : key_sequence) {
        printf(" 0x%x ", (int)ch);
      }
      printf("\n");
      key_sequence = {};

    } else {
      vk = vkey_t::none;
    }

    /* @brief here is where the change of in dispatch and other searching may
     * produce results for listeners. The filter has produced results into two
     * distinct variables: vk or c. When one is set, the other is turned off. A
     * type of variant, but really small data. Both are areas of storage for
     * convenience. A "vk" or the "key_sequence" has the keystroke
     * information.*/
    if (vk != vkey_t::none)
      printf("vk        input - %hu\n", static_cast<u_int16_t>(vk));
    else
      for (auto ch : key_sequence) {
        printf("character input - %c\n", ch);
      }
  }

  // exiting without disabling raw mod causes no input to show.
  // so it disables it here.
  disable_raw_mode();

  return EXIT_SUCCESS;
}
