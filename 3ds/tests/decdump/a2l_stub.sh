#!/bin/sh
# DECDUMP stub for arm-none-eabi-addr2line: for every argument that starts with 0x it prints fn_<arg> and file.c:1. Other arguments are ignored.
for a in "$@"; do
  case "$a" in
    0x*) printf 'fn_%s\nfile.c:1\n' "$a" ;;
  esac
done
