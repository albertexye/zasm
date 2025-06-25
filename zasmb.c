#include <assert.h>
#include <dirent.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifndef BUILD_DIR
#define BUILD_DIR "out"
#endif

#ifndef SRC_DIR
#define SRC_DIR "src"
#endif

#ifndef WARNINGS
#define WARNINGS ""
#endif

#ifndef ARENA_SIZE
#define ARENA_SIZE 8192
#endif

#define COLOR_ERROR "\x1b[31m"
#define COLOR_SUCCESS "\x1b[32m"
#define COLOR_IGNORE "\x1b[90m"
#define COLOR_RESET "\x1b[0m"

#define DEBUG_FLAGS                                                            \
  "-O0 -g -fno-limit-debug-info -fno-omit-frame-pointer "                      \
  "-Weverything -Werror "                                                      \
  "-fsanitize=address,undefined,leak"
#define RELEASE_FLAGS "-O3 -ffast-math -DNDEBUG -flto -w"
#define NATIVE_FLAGS "-O3 -ffast-math -DNDEBUG -flto -w -march=native"

static const char TARGET_NAMES[] = "cdsmf";

struct StrBuilder {
  const char **strs;
  size_t len, cap;
};

static void printError(const char *const err, ...) {
  fputs(COLOR_ERROR "error: " COLOR_RESET, stderr);
  va_list arg;
  va_start(arg, err);
  vfprintf(stderr, err, arg);
  va_end(arg);
  fputc('\n', stderr);
}

static void *zalloc(const size_t len) {
  static uint8_t arena[ARENA_SIZE];
  static size_t pos = 0;
  if (pos + len > sizeof(arena)) {
    printError("OOM, consider setting ARENA_SIZE to a higher value");
    exit(1);
  }
  void *const mem = arena + pos;
  pos += len;
  return mem;
}

static void addStr(struct StrBuilder *const sb, const char *const str) {
  if (sb->len == sb->cap) {
    if (sb->cap == 0) sb->cap = 4;
    else sb->cap *= 2;
    const char **mem = zalloc(sizeof(char *) * sb->cap);
    memcpy(mem, sb->strs, sizeof(char *) * sb->len);
    sb->strs = mem;
  }
  sb->strs[sb->len] = str;
  ++(sb->len);
}

static char *endStr(struct StrBuilder *const sb) {
  size_t total = 0;
  for (size_t i = 0; i < sb->len; ++i)
    total += strlen(sb->strs[i]);
  char *const str = zalloc(total + 1);
  size_t off = 0;
  for (size_t i = 0; i < sb->len; ++i) {
    strcpy(str + off, sb->strs[i]);
    off += strlen(sb->strs[i]);
  }
  *sb = (struct StrBuilder){0};
  return str;
}

static const char *stripPath(const char *const path, size_t *const len) {
  size_t start = 0;
  for (; path[start] == '/'; ++start);
  const size_t l = strlen(path);
  size_t end = 0;
  for (; end < l && path[l - 1 - end] == '/'; ++end);
  *len = l - start - end;
  return path + start;
}

static char *buildPath(struct StrBuilder *const sb) {
  size_t total = 0;
  for (size_t i = 0; i < sb->len; ++i) {
    size_t len;
    stripPath(sb->strs[i], &len);
    total += len;
  }
  total += sb->len;
  char *const path = zalloc(total);
  size_t off = 0;
  for (size_t i = 0; i < sb->len; ++i) {
    size_t len;
    const char *const p = stripPath(sb->strs[i], &len);
    if (i) path[off++] = '/';
    memcpy(path + off, p, len);
    off += len;
  }
  path[off] = 0;
  *sb = (struct StrBuilder){0};
  return path;
}

static char *getOutDir(const bool debug, const bool native, const char target) {
  const char buf[2] = {target, 0};
  struct StrBuilder sb = {0};
  addStr(&sb, BUILD_DIR);
  if (debug) addStr(&sb, "debug");
  else if (native) addStr(&sb, "release-native");
  else addStr(&sb, "release");
  addStr(&sb, buf);
  return buildPath(&sb);
}

static void runCmd(const char *const cmd) {
  printf(COLOR_IGNORE "%s" COLOR_RESET "\n", cmd);
  FILE *const p = popen(cmd, "r");
  if (p == NULL) {
    printError("popen failed");
    exit(1);
  }
  char buf[64];
  while (fgets(buf, sizeof(buf), p) != NULL)
    printf("%s", buf);
  const int code = pclose(p);
  if (code == 0) return;
  printError("subcommand failed with code %d", code);
  exit(1);
}

static char toUpper(const char c) {
  if ('a' <= c && c <= 'z')
    return c - 'a' + 'A';
  return c;
}

static void compileFile(
  const char *const src, const char *const dst,
  const bool debug, const bool native, const char target
) {
  char t[2] = {toUpper(target), 0};
  struct StrBuilder sb = {0};
  addStr(&sb, "clang -xc -c -std=c23 -DZA_BUILD_TARGET=ZA_TGT_");
  addStr(&sb, t);
  addStr(&sb, " -o ");
  addStr(&sb, dst);
  addStr(&sb, " ");
  addStr(&sb, src);
  addStr(&sb, " " WARNINGS);
  addStr(&sb, " ");
  if (debug) addStr(&sb, DEBUG_FLAGS);
  else if (native) addStr(&sb, NATIVE_FLAGS);
  else addStr(&sb, RELEASE_FLAGS);
  runCmd(endStr(&sb));
}

