/**
 * @file zasmb.c
 * @brief Build system for the zasm project.
 *
 * Handles compilation and linking of C source files for multiple targets and build types.
 */

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

/**
 * @brief List of supported target names.
 */
static const char TargetNames[] = "acdfmnps";

/**
 * @struct StrBuilder
 * @brief Utility for building strings from multiple parts.
 *
 * Used to efficiently concatenate strings for command and path construction.
 */
struct StrBuilder {
  const char **strs; /**< Array of string parts. */
  size_t len, cap;   /**< Current length and capacity. */
};

/**
 * @brief Print a formatted error message to stderr in red.
 * @param err Format string (like printf)
 * @param ... Arguments for the format string
 */
static void printError(const char *const err, ...);

// Arena allocator state
static uint8_t ArenaMem[8192];
static size_t ArenaPos = 0;

/**
 * @brief Allocate memory from a simple arena allocator.
 * @param len Number of bytes to allocate
 * @return Pointer to allocated memory
 */
static void *zalloc(const size_t len);

/**
 * @brief Reset the arena allocator (does not free memory).
 */
static void zclear(void);

/**
 * @brief Add a string to a StrBuilder.
 * @param sb Pointer to StrBuilder
 * @param str String to add
 */
static void addStr(struct StrBuilder *const sb, const char *const str);

/**
 * @brief Concatenate all strings in a StrBuilder and return the result.
 * @param sb Pointer to StrBuilder
 * @return Newly allocated concatenated string
 */
static char *endStr(struct StrBuilder *const sb);

/**
 * @brief Remove leading and trailing slashes from a path.
 * @param path Input path
 * @param len Output: length of stripped path
 * @return Pointer to start of stripped path
 */
static const char *stripPath(const char *const path, size_t *const len);

/**
 * @brief Build a path by joining all parts in a StrBuilder with '/'.
 * @param sb Pointer to StrBuilder
 * @return Newly allocated path string
 */
static char *buildPath(struct StrBuilder *const sb);

/**
 * @brief Get the output directory for a build configuration.
 * @param debug True for debug build
 * @param native True for native build
 * @param target Target character
 * @return Newly allocated output directory path
 */
static char *getOutDir(const bool debug, const bool native, const char target);

/**
 * @brief Run a shell command and print its output.
 * @param cmd Command string to execute
 */
static void runCmd(const char *const cmd);

/**
 * @brief Convert a character to uppercase (ASCII only).
 * @param c Input character
 * @return Uppercase character
 */
static char toUpper(const char c);

/**
 * @brief Compile a single C source file to an object file.
 * @param src Source file path
 * @param dst Output object file path
 * @param debug True for debug build
 * @param native True for native build
 * @param target Target character
 */
static void compileFile(
  const char *const src, const char *const dst,
  const bool debug, const bool native, const char target
);

/**
 * @brief Check if a directory entry is a C source file.
 * @param dir Directory entry
 * @return True if entry is a .c file
 */
static bool isCFile(const struct dirent *const dir);

/**
 * @brief Count the number of C source files in a directory.
 * @param d Directory pointer
 * @return Number of .c files
 */
static size_t countCFiles(DIR *const d);

/**
 * @brief Write the base names of C source files in a directory to an array.
 * @param files Output array of file base names
 * @param d Directory pointer
 */
static void writeCFiles(char **files, DIR *const d);

/**
 * @brief Get the list of C source file base names in the source directory.
 * @param file_count Output: number of files
 * @return Array of file base names (without .c extension)
 */
static char **getCFiles(size_t *const file_count);

/**
 * @brief Get the full path to a source file given its base name.
 * @param name File base name
 * @return Newly allocated source file path
 */
static char *getSrcFile(const char *const name);

/**
 * @brief Get the full path to an object file given its base name and output directory.
 * @param name File base name
 * @param out_dir Output directory
 * @return Newly allocated object file path
 */
static char *getDstFile(const char *const name, const char *const out_dir);

/**
 * @brief Determine if a source file needs to be recompiled.
 * @param src Source file path
 * @param dst Object file path
 * @return True if src is newer than dst or dst does not exist
 */
static bool needCompile(const char *const src, const char *const dst);

/**
 * @brief Compile all C source files if needed.
 * @param out_dir Output directory
 * @param files Array of file base names
 * @param file_count Number of files
 * @param debug True for debug build
 * @param native True for native build
 * @param target Target character
 * @param force Force recompilation
 * @return True if any file was compiled
 */
static bool compileFiles(
  const char *const out_dir,
  char **files, const size_t file_count,
  const bool debug, const bool native,
  const char target, const bool force
);

/**
 * @brief Link all object files into the final executable.
 * @param files Array of file base names
 * @param file_count Number of files
 * @param out_dir Output directory
 * @param target Target character
 * @param debug True for debug build
 * @param native True for native build
 */
static void linkFiles(
  char **files, const size_t file_count,
  const char *const out_dir, const char target,
  const bool debug, const bool native
);

/**
 * @brief Compile and link all source files for a given configuration.
 * @param out_dir Output directory
 * @param debug True for debug build
 * @param native True for native build
 * @param target Target character
 * @param force Force recompilation
 */
static void compile(const char *const out_dir, const bool debug,
                    const bool native, const char target, const bool force);

/**
 * @brief Convert a character to lowercase (ASCII only).
 * @param c Input character
 * @return Lowercase character
 */
static char toLower(const char c);

/**
 * @brief Check if a character is a valid target.
 * @param c Target character
 * @return True if valid target
 */
static bool isTarget(const char c);

