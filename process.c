/*-
 * Copyright 2010-2012 Chris Spiegel.
 *
 * This file is part of Bocfel.
 *
 * Bocfel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version
 * 2 or 3, as published by the Free Software Foundation.
 *
 * Bocfel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bocfel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

#ifdef ZTERP_GLK
#include <glk.h>
#endif

#include "process.h"
#include "branch.h"
#include "dict.h"
#include "math.h"
#include "memory.h"
#include "objects.h"
#include "random.h"
#include "screen.h"
#include "stack.h"
#include "util.h"
#include "zoom.h"
#include "zterp.h"

uint16_t zargs[8];
int znargs;

static jmp_buf *jumps;
static size_t njumps;

/* Each time an interrupt happens, process_instructions() is called
 * (effectively starting a whole new round of interpreting).  This
 * variable holds the current level of interpreting: 0 for no
 * interrupts, 1 if one interrupt has been called, 2 if an interrupt was
 * called inside of an interrupt, and so on.
 */
static long ilevel = -1;

long interrupt_level(void)
{
  return ilevel;
}

/* When this is called, the interrupt at level “level” will stop
 * running: if a single interrupt is running, then break_from(1) will
 * stop the interrupt, going back to the main program.  Breaking from
 * interrupt level 0 (which is not actually an interrupt) will end the
 * program.  This is how @quit is implemented.
 */
void break_from(long level)
{
  ilevel = level - 1;
  longjmp(jumps[level], 1);
}

/* To signal a restart, longjmp() is called with 2; this advises
 * process_instructions() to restart the story file and then continue
 * execution, whereas a value of 1 tells it to return immediately.
 */
static void zrestart(void)
{
  ilevel = 0;
  longjmp(jumps[0], 2);
}

/* If a restore happens inside of an interrupt, the level needs to be
 * set back to 0; to differentiate from a restart, call longjmp() with a
 * value of 3.
 */
void reset_level(void)
{
  ilevel = 0;
  longjmp(jumps[0], 3);
}

/* Returns 1 if decoded, 0 otherwise (omitted) */
static int decode_base(uint8_t type, uint16_t *loc)
{
  switch(type)
  {
    case 0: /* Large constant. */
      *loc = WORD(pc);
      pc += 2;
      break;
    case 1: /* Small constant. */
      *loc = BYTE(pc++);
      break;
    case 2: /* Variable. */
      *loc = variable(BYTE(pc++));
      break;
    default: /* Omitted. */
      return 0;
  }

  return 1;
}

static void decode_var(uint8_t types)
{
  uint16_t ret;

  for(int i = 6; i >= 0; i -= 2)
  {
    if(!decode_base((types >> i) & 0x03, &ret)) return;
    zargs[znargs++] = ret;
  }
}

/* op[0] is 0OP, op[1] is 1OP, etc */
static void (*op[5][256])(void);
static const char *opnames[5][256];
enum { ZERO, ONE, TWO, VAR, EXT };

#define op_call(opt, opnumber)	(op[opt][opnumber]())

/* This nifty trick is from Frotz. */
static void zextended(void)
{
  uint8_t opnumber = BYTE(pc++);

  decode_var(BYTE(pc++));

  /* §14.2.1
   * The exception for 0x80–0x83 is for the Zoom extensions.
   * Standard 1.1 implicitly updates §14.2.1 to recommend ignoring
   * opcodes in the range EXT:30 to EXT:255, due to the fact that
   * @buffer_screen is EXT:29.
   */
  if(opnumber > 0x1d && (opnumber < 0x80 || opnumber > 0x83)) return;

  op_call(EXT, opnumber);
}

static void illegal_opcode(void)
{
#ifndef ZTERP_NO_SAFETY_CHECKS
  die("illegal opcode (pc = 0x%lx)", zassert_pc);
#else
  die("illegal opcode");
#endif
}

