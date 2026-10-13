/**
 * read file header (number of variables and number of clauses
 */
static void 
read_header (FILE* input, type_instance inst)
{
  char c, cnf[256];
  int  i;
  
  c = fgetc (input);
  while ( (c != 'c' ) && ( ! feof (input) ) ) 
    c = fgetc (input);
  i = 0;
  while ( ( c != ' ' ) && ( ! feof (input) ) ) 
    {
      cnf[i++] = c; 
      c = fgetc (input);
    }
  cnf[i] = '\0';
  if ( feof (input) || ( strcmp (cnf, "cnf") != 0 ) ) 
    {
      fprintf (stderr, "A set of clauses in the Zeta format was expected.\n");
      fflush (stderr);
      exit (0);
    }
    
  fscanf (input, "%d", &VARIABLE_NUMBER(inst));
  if ( VARIABLE_NUMBER(inst) == 0 ) 
    {
      fprintf (stderr, "The number of variables was expected.\n"); 
      fflush (stderr);
      exit (0);
    }
  fscanf (input, "%d", &CLAUSE_NUMBER(inst));
  if ( CLAUSE_NUMBER(inst) == 0 ) 
    {
      fprintf (stderr, "The number of clauses was expected.\n"); 
      fflush (stderr);
      exit (0);
    }

  c = fgetc (input);
  while ( c != '\n' ) 
    c = fgetc (input);
}

/**
 * read a clause
 */
static void 
read_clause (FILE* input, int* clause, int* size, int* card)
{
  int i;

  *card = 0;
  if ( fscanf (input, "%d", size) == 0 )
      fscanf (input, "*%d %d", card, size);
  for ( i = 0 ; i < *size ; i++ ) 
    fscanf (input, "%d", &clause[i]);
  clause[*size] = 0;
}

/**
 * read a problem instance file, return a new instance
 */
type_instance
read_instance (FILE * input)
{
  int *clause, size, card;
  
  read_header (input, newinst);

  while ( ( CURRENT_CLAUSE_INDEX(newinst) < CLAUSE_NUMBER(newinst) )&&
	  ( ! feof (input) ) ) 
    {
      read_clause (input, clause, &size, &card);
      if ( size == 0 ) 
	{
	  fprintf (stderr, "Warning, empty clause.\n"); 
	  fflush(stderr);
	}
    }
  if ( CURRENT_CLAUSE_INDEX(newinst) != CLAUSE_NUMBER(newinst) ) 
    {
      fprintf (stderr, "Unexpected end of file.\n"); 
      fflush (stderr);
      exit (0);
    }
  build_occurrences (newinst, 1);
  return newinst;
}