/**
 * @brief Build for a specific configuration and target.
 * @param debug True for debug build
 * @param native True for native build
 * @param target Target character
 * @param force Force recompilation
 */
static void build(const bool debug, const bool native,
                  const char target, const bool force);

/**
 * @brief Usage string for the build tool.
 */
static const char *const USAGE = "usage: zasmb [d|r|n] [l|c|d|s|m] [f]\n";

/**
 * @brief Main entry point for the build tool.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(const int argc, char *const argv[]);

static void printError(const char *const err, ...) {
  fputs(COLOR_ERROR "error: " COLOR_RESET, stderr);
  va_list arg;
  va_start(arg, err);
  vfprintf(stderr, err, arg);
  va_end(arg);
  fputc('\n', stderr);
}

static void *zalloc(const size_t len) {
  if (ArenaPos + len > sizeof(ArenaMem)) {
    printError("OOM");
    exit(1);
  }
  void *const mem = ArenaMem + ArenaPos;
  ArenaPos += len;
  return mem;
}

static void zclear(void) {
  // Reset arena position for reuse
  ArenaPos = 0;
}

static void addStr(struct StrBuilder *const sb, const char *const str) {
  // Grow array if needed
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
  // Concatenate all strings in builder
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
  // Remove leading and trailing slashes
  size_t start = 0;
  for (; path[start] == '/'; ++start);
  const size_t l = strlen(path);
  size_t end = 0;
  for (; end < l && path[l - 1 - end] == '/'; ++end);
  *len = l - start - end;
  return path + start;
}

static char *buildPath(struct StrBuilder *const sb) {
  // Join all parts with '/'
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
  // Build output directory path
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
  // Print and run shell command
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
  // Convert to uppercase if lowercase
  if ('a' <= c && c <= 'z')
    return c - 'a' + 'A';
  return c;
}

static void compileFile(
  const char *const src, const char *const dst,
  const bool debug, const bool native, const char target
) {
  // Build and run clang command for a single file
  char t[2] = {toUpper(target), 0};
  struct StrBuilder sb = {0};
  addStr(&sb, "clang -xc -c -std=c23 -DZA_TGT=ZA_TGT_");
  addStr(&sb, t);
  addStr(&sb, " -o ");
  addStr(&sb, dst);
  addStr(&sb, " ");
  addStr(&sb, src);
  addStr(&sb, " " WARNINGS " ");
  if (debug) addStr(&sb, DEBUG_FLAGS);
  else if (native) addStr(&sb, NATIVE_FLAGS);
  else addStr(&sb, RELEASE_FLAGS);
  runCmd(endStr(&sb));
}

static bool isCFile(const struct dirent *const dir) {
  // Check if file has .c extension
  if (dir->d_type != DT_REG) return false;
  const char *const name = dir->d_name;
  const size_t len = strlen(name);
  if (len < 2) return false;
  return name[len - 2] == '.' && name[len - 1] == 'c';
}

static size_t countCFiles(DIR *const d) {
  // Count .c files in directory
  struct dirent *dir;
  size_t count = 0;
  while ((dir = readdir(d)) != NULL) {
    if (isCFile(dir)) ++count;
  }
  return count;
}

static void writeCFiles(char **files, DIR *const d) {
  // Write base names of .c files to array
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
  // Get all .c file base names in SRC_DIR
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
  // Build full path to source file
  struct StrBuilder sb = {0};
  addStr(&sb, SRC_DIR "/");
  addStr(&sb, name);
  addStr(&sb, ".c");
  return endStr(&sb);
}

static char *getDstFile(const char *const name, const char *const out_dir) {
  // Build full path to object file
  struct StrBuilder sb = {0};
  addStr(&sb, out_dir);
  addStr(&sb, "/");
  addStr(&sb, name);
  addStr(&sb, ".o");
  return endStr(&sb);
}

static bool needCompile(const char *const src, const char *const dst) {
  // Check if object file is missing or older than source
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
  // Compile all files if needed
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
  // Link all object files into executable
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
  // Compile and link all files for configuration
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
  // Convert to lowercase if uppercase
  if ('A' <= c && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

static bool isTarget(const char c) {
  // Check if character is a valid target
  constexpr size_t target_count = sizeof(TargetNames) / sizeof(TargetNames[0]);
  for (size_t i = 0; i < target_count; ++i)
    if (c == TargetNames[i]) return true;
  return false;
}

static void build(const bool debug, const bool native,
                  const char target, const bool force) {
  // Build for a specific configuration
  printf("build type: %s\n", debug ? "debug" : (native ? "native" : "release"));
  printf("target: %c\n", target);
  const char *const out_dir = getOutDir(debug, native, target);
  printf("out dir: %s\n", out_dir);
  compile(out_dir, debug, native, target, force);
}

int main(const int argc, char *const argv[]) {
  // Parse command line arguments and run build
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
  if (build_type == 'd') debug = true;
  else if (build_type == 'r');
  else if (build_type == 'n') native = true;
  else {
    printError("invalid build type: %c", build_type);
    return 1;
  }
  const char target = toLower(argv[2][0]);
  if (!isTarget(target)) {
    printError("invalid target: %c", target);
    return 1;
  }

  if (target != 'a') build(debug, native, target, force);
  else {
    for (size_t i = 1; i < sizeof(TargetNames) / sizeof(TargetNames[0]) - 1; ++i) {
      const char t = TargetNames[i];
      build(debug, native, t, force);
      zclear();
    }
  }

  printf(COLOR_SUCCESS "build complete\n" COLOR_RESET);
  return 0;
}
