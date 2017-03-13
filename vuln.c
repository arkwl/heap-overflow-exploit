#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <ctype.h>

#include "my_malloc.h"

#define MAX_GRP 100

#define err_abort(x) do { \
      if (!(x)) {\
         fprintf(stderr, "Fatal error: %s:%d: ", __FILE__, __LINE__);   \
         perror(""); \
         exit(1);\
      }\
   } while (0)

void print_escaped(FILE *fp, const char* buf, unsigned len) {
   int i;
   for (i=0; i < len; i++) {
      if (isprint(buf[i]))
         fputc(buf[i], stderr);
      else fprintf(stderr, "\\x%02hhx", buf[i]);
   }
}

/************ Function vulnerable to buffer overflow on stack ***************/

int auth(const char *username, int ulen, const char *pass, int plen) {
  char *user = alloca(LEN2 + (random() % LEN2));
  // make offsets unique for each group

  bcopy(username, user, ulen); // possible buffer overflow 

  unsigned l = (plen < ulen) ? plen : ulen;
  return (strncmp(user, pass, l) == 0);
}

int wrauth(const char *username, int ulen, const char *pass, int plen) {
   return auth(username, ulen, pass, plen);
}

int login_attempts;
void g(const char *username, int ulen, const char *pass, int plen) {
  int authd=0;
  char *s1 = "/bin/ls";
  char *s2 = "/bin/false";
  if (RANDOM) 
     authd |= wrauth(username, ulen, pass, plen);
  else authd |= auth(username, ulen, pass, plen);

  if (authd) {
     // Successfully authenticated
     fprintf(stderr, "Authentication succeeded, executing ls\n");
     err_abort(execl(s1, "ls", NULL)>=0); // Execute a shell, or
  }
  else { // Authentication failure
     fprintf(stderr, "Login denied, ");
     if (login_attempts++ > 3) {
        fprintf(stderr, "executing /bin/false\n");
        err_abort(execl(s2, "false", NULL)>=0); // a program that prints an error
     }
     else fprintf(stderr, "try again\n");
  }
}

#ifndef ASM_ONLY
void padding() {
int i, z;
#include "padding.h"
}
#endif

void ownme() {
   fprintf(stderr, "ownme called\n");
}

int main_loop(unsigned seed) {
   int nread;
   char *user=NULL, *pass=NULL;
   unsigned ulen=0, plen=0;

   srandom(seed);
   unsigned s = (unsigned)random();
   s = s % LEN1 + LEN1;
   char *rdbuf = (char*)alloca(s);
   char *tbuf;
   unsigned tbufsz = ((unsigned)random()) % LEN1;
   tbuf = (char*)alloca(tbufsz);

   do {
      err_abort((nread = read(0, rdbuf, s-1)) >= 0);
      if (nread == 0) {
         fprintf(stderr, "vuln: quitting\n");
         return 0;
      }
      fprintf(stderr, "vuln: Received:'");
      print_escaped(stderr, rdbuf, nread);
      fprintf(stderr, "'\n");
      rdbuf[nread] = '\0'; // null-terminate
      switch (rdbuf[0]) {

      case 'e': // echo command: e <string_to_echo>
         printf(&rdbuf[2]);
         fflush(stdout);
         break;

      case 'u': // provide username
         ulen = nread-3; // skips last char
         user = malloc(ulen);
         bcopy(&rdbuf[2], user, ulen);
         break;

      case 'p': // provide username
         pass = malloc(plen);
         plen = nread-3;
         bcopy(&rdbuf[2], pass, plen);
         break;

      case 'l': { // login using previously supplied username and password
         if (user != NULL && pass != NULL) {
            fprintf(stderr, "vuln: Got user=%s, pass=%s\n", user, pass);
            g(user, ulen, pass, plen);
            free(pass);
            free(user);
            user=pass=NULL;
            ulen=0; plen=0;
         }
         else fprintf(stderr, "vuln: Use u and p commands before logging in\n");
         break;
      }

      case 'q':
         fprintf(stderr, "vuln: quitting\n");
         return 0;

      default:
         fprintf(stderr, "vuln: Invalid operation. Valid commands are:\n");
         fprintf(stderr, "\te <data>: echo <data>\n");
         fprintf(stderr, "\tu <user>: enter username\n");
         fprintf(stderr, "\tp <pass>: enter password\n");
         fprintf(stderr, 
                 "\tl: login using previously provided username/password\n");
         fprintf(stderr, "\tq: quit\n");
         break;
      }
   } while (1);
}

int main(int argc, char *argv[]) {

   unsigned seed=GRP;

   if (argc >= 2) seed = atoi(argv[1]);
   if (seed > MAX_GRP) {
      fprintf(stderr, "Usage: %s <group_id>\n", argv[0]);
      fprintf(stderr, "<group_id> must be between 0 and %d\n", MAX_GRP);
      exit(1);
   }

  return main_loop(seed);
};
