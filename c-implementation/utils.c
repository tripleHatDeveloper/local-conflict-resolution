#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "public-types.h"
#include "dp.h"
#include "utils.h"
#include "opts.h"

/**
 * load correspondance file (variable names
 */

char **
load_names (char *file, type_instance kb)
{
  char **names = NEW_ARRAY (char *, CLAUSE_NUMBER(kb));
  FILE *f;
  int i;
#ifdef _GNU_SOURCE
  char *buff;
#else
  char buff[255];
#endif

  assert ( names != NULL );

  if ( (f = fopen (file, "r")) == NULL )
    {
      fprintf (stderr, "unable to open file %s\n", file);
      exit (1);
    }

#ifdef _GNU_SOURCE 
  while ( fscanf (f, "%ms %d\n", &buff, &i) == 2 )
    {
      names[i] = buff;
      assert ( names[i] != NULL );
    }
#else
  while ( fscanf (f, "%s %d\n", buff, &i) == 2 )
    {
      names[i] = strdup (buff);
      assert ( names[i] != NULL );
    }
#endif
  
  fclose (f);
  return names;
}


void 
error (int code, char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);

  fprintf (stderr, "kernels: ");
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  
  va_end (ap);

  if (code == 255) 
    {
      fprintf (stderr, "ESPACE computes removed sets of two conflicting sets of clauses\n\n");
      fprintf (stderr, "Usage: espace [-mavtb] zeta-file\n\n");
      fprintf (stderr, "Options:\n");
      fprintf (stderr, "  -m\tcompute minimal cardinality sets\n");
      fprintf (stderr, "  -a\tdo not limit computation to E1 set elements\n");
      fprintf (stderr, "  -v\tverbose mode\n");
      fprintf (stderr, "  -t\ttiming mesurements\n");
      fprintf (stderr, "  -b\t\"brute force\" mode : do not take different sets into account\n\n");
      fprintf (stderr, "For any questions : <omar.doukari@lsis.org>");
    }

  exit (code);
}
  
/**
 * prints a message if verbose mode is active
 */
void
vmsg (char *fmt, ...)
{
  va_list ap;
  if (Opt_Verb)
    {
      va_start(ap, fmt);
      vprintf(fmt, ap);
      va_end(ap);
    }
}
