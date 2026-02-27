#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void die_perror(const char *msg) {
  perror(msg);
  exit(1);
}

static void die_msg(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static void ensure_room_dir(const char *room_dir) {
  if (mkdir(room_dir, 0777) == -1) {
    if (errno != EEXIST)
      die_perror("mkdir");
  }

  struct stat st;
  if (stat(room_dir, &st) == -1)
    die_perror("stat(room_dir)");
  if (!S_ISDIR(st.st_mode))
    die_msg("Room path exists but is not a directory");
}

static void ensure_user_fifo(const char *fifo_path) {
  struct stat st;
  if (lstat(fifo_path, &st) == 0) {
    if (!S_ISFIFO(st.st_mode))
      die_msg("User path exists but is not a FIFO");
    return;
  }
  if (errno != ENOENT)
    die_perror("lstat(user_fifo)");

  if (mkfifo(fifo_path, 0666) == -1) {
    if (errno == EEXIST)
      return;
    die_perror("mkfifo");
  }
}

static void write_all(int fd, const char *buf, size_t len) {
  size_t off = 0;
  while (off < len) {
    ssize_t n = write(fd, buf + off, len - off);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return;
    }
    off += (size_t)n;
  }
}

static void print_prompt(const char *roomname, const char *username) {
  printf("[%s] %s > ", roomname, username);
  fflush(stdout);
}

static void send_to_room(const char *room_dir, const char *username,
                         const char *msg) {
  DIR *d = opendir(room_dir);
  if (!d)
    die_perror("opendir");

  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (strcmp(ent->d_name, username) == 0)
      continue;

    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "%s/%s", room_dir, ent->d_name);
    if (n < 0 || (size_t)n >= sizeof(path))
      continue;

    struct stat st;
    if (lstat(path, &st) == -1)
      continue;
    if (!S_ISFIFO(st.st_mode))
      continue;

    pid_t pid = fork();
    if (pid < 0) {
      continue;
    }
    if (pid == 0) {
      int fd = open(path, O_WRONLY | O_NONBLOCK);
      if (fd < 0) {
        _exit(0);
      }
      write_all(fd, msg, strlen(msg));
      close(fd);
      _exit(0);
    }
    while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {
    }
  }

  closedir(d);
}

static void reader_loop(const char *roomname, const char *username,
                        const char *fifo_path) {
  int fd = open(fifo_path, O_RDWR);
  if (fd < 0)
    die_perror("open(user_fifo)");

  FILE *f = fdopen(fd, "r");
  if (!f)
    die_perror("fdopen");

  char line[4096];
  while (fgets(line, sizeof(line), f)) {
    printf("\r\033[2K");
    printf("[%s] %s", roomname, line);
    print_prompt(roomname, username);
  }

  fclose(f);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <roomname> <username>\n", argv[0]);
    return 1;
  }

  const char *roomname = argv[1];
  const char *username = argv[2];

  char room_dir[PATH_MAX];
  char fifo_path[PATH_MAX];

  int n1 = snprintf(room_dir, sizeof(room_dir), "/tmp/chatroom-%s", roomname);
  if (n1 < 0 || (size_t)n1 >= sizeof(room_dir))
    die_msg("Room name too long");

  int n2 =
      snprintf(fifo_path, sizeof(fifo_path), "%s/%s", room_dir, username);
  if (n2 < 0 || (size_t)n2 >= sizeof(fifo_path))
    die_msg("Username too long");

  ensure_room_dir(room_dir);
  ensure_user_fifo(fifo_path);

  setvbuf(stdout, NULL, _IOLBF, 0);
  printf("Welcome to %s!\n", roomname);

  pid_t rpid = fork();
  if (rpid < 0)
    die_perror("fork(reader)");
  if (rpid == 0) {
    reader_loop(roomname, username, fifo_path);
    return 0;
  }

  char *line = NULL;
  size_t cap = 0;
  while (1) {
    print_prompt(roomname, username);

    ssize_t got = getline(&line, &cap, stdin);
    if (got < 0)
      break;

    while (got > 0 && (line[got - 1] == '\n' || line[got - 1] == '\r')) {
      line[--got] = '\0';
    }
    if (got == 0)
      continue;

    char msg[8192];
    int nm = snprintf(msg, sizeof(msg), "%s: %s\n", username, line);
    if (nm < 0 || (size_t)nm >= sizeof(msg))
      continue;

    send_to_room(room_dir, username, msg);
    printf("[%s] %s: %s\n", roomname, username, line);
  }

  free(line);
  kill(rpid, SIGTERM);
  return 0;
}

