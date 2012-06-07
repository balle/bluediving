// --Byron Darrah

/* Given a string, replaces all instances of "oldpiece" with "newpiece".
 *
 * Modified this routine to eliminate recursion and to avoid infinite
 * expansion of string when newpiece contains oldpiece.  --Byron

*/

// Modifications by Ollie Whitehouse <ol at uncon dot org>
// 
#include <stdio.h>

char *replace(char *string, char *oldpiece, char *newpiece) {

   int str_index, newstr_index, oldpiece_index, end,

   new_len, old_len, cpy_len;
   char *c;
   static char newstring[10000];

   if ((c = (char *) strstr(string, oldpiece)) == NULL)

      return string;

   new_len        = strlen(newpiece);
   old_len        = strlen(oldpiece);
   end            = strlen(string)   - old_len;
   oldpiece_index = c - string;


   newstr_index = 0;
   str_index = 0;
   while(str_index <= end && c != NULL)
   {

      /* Copy characters from the left of matched pattern occurence */
      cpy_len = oldpiece_index-str_index;
      strncpy(newstring+newstr_index, string+str_index, cpy_len);
      newstr_index += cpy_len;
      str_index    += cpy_len;

      /* Copy replacement characters instead of matched pattern */
      strcpy(newstring+newstr_index, newpiece);
      newstr_index += new_len;
      str_index    += old_len;

      /* Check for another pattern match */
      if((c = (char *) strstr(string+str_index, oldpiece)) != NULL)
         oldpiece_index = c - string;


   }
   /* Copy remaining characters from the right of last matched pattern */    strcpy(newstring+newstr_index, 
	string+str_index);

	return newstring;
}
