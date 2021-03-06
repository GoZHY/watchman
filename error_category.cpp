/* Copyright 2016-present Facebook, Inc.
 * Licensed under the Apache License, Version 2.0 */
#include "watchman.h"
#include "watchman_error_category.h"

using std::generic_category;

namespace watchman {

const char* error_category::name() const noexcept {
  return "watchman";
}

std::string error_category::message(int) const {
  return "the programmer should not be trying to render an error message "
         "using watchman::error_category, please report this bug!";
}

const std::error_category& error_category() {
  static class error_category cat;
  return cat;
}

const char* inotify_category::name() const noexcept {
  return "inotify";
}

std::string inotify_category::message(int err) const {
  switch (err) {
    case EMFILE:
      return "The user limit on the total number of inotify "
             "instances has been reached; increase the "
             "fs.inotify.max_user_instances sysctl";
    case ENFILE:
      return "The system limit on the total number of file descriptors "
             "has been reached";
    case ENOMEM:
      return "Insufficient kernel memory is available";
    case ENOSPC:
      return "The user limit on the total number of inotify watches "
             "was reached; increase the fs.inotify.max_user_watches sysctl";
    default:
      return std::generic_category().message(err);
  }
}

const std::error_category& inotify_category() {
  static class inotify_category cat;
  return cat;
}

bool error_category::equivalent(const std::error_code& code, int condition)
    const noexcept {
  if (code.category() == inotify_category()) {
    // Treat inotify the same as the generic category for the purposes of
    // equivalence; it is the same namespace, we just provide different
    // renditions of the error messages.
    return equivalent(
        std::error_code(code.value(), std::generic_category()), condition);
  }

  switch (static_cast<error_code>(condition)) {
    case error_code::no_such_file_or_directory:
      return
#ifdef _WIN32
          code == windows_error_code(ERROR_FILE_NOT_FOUND) ||
          code == windows_error_code(ERROR_DEV_NOT_EXIST) ||
#endif
          code == std::errc::no_such_file_or_directory;

    case error_code::not_a_directory:
      return
#ifdef _WIN32
          code == windows_error_code(ERROR_PATH_NOT_FOUND) ||
#endif
          code == std::errc::not_a_directory;

    case error_code::too_many_symbolic_link_levels:
      // POSIX says open with O_NOFOLLOW should set errno to ELOOP if the path
      // is a symlink. However, FreeBSD (which ironically originated O_NOFOLLOW)
      // sets it to EMLINK.  So we check for either condition here.
      return code == std::errc::too_many_symbolic_link_levels ||
          code == std::errc::too_many_links;

    case error_code::permission_denied:
      return
#ifdef _WIN32
          code == windows_error_code(ERROR_ACCESS_DENIED) ||
          code == windows_error_code(ERROR_INVALID_ACCESS) ||
          code == windows_error_code(ERROR_WRITE_PROTECT) ||
#endif
          code == std::errc::permission_denied ||
          code == std::errc::operation_not_permitted;

    case error_code::system_limits_exceeded:
      return
#ifdef _WIN32
          code == windows_error_code(ERROR_TOO_MANY_OPEN_FILES) ||
#endif
          code == std::errc::too_many_files_open_in_system ||
          code == std::errc::too_many_files_open;

    case error_code::timed_out:
      return
#ifdef _WIN32
          code == windows_error_code(ERROR_TIMEOUT) ||
          code == windows_error_code(WAIT_TIMEOUT) ||
#endif
          code == std::errc::timed_out;

    default:
      return false;
  }
}
}