void setup_opcodes(void)
{
  for(int i = 0; i < 5; i++)
  {
    for(int j = 0; j < 256; j++)
    {
      op[i][j] = illegal_opcode;
    }
  }
#define OP(minver, maxver, args, opcode, fn) do { if(zversion >= minver && zversion <= maxver) op[args][opcode] = fn; opnames[args][opcode] = #fn; } while(0)
  OP(1, 6, ZERO, 0x00, zrtrue);
  OP(1, 6, ZERO, 0x01, zrfalse);
  OP(1, 6, ZERO, 0x02, zprint);
  OP(1, 6, ZERO, 0x03, zprint_ret);
  OP(1, 6, ZERO, 0x04, znop);
  OP(1, 4, ZERO, 0x05, zsave);
  OP(1, 4, ZERO, 0x06, zrestore);
  OP(1, 6, ZERO, 0x07, zrestart);
  OP(1, 6, ZERO, 0x08, zret_popped);
  OP(1, 4, ZERO, 0x09, zpop);
  OP(5, 6, ZERO, 0x09, zcatch);
  OP(1, 6, ZERO, 0x0a, zquit);
  OP(1, 6, ZERO, 0x0b, znew_line);
  OP(3, 3, ZERO, 0x0c, zshow_status);
  OP(4, 6, ZERO, 0x0c, znop); /* §15: Technically illegal in V4+, but a V5 Wishbringer accidentally uses this opcode. */
  OP(3, 6, ZERO, 0x0d, zverify);
  OP(5, 6, ZERO, 0x0e, zextended);
  OP(5, 6, ZERO, 0x0f, zpiracy);

  OP(1, 6, ONE, 0x00, zjz);
  OP(1, 6, ONE, 0x01, zget_sibling);
  OP(1, 6, ONE, 0x02, zget_child);
  OP(1, 6, ONE, 0x03, zget_parent);
  OP(1, 6, ONE, 0x04, zget_prop_len);
  OP(1, 6, ONE, 0x05, zinc);
  OP(1, 6, ONE, 0x06, zdec);
  OP(1, 6, ONE, 0x07, zprint_addr);
  OP(4, 6, ONE, 0x08, zcall_1s);
  OP(1, 6, ONE, 0x09, zremove_obj);
  OP(1, 6, ONE, 0x0a, zprint_obj);
  OP(1, 6, ONE, 0x0b, zret);
  OP(1, 6, ONE, 0x0c, zjump);
  OP(1, 6, ONE, 0x0d, zprint_paddr);
  OP(1, 6, ONE, 0x0e, zload);
  OP(1, 4, ONE, 0x0f, znot);
  OP(5, 6, ONE, 0x0f, zcall_1n);

  OP(1, 6, TWO, 0x01, zje);
  OP(1, 6, TWO, 0x02, zjl);
  OP(1, 6, TWO, 0x03, zjg);
  OP(1, 6, TWO, 0x04, zdec_chk);
  OP(1, 6, TWO, 0x05, zinc_chk);
  OP(1, 6, TWO, 0x06, zjin);
  OP(1, 6, TWO, 0x07, ztest);
  OP(1, 6, TWO, 0x08, zor);
  OP(1, 6, TWO, 0x09, zand);
  OP(1, 6, TWO, 0x0a, ztest_attr);
  OP(1, 6, TWO, 0x0b, zset_attr);
  OP(1, 6, TWO, 0x0c, zclear_attr);
  OP(1, 6, TWO, 0x0d, zstore);
  OP(1, 6, TWO, 0x0e, zinsert_obj);
  OP(1, 6, TWO, 0x0f, zloadw);
  OP(1, 6, TWO, 0x10, zloadb);
  OP(1, 6, TWO, 0x11, zget_prop);
  OP(1, 6, TWO, 0x12, zget_prop_addr);
  OP(1, 6, TWO, 0x13, zget_next_prop);
  OP(1, 6, TWO, 0x14, zadd);
  OP(1, 6, TWO, 0x15, zsub);
  OP(1, 6, TWO, 0x16, zmul);
  OP(1, 6, TWO, 0x17, zdiv);
  OP(1, 6, TWO, 0x18, zmod);
  OP(4, 6, TWO, 0x19, zcall_2s);
  OP(5, 6, TWO, 0x1a, zcall_2n);
  OP(5, 6, TWO, 0x1b, zset_colour);
  OP(5, 6, TWO, 0x1c, zthrow);

  OP(1, 6, VAR, 0x00, zcall);
  OP(1, 6, VAR, 0x01, zstorew);
  OP(1, 6, VAR, 0x02, zstoreb);
  OP(1, 6, VAR, 0x03, zput_prop);
  OP(1, 6, VAR, 0x04, zread);
  OP(1, 6, VAR, 0x05, zprint_char);
  OP(1, 6, VAR, 0x06, zprint_num);
  OP(1, 6, VAR, 0x07, zrandom);
  OP(1, 6, VAR, 0x08, zpush);
  OP(1, 6, VAR, 0x09, zpull);
  OP(3, 6, VAR, 0x0a, zsplit_window);
  OP(3, 6, VAR, 0x0b, zset_window);
  OP(4, 6, VAR, 0x0c, zcall_vs2);
  OP(4, 6, VAR, 0x0d, zerase_window);
  OP(4, 6, VAR, 0x0e, zerase_line);
  OP(4, 6, VAR, 0x0f, zset_cursor);
  OP(4, 6, VAR, 0x10, zget_cursor);
  OP(4, 6, VAR, 0x11, zset_text_style);
  OP(4, 6, VAR, 0x12, znop); /* XXX buffer_mode */
  OP(3, 6, VAR, 0x13, zoutput_stream);
  OP(3, 6, VAR, 0x14, zinput_stream);
  OP(3, 6, VAR, 0x15, zsound_effect);
  OP(4, 6, VAR, 0x16, zread_char);
  OP(4, 6, VAR, 0x17, zscan_table);
  OP(5, 6, VAR, 0x18, znot);
  OP(5, 6, VAR, 0x19, zcall_vn);
  OP(5, 6, VAR, 0x1a, zcall_vn2);
  OP(5, 6, VAR, 0x1b, ztokenise);
  OP(5, 6, VAR, 0x1c, zencode_text);
  OP(5, 6, VAR, 0x1d, zcopy_table);
  OP(5, 6, VAR, 0x1e, zprint_table);
  OP(5, 6, VAR, 0x1f, zcheck_arg_count);

  OP(5, 6, EXT, 0x00, zsave5);
  OP(5, 6, EXT, 0x01, zrestore5);
  OP(5, 6, EXT, 0x02, zlog_shift);
  OP(5, 6, EXT, 0x03, zart_shift);
  OP(5, 6, EXT, 0x04, zset_font);
  OP(6, 6, EXT, 0x05, znop); /* XXX draw_picture */
  OP(6, 6, EXT, 0x06, zpicture_data);
  OP(6, 6, EXT, 0x07, znop); /* XXX erase_picture */
  OP(6, 6, EXT, 0x08, znop); /* XXX set_margins */
  OP(5, 6, EXT, 0x09, zsave_undo);
  OP(5, 6, EXT, 0x0a, zrestore_undo);
  OP(5, 6, EXT, 0x0b, zprint_unicode);
  OP(5, 6, EXT, 0x0c, zcheck_unicode);
  OP(5, 6, EXT, 0x0d, zset_true_colour);
  OP(6, 6, EXT, 0x10, znop); /* XXX move_window */
  OP(6, 6, EXT, 0x11, znop); /* XXX window_size */
  OP(6, 6, EXT, 0x12, znop); /* XXX window_style */
  OP(6, 6, EXT, 0x13, zget_wind_prop);
  OP(6, 6, EXT, 0x14, znop); /* XXX scroll_window */
  OP(6, 6, EXT, 0x15, zpop_stack);
  OP(6, 6, EXT, 0x16, znop); /* XXX read_mouse */
  OP(6, 6, EXT, 0x17, znop); /* XXX mouse_window */
  OP(6, 6, EXT, 0x18, zpush_stack);
  OP(6, 6, EXT, 0x19, znop); /* XXX put_wind_prop */
  OP(6, 6, EXT, 0x1a, zprint_form);
  OP(6, 6, EXT, 0x1b, zmake_menu);
  OP(6, 6, EXT, 0x1c, znop); /* XXX picture_table */
  OP(6, 6, EXT, 0x1d, zbuffer_screen);

  /* Zoom extensions. */
  OP(5, 6, EXT, 0x80, zstart_timer);
  OP(5, 6, EXT, 0x81, zstop_timer);
  OP(5, 6, EXT, 0x82, zread_timer);
  OP(5, 6, EXT, 0x83, zprint_timer);
#undef OP
}

