#include "quantum.h"
#include "transactions.h"

oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    return OLED_ROTATION_180;
}

#define CRAB_LINE 4

#define FRAME_LINES 4
#define FRAMES 6
const char frame_0_line_0[] PROGMEM = "_~^~^~_";
const char frame_0_line_1[] PROGMEM = "\\) /  o o  \\ (/";
const char frame_0_line_2[] PROGMEM = "'_   ~   _'";
const char frame_0_line_3[] PROGMEM = "\\ '-----' /";
const char *const frame_table_0[FRAME_LINES] = { frame_0_line_0, frame_0_line_1, frame_0_line_2, frame_0_line_3 };
const char frame_1_line_0[] PROGMEM = "_~^~^~_";
const char frame_1_line_1[] PROGMEM = "\\) /  o o  \\ (/";
const char frame_1_line_2[] PROGMEM = "'-,   -  _'\\";
const char frame_1_line_3[] PROGMEM = "| '----' ";
const char *const frame_table_1[FRAME_LINES] = { frame_1_line_0, frame_1_line_1, frame_1_line_2, frame_1_line_3 };
const char frame_2_line_0[] PROGMEM = ".~'^'^-, (/";
const char frame_2_line_1[] PROGMEM = "\\) /  o O  |'";
const char frame_2_line_2[] PROGMEM = "'-,   -  _'\\";
const char frame_2_line_3[] PROGMEM = "| '----' ";
const char *const frame_table_2[FRAME_LINES] = { frame_2_line_0, frame_2_line_1, frame_2_line_2, frame_2_line_3 };
const char frame_3_line_0[] PROGMEM = ".~'^'^-, (/";
const char frame_3_line_1[] PROGMEM = "\\) /  o O  |'";
const char frame_3_line_2[] PROGMEM = "'-,   -  _'\\";
const char frame_3_line_3[] PROGMEM = "| '----' ";
const char *const frame_table_3[FRAME_LINES] = { frame_3_line_0, frame_3_line_1, frame_3_line_2, frame_3_line_3 };
const char frame_4_line_0[] PROGMEM = "_~^~^~_";
const char frame_4_line_1[] PROGMEM = "\\) /  o o  \\ (/";
const char frame_4_line_2[] PROGMEM = "'-,   -  _'\\";
const char frame_4_line_3[] PROGMEM = "| '----' ";
const char *const frame_table_4[FRAME_LINES] = { frame_4_line_0, frame_4_line_1, frame_4_line_2, frame_4_line_3 };
const char frame_5_line_0[] PROGMEM = "_~^~^~_";
const char frame_5_line_1[] PROGMEM = "\\) /  o o  \\ (/";
const char frame_5_line_2[] PROGMEM = "'_   ~   _'";
const char frame_5_line_3[] PROGMEM = "/ '-----' \\";
const char *const frame_table_5[FRAME_LINES] = { frame_5_line_0, frame_5_line_1, frame_5_line_2, frame_5_line_3 };
const char *const const* frames[FRAMES] = { frame_table_0, frame_table_1, frame_table_2, frame_table_3, frame_table_4, frame_table_5 };
const uint32_t frames_pad[FRAMES] = { 33685508, 33619972, 33619972, 33619972, 33619972, 50528517 };

const char slash_legs[] PROGMEM = "/ '-----' \\";
const char pipe_legs[] PROGMEM = "| '-----' |";

enum State {
  PRIMARY_FORWARD,
  PRIMARY_BACK,
  SECONDARY_FORWARD,
  SECONDARY_BACK,
};

typedef struct _animation_t {
  uint8_t frame;
} animation_t;

#define MAX_CHAR_WIDTH 21

void write_max(const char *data, uint8_t skip, uint8_t pad) {
  uint8_t remaining = MAX_CHAR_WIDTH;

  if (skip > pad) {
    // consume some skip with our pad
    skip -= pad;
    // and dont pad
    pad = 0;
  } else {
    // else skip is lte our pad, so lets just pad less
    pad -= skip;
    // and not skip
    skip = 0;
  }

  while (pad-- && remaining) {
    oled_write_char(' ', false);
    remaining--;
  }

  uint8_t c = pgm_read_byte(data);
  while (c != 0 && remaining) {
      if (skip) {
        c = pgm_read_byte(++data);
        skip--;
      } else {
        oled_write_char(c, false);
        c = pgm_read_byte(++data);
        remaining--;
      }
  }

  oled_advance_page(true);
}

void render_frame(uint8_t index) {
  // Number of extra spaces to pad
  uint8_t extra = 0;

  uint8_t i = index;

  if (i >= FRAMES) {
    extra = i - FRAMES;
    // Use the final frame as the template
    i = FRAMES - 1;
  }

  uint8_t skip = 0;

  // If we are the right side
  if (!is_keyboard_master()) {
    // The width of the crab
    skip = 15;
  }

  for (int j = 0; j < FRAME_LINES; j++) {
    oled_set_cursor(0, CRAB_LINE + j);
    uint8_t pad = extra + ((frames_pad[i] >> (j * 8)) & 0xFF);

    // If its the legs, do something different when its not the "intro"
    if (extra && j == (FRAME_LINES - 1)) {
      if (index % 2 == 0) {
        write_max(pipe_legs, skip, pad);
      } else {
        write_max(slash_legs, skip, pad);
      }
    } else {
      write_max(frames[i][j], skip, pad);
    }
  }
}

enum State state = PRIMARY_FORWARD;

uint8_t move = 0;
int8_t frame_index = 0;

void render_next_frame(void) {
  animation_t msg = {frame_index};

  switch (state) {
    case PRIMARY_FORWARD:
      render_frame(frame_index);
      if (++frame_index >= 27) {
        frame_index = FRAMES;
        state = SECONDARY_FORWARD;
      }
      break;
    case SECONDARY_FORWARD:
      if (!transaction_rpc_send(USER_SYNC_A, sizeof(animation_t), &msg)) {
        transaction_rpc_send(USER_SYNC_A, sizeof(animation_t), &msg);
      }

      if (++frame_index >= 27) {
        state = SECONDARY_BACK;
      }
      break;
    case SECONDARY_BACK:
      if (!transaction_rpc_send(USER_SYNC_A, sizeof(animation_t), &msg)) {
        transaction_rpc_send(USER_SYNC_A, sizeof(animation_t), &msg);
      }

      if (--frame_index <= 0) {
        frame_index = 27;
        state = PRIMARY_BACK;
      }

      break;
    case PRIMARY_BACK:
      render_frame(frame_index);
      if (--frame_index <= 0) {
        frame_index = 0;
        state = PRIMARY_FORWARD;
      }
      break;
  }
}

bool oled_task_user(void) {
    if (is_keyboard_master()) {
      if (move) {
        render_next_frame();
        move--;
      }
    }
    return false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    move++;
    return true;
};

void secondary_handler(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
  if (!is_oled_on()) {
    oled_on();
  }
  const animation_t *msg = (const animation_t*)in_data;
  render_frame(msg->frame);
}

void keyboard_post_init_user(void) {
  if (!is_keyboard_master()) {
      transaction_register_rpc(USER_SYNC_A, secondary_handler);
  }
}

