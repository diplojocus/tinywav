/**
 * Copyright (c) 2015, Martin Roth (mhroth@gmail.com)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "tinywav.h"
#include <stdio.h>

#define NUM_CHANNELS 1
#define SAMPLE_RATE 48000
#define BLOCK_SIZE 512
#define NUM_ITERATIONS 3

int main(int argc, char *argv[]) {

  // converts any wav file to mono 32bit floating point (takes the left channel)

  if (argc < 3) {
    printf("./convert infile.wav outfile.wav\n");
    return -1;
  }

  const char *inputPath = argv[1];

  TinyWav tw;
  int result = tinywav_read(&tw, inputPath);

  return result;
}
