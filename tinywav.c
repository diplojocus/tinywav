/**
 * Copyright (c) 2015,2016, Martin Roth (mhroth@gmail.com)
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


#include <assert.h>
#include <stddef.h>
#if _WIN32
#include <winsock2.h>
#include <malloc.h>
#else
#include <alloca.h>
#include <netinet/in.h>
#endif
#include "tinywav.h"

typedef struct TinyWavHeader {
  uint32_t ChunkID;
  uint32_t ChunkSize;
  uint32_t Format;
  uint32_t Subchunk1ID;
  uint32_t Subchunk1Size;
  uint16_t AudioFormat;
  uint16_t NumChannels;
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
  uint32_t Subchunk2ID;
  uint32_t Subchunk2Size;
} TinyWavHeader;

int tinywav_new(TinyWav *tw,
    int16_t numChannels, int32_t samplerate,
    TinyWavSampleFormat sampFmt, TinyWavChannelFormat chanFmt,
    const char *path) {
#if _WIN32
  errno_t err = fopen_s(&tw->f, path, "w");
  assert(err == 0);
#else
  tw->f = fopen(path, "w");
#endif
  assert(tw->f != NULL);
  tw->numChannels = numChannels;
  tw->totalFramesWritten = 0;
  tw->sampFmt = sampFmt;
  tw->chanFmt = chanFmt;

  // prepare WAV header
  TinyWavHeader h;
  h.ChunkID = htonl(0x52494646); // "RIFF"
  h.ChunkSize = 0; // fill this in on file-close
  h.Format = htonl(0x57415645); // "WAVE"
  h.Subchunk1ID = htonl(0x666d7420); // "fmt "
  h.Subchunk1Size = 16; // PCM
  h.AudioFormat = (tw->sampFmt-1); // 1 PCM, 3 IEEE float
  h.NumChannels = numChannels;
  h.SampleRate = samplerate;
  h.ByteRate = samplerate * numChannels * tw->sampFmt;
  h.BlockAlign = numChannels * tw->sampFmt;
  h.BitsPerSample = 8*tw->sampFmt;
  h.Subchunk2ID = htonl(0x64617461); // "data"
  h.Subchunk2Size = 0; // fill this in on file-close

  // write WAV header
  fwrite(&h, sizeof(TinyWavHeader), 1, tw->f);

  return 0;
}

int tinywav_read(TinyWav *tw, const char *path) {

  tw->in_f = fopen(path, "r");

  if (tw->in_f == NULL) return -1; // input file doesn't exist

  // calculate file size
  fseek(tw->in_f, 0, SEEK_END);
  uint64_t fileSize = ftell(tw->in_f);
  rewind(tw->in_f);

  const size_t headerSize = sizeof(TinyWavHeader);

  if (fileSize < headerSize) return -2; // unreadable wav file

  TinyWavHeader h;
  fread(&h, 1, headerSize, tw->in_f);


  printf("HeaderSize: %lu\n", headerSize);
  printf("h.ChunkID %#08x\n", ntohl(h.ChunkID));
  printf("h.ChunkSize %u\n", h.ChunkSize);
  printf("h.Format %#08x\n", ntohl(h.Format));
  printf("h.Subchunk1ID %#08x\n", ntohl(h.Subchunk1ID));
  printf("h.Subchunk1Size %u\n", h.Subchunk1Size);
  printf("h.AudioFormat %u\n", h.AudioFormat);
  printf("h.NumChannels %u\n", h.NumChannels);
  printf("h.SampleRate %u\n", h.SampleRate);
  printf("h.ByteRate %u\n", h.ByteRate);
  printf("h.BlockAlign %u\n", h.BlockAlign);
  printf("h.BitsPerSample %u\n", h.BitsPerSample);
  uint8_t buf4[4];
  buf4[0] = (h.Subchunk2ID & 0x000000ff);
  buf4[1] = (h.Subchunk2ID & 0x0000ff00) >> 8;
  buf4[2] = (h.Subchunk2ID & 0x00ff0000) >> 16;
  buf4[3] = (h.Subchunk2ID & 0xff000000) >> 24;
  printf("h.Subchunk2ID %#08x %c%c%c%c\n", ntohl(h.Subchunk2ID), buf4[0], buf4[1], buf4[2], buf4[3]);
  printf("h.Subchunk2Size %u\n", h.Subchunk2Size);

  if (h.AudioFormat != 1 && h.AudioFormat != 3) return -2; // needs to be PCM or IEEE float encoded

  return 0;
}

size_t tinywav_write_f(TinyWav *tw, void *f, int len) {
  switch (tw->sampFmt) {
    case TW_INT16: {
      int16_t *z = (int16_t *) alloca(tw->numChannels*len*sizeof(int16_t));
      switch (tw->chanFmt) {
        case TW_INTERLEAVED: {
          const float *const x = (const float *const) f;
          for (int i = 0; i < tw->numChannels*len; ++i) {
            z[i] = (int16_t) (x[i] * 32767.0f);
          }
          break;
        }
        case TW_INLINE: {
          const float *const x = (const float *const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = (int16_t) (x[j*len+i] * 32767.0f);
            }
          }
          break;
        }
        case TW_SPLIT: {
          const float **const x = (const float **const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = (int16_t) (x[j][i] * 32767.0f);
            }
          }
          break;
        }
        default: return 0;
      }

      tw->totalFramesWritten += len;
      return fwrite(z, sizeof(int16_t), tw->numChannels*len, tw->f);
      break;
    }
    case TW_FLOAT32: {
      float *z = (float *) alloca(tw->numChannels*len*sizeof(float));
      switch (tw->chanFmt) {
        case TW_INTERLEAVED: {
          tw->totalFramesWritten += len;
          return fwrite(f, sizeof(float), tw->numChannels*len, tw->f);
        }
        case TW_INLINE: {
          const float *const x = (const float *const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = x[j*len+i];
            }
          }
          break;
        }
        case TW_SPLIT: {
          const float **const x = (const float **const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = x[j][i];
            }
          }
          break;
        }
        default: return 0;
      }

      tw->totalFramesWritten += len;
      return fwrite(z, sizeof(float), tw->numChannels*len, tw->f);
    }
    default: return 0;
  }
}

void tinywav_close(TinyWav *tw) {
  uint32_t data_len = tw->totalFramesWritten * tw->numChannels * tw->sampFmt;

  // set length of data
  fseek(tw->f, 4, SEEK_SET);
  uint32_t chunkSize_len = 36 + data_len;
  fwrite(&chunkSize_len, sizeof(uint32_t), 1, tw->f);

  fseek(tw->f, 40, SEEK_SET);
  fwrite(&data_len, sizeof(uint32_t), 1, tw->f);

  fclose(tw->f);
  tw->f = NULL;
}

bool tinywav_isOpen(TinyWav *tw) {
  return (tw->f != NULL);
}