void process_instructions(void)
{
  if(njumps <= ++ilevel)
  {
    jumps = realloc(jumps, ++njumps * sizeof *jumps);
    if(jumps == NULL) die("unable to allocate memory for jump buffer");
  }

  switch(setjmp(jumps[ilevel]))
  {
    case 1: /* Normal break from interrupt. */
      return;
    case 2: /* Special break: a restart was requested. */
      {
        /* §6.1.3: Flags2 is preserved on a restart. */
        uint16_t flags2 = WORD(0x10);

        process_story();

        STORE_WORD(0x10, flags2);
      }
      break;
    case 3: /* Do nothing: restore was called so keep interpreting. */
      break;
  }

  while(1)
  {
    uint8_t opcode;

#if defined(ZTERP_GLK) && defined(ZTERP_GLK_TICK)
    glk_tick();
#endif

    ZPC(pc);

    opcode = BYTE(pc++);

    /* long 2OP */
    if(opcode < 0x80)
    {
      znargs = 2;

      if(opcode & 0x40) zargs[0] = variable(BYTE(pc++));
      else              zargs[0] = BYTE(pc++);

      if(opcode & 0x20) zargs[1] = variable(BYTE(pc++));
      else              zargs[1] = BYTE(pc++);

      op_call(TWO, opcode & 0x1f);
    }

    /* short 1OP */
    else if(opcode < 0xb0)
    {
      znargs = 1;

      if(opcode < 0x90) /* large constant */
      {
        zargs[0] = WORD(pc);
        pc += 2;
      }
      else if(opcode < 0xa0) /* small constant */
      {
        zargs[0] = BYTE(pc++);
      }
      else /* variable */
      {
        zargs[0] = variable(BYTE(pc++));
      }

      op_call(ONE, opcode & 0x0f);
    }

    /* short 0OP (plus EXT) */
    else if(opcode < 0xc0)
    {
      znargs = 0;

      op_call(ZERO, opcode & 0x0f);
    }

    /* variable 2OP */
    else if(opcode < 0xe0)
    {
      znargs = 0;

      decode_var(BYTE(pc++));

      op_call(TWO, opcode & 0x1f);
    }

    /* Double variable VAR */
    else if(opcode == 0xec || opcode == 0xfa)
    {
      uint8_t types1, types2;

      znargs = 0;

      types1 = BYTE(pc++);
      types2 = BYTE(pc++);
      decode_var(types1);
      decode_var(types2);

      op_call(VAR, opcode & 0x1f);
    }

    /* variable VAR */
    else
    {
      znargs = 0;

      read_pc = pc - 1;

      decode_var(BYTE(pc++));

      op_call(VAR, opcode & 0x1f);
    }
  }
}
