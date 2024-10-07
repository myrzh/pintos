#include "tests/threads/tests.h"
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads/malloc.h"

enum test_args_type
  {
    test_no_args,
    test_args_1uint,
    test_args_2uint,
    test_args_3uint,
    test_args_4uint
  };

struct test 
  {
    const char *name;
    test_func *function;
    enum test_args_type args_type;
  };

static const struct test tests[] = 
  {
    {"alarm-single", test_alarm_single},
    {"alarm-multiple", test_alarm_multiple},
    {"alarm-simultaneous", test_alarm_simultaneous},
    {"alarm-priority", test_alarm_priority},
    {"alarm-zero", test_alarm_zero},
    {"alarm-negative", test_alarm_negative},
    {"priority-change", test_priority_change},
    {"priority-donate-one", test_priority_donate_one},
    {"priority-donate-multiple", test_priority_donate_multiple},
    {"priority-donate-multiple2", test_priority_donate_multiple2},
    {"priority-donate-nest", test_priority_donate_nest},
    {"priority-donate-sema", test_priority_donate_sema},
    {"priority-donate-lower", test_priority_donate_lower},
    {"priority-donate-chain", test_priority_donate_chain},
    {"priority-fifo", test_priority_fifo},
    {"priority-preempt", test_priority_preempt},
    {"priority-sema", test_priority_sema},
    {"priority-condvar", test_priority_condvar},
    {"mlfqs-load-1", test_mlfqs_load_1},
    {"mlfqs-load-60", test_mlfqs_load_60},
    {"mlfqs-load-avg", test_mlfqs_load_avg},
    {"mlfqs-recent-1", test_mlfqs_recent_1},
    {"mlfqs-fair-2", test_mlfqs_fair_2},
    {"mlfqs-fair-20", test_mlfqs_fair_20},
    {"mlfqs-nice-2", test_mlfqs_nice_2},
    {"mlfqs-nice-10", test_mlfqs_nice_10},
    {"mlfqs-block", test_mlfqs_block},
    {"narrow-bridge", test_narrow_bridge, test_args_4uint},
  };

static const char *test_name;

void call_test(const struct test* t, const char* name_args);

/* Runs the test named NAME. */
void
run_test (const char *name) 
{
  const struct test *t;

  for (t = tests; t < tests + sizeof tests / sizeof *tests; t++)
  {
    // Tests without args
    if (!strcmp (name, t->name))
      {
        test_name = name;
        msg ("begin");
        call_test(t, name);
        msg ("end");
        return;
      }

    // Tests with args
    if (strstr(name, " ") && !strncmp(name, t->name, strstr(name, " ") - name ) )
    {
       size_t sz = strstr(name, " ") - name;
       char* nm = malloc(sz + 1);
       memcpy(nm, name, sz);
       nm[sz] = 0;

       test_name = nm;
       msg ("begin");
       call_test(t, name);
       msg ("end");

       free(nm);
       return;
    }
  }

  PANIC ("no test named \"%s\"", name);
}

/** Prints FORMAT as if with printf(),
   prefixing the output by the name of the test
   and following it with a new-line character. */
void
msg (const char *format, ...) 
{
  va_list args;
  
  printf ("(%s) ", test_name);
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
  putchar ('\n');
}

/** Prints failure message FORMAT as if with printf(),
   prefixing the output by the name of the test and FAIL:
   and following it with a new-line character,
   and then panics the kernel. */
void
fail (const char *format, ...) 
{
  va_list args;
  
  printf ("(%s) FAIL: ", test_name);
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
  putchar ('\n');

  PANIC ("test failed");
}

/** Prints a message indicating the current test passed. */
void
pass (void) 
{
  printf ("(%s) PASS\n", test_name);
}

static int check_args_count(unsigned int need, unsigned int cnt)
{
      if (cnt < need)
      {
          PANIC("invalid arguments count");
          return 0;
      }
      return 1;
}

void call_test(const struct test* t, const char* name_args)
{
   if (t->args_type == test_no_args)
   {
      test_func* p = (test_func*) t->function;
      p();
   }
   else if ((t->args_type >= test_args_1uint && t->args_type <= test_args_4uint) && strstr(name_args, " "))
   {
      const char* cargs = strstr(name_args, " ") + 1;
      char* params[16] = { 0 };
      char *token, *save_ptr, *args;
      unsigned int cnt = 0;
      size_t sz = sizeof(char) * (strlen(cargs) + 1);

      // Copy args, parsing it
      args = (char*) malloc(sz);
      memcpy(args, cargs, sz);

      for (token = strtok_r (args, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr))
      {
          if (cnt < sizeof(params) / sizeof(params[0]))
          {
              params[cnt] = token;
              cnt++;
          }
      }

      if (t->args_type == test_args_1uint)
      {
          test_func_args_1uint* p = (test_func_args_1uint *) t->function;
          check_args_count(1, cnt);
          p((unsigned int) atoi(params[0]));
      }
      else if (t->args_type == test_args_2uint)
      {
          test_func_args_2uint* p = (test_func_args_2uint *) t->function;
          check_args_count(2, cnt);
          p((unsigned int) atoi(params[0]), (unsigned int) atoi(params[1]) );
      }
      else if (t->args_type == test_args_3uint)
      {
          test_func_args_3uint* p = (test_func_args_3uint *) t->function;
          check_args_count(3, cnt);
          p((unsigned int) atoi(params[0]), (unsigned int) atoi(params[1]), (unsigned int) atoi(params[2]) );
      }
      else if (t->args_type == test_args_4uint)
      {
          test_func_args_4uint* p = (test_func_args_4uint *) t->function;
          check_args_count(3, cnt);
          p((unsigned int) atoi(params[0]), (unsigned int) atoi(params[1]), (unsigned int) atoi(params[2]), (unsigned int) atoi(params[3]) );          
      }

      // Free args copy
      free(args);
   }
   else
   {
       PANIC("invalid arguments");
   }
}