static bool isCFile(const struct dirent *const dir) {
  if (dir->d_type != DT_REG) return false;
  const char *const name = dir->d_name;
  const size_t len = strlen(name);
  if (len < 2) return false;
  return name[len - 2] == '.' && name[len - 1] == 'c';
}

static size_t countCFiles(DIR *const d) {
  struct dirent *dir;
  size_t count = 0;
  while ((dir = readdir(d)) != NULL) {
    if (isCFile(dir)) ++count;
  }
  return count;
}

static void writeCFiles(char **files, DIR *const d) {
  size_t i = 0;
  struct dirent *dir;
  while ((dir = readdir(d)) != NULL) {
    if (!isCFile(dir)) continue;
    const size_t name_len = strlen(dir->d_name);
    files[i] = zalloc(name_len - 1);
    memcpy(files[i], dir->d_name, name_len - 2);
    files[i][name_len - 2] = 0;
    ++i;
  }
}

static char **getCFiles(size_t *const file_count) {
  DIR *const d = opendir(SRC_DIR);
  if (d == NULL) {
    printError("failed to open %s", SRC_DIR);
    exit(1);
  }
  *file_count = countCFiles(d);
  char **const files = zalloc(sizeof(char *) * *file_count);
  rewinddir(d);
  writeCFiles(files, d);
  closedir(d);
  return files;
}

static char *getSrcFile(const char *const name) {
  struct StrBuilder sb = {0};
  addStr(&sb, SRC_DIR "/");
  addStr(&sb, name);
  addStr(&sb, ".c");
  return endStr(&sb);
}

static char *getDstFile(const char *const name, const char *const out_dir) {
  struct StrBuilder sb = {0};
  addStr(&sb, out_dir);
  addStr(&sb, "/");
  addStr(&sb, name);
  addStr(&sb, ".o");
  return endStr(&sb);
}

static bool needCompile(const char *const src, const char *const dst) {
  struct stat st;
  assert(stat(src, &st) >= 0);
  const time_t src_time = st.st_mtim.tv_sec;
  if (stat(dst, &st) < 0) return true;
  const time_t exe_time = st.st_mtim.tv_sec;
  return exe_time < src_time;
}

static bool compileFiles(
  const char *const out_dir,
  char **files, const size_t file_count,
  const bool debug, const bool native,
  const char target, const bool force
) {
  bool compiled = false;
  for (size_t i = 0; i < file_count; ++i) {
    const char *const src = getSrcFile(files[i]);
    const char *const dst = getDstFile(files[i], out_dir);
    const bool need = needCompile(src, dst);
    if (!force && !need) continue;
    compiled = true;
    compileFile(src, dst, debug, native, target);
  }
  return compiled;
}

static void linkFiles(
  char **files, const size_t file_count,
  const char *const out_dir, const char target,
  const bool debug, const bool native
) {
  struct StrBuilder sb = {0};
  const char t[2] = {target, 0};
  addStr(&sb, "clang -o ");
  addStr(&sb, out_dir);
  addStr(&sb, "/zasm");
  addStr(&sb, t);
  addStr(&sb, " ");
  if (debug) addStr(&sb, DEBUG_FLAGS);
  else if (native) addStr(&sb, NATIVE_FLAGS);
  else addStr(&sb, RELEASE_FLAGS);
  for (size_t i = 0; i < file_count; ++i) {
    char *const src = getDstFile(files[i], out_dir);
    addStr(&sb, " ");
    addStr(&sb, src);
  }
  char *const cmd = endStr(&sb);
  runCmd(cmd);
}

static void compile(const char *const out_dir, const bool debug,
                    const bool native, const char target, const bool force) {
  size_t file_count;
  char **files = getCFiles(&file_count);
  struct StrBuilder sb = {0};
  addStr(&sb, "mkdir -p ");
  addStr(&sb, out_dir);
  runCmd(endStr(&sb));
  if (!compileFiles(out_dir, files, file_count, debug, native, target, force))
    return;
  linkFiles(files, file_count, out_dir, target, debug, native);
}

static char toLower(const char c) {
  if ('A' <= c && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

static bool isTarget(const char c) {
  constexpr size_t target_count = sizeof(TARGET_NAMES) / sizeof(TARGET_NAMES[0]);
  for (size_t i = 0; i < target_count; ++i)
    if (c == TARGET_NAMES[i]) return true;
  return false;
}

static const char *const USAGE = "usage: zasmb [d|r|n] [l|c|d|s|m] [f]\n";

int main(const int argc, char *const argv[]) {
  if (argc < 3 || argc > 4 || argv[1][1] || argv[2][1]) {
    printf(USAGE);
    return 1;
  }
  bool force = false;
  if (argc == 4) {
    if (toLower(argv[3][0]) != 'f' || argv[3][1]) {
      printf(USAGE);
      return 1;
    }
    force = true;
  }
  const char build_type = toLower(argv[1][0]);
  bool debug = false;
  bool native = false;
  if (build_type == 'd') {
    debug = true;
    printf("build type: debug\n");
  } else if (build_type == 'r') {
    printf("build type: release\n");
  } else if (build_type == 'n') {
    native = true;
    printf("build type: native\n");
  } else {
    printError("invalid build type: %c", build_type);
    return 1;
  }

  const char target = toLower(argv[2][0]);
  if (!isTarget(target)) {
    printError("invalid target: %c", target);
    return 1;
  }
  printf("target: %c\n", target);

  const char *const out_dir = getOutDir(debug, native, target);
  printf("out dir: %s\n", out_dir);
  compile(out_dir, debug, native, target, force);
  printf(COLOR_SUCCESS "build complete\n" COLOR_RESET);
  return 0;
}
