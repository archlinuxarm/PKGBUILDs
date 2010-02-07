#include <stdio.h>
#include <fnmatch.h>
#include <string.h>
#include <malloc.h>

static char *my_getline(FILE *file) {
  static size_t size = 1024;
  static char *buf = NULL;
  static unsigned int i = 0, r = 0;;

  if(buf == NULL)
    buf = (char*)malloc(size);

  if(i) {
    memmove(buf, buf+i, size-i);
    r -= i;
    i = 0;
  }

  while(1) {
    if(i == size) {
      size *= 2;
      buf = (char*)realloc(buf, size);
    }

    if(i==r)
      r += fread(buf+i, 1, size-i, file);

    if(i==r && i == 0) {
      free(buf);
      buf = NULL;
      r = 0;
      return NULL;
    }

    if(i==r || buf[i] == '\n') {
      buf[i++] = '\0';
      return buf;
    }
    i++;
  }
}

int main(int argc, char *argv[]) {
  char *line, *pattern, *module;
  char *pos1, *pos2;

  if(argc != 3) {
      fprintf(stderr, "usage: resolve-modalias <alias file> <modalias>\n");
      return 1;
  }

  FILE *f=fopen(argv[1], "r");
  if(!f) {
    perror("error opening alias file");
    return 1;
  }

  while((line=my_getline(f))!=NULL) {
    if(!strncmp(line, "alias", strlen("alias"))) {
      pos1 = index(line, ' ');
      pos2 = index(pos1+1, ' ');
      pattern = pos1+1;
      *pos2 = '\0';
      module = pos2+1;

      if(!fnmatch(pattern, argv[2], 0))
        printf("%s\n", module);
    }
  }
  return 0;
}
//vim: set ts=2 sw=2 et:
