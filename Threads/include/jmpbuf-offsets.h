#ifndef JMPBUF_OFFSETS_H
#define JMPBUF_OFFSETS_H

/*
   This assignment only supports x86-64 architecture. The docker image
   provided for this assignment is based on x86-64 architecture. The macros
   have been taken from the glibc source code, accessible at
   https://github.com/bminor/glibc/blob/5ef608f36493c5d711418c5d31a7ebe710decc6e/sysdeps/x86_64/jmpbuf-offsets.h#L26

   Private macros for accessing __jmp_buf contents.  x86-64 version.
   Copyright (C) 2006-2023 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* We only need to save stackpointer and program counter.  */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7
#define JB_SIZE (8 * 8)

#endif // JMPBUF_OFFSETS_H