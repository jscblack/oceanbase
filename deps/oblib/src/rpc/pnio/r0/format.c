void format_init(format_t* f, int64_t limit) {
  f->limit = limit;
  f->pos = 0;
  f->buf[0] = 0;
}

void format_reset(format_t* f) {
  f->pos = 0;
  f->buf[0] = 0;
}

char* format_gets(format_t* f) {
  return f->buf;
}

char* format_vsf(format_t* f, const char* format, va_list ap) {
  char* buf = f->buf + f->pos;
  int64_t limit = f->limit - f->pos;
  int64_t cnt = vsnprintf(buf, limit, format, ap);
  if (cnt < 0 || cnt >= limit) {
    format_reset(f);
  } else {
    f->pos += cnt;
  }
  return buf;
}

char* format_append(format_t* f, const char* format, ...) {
  char* ret = NULL;
  va_list ap;
  va_start(ap, format);
  ret = format_vsf(f, format, ap);
  va_end(ap);
  return ret;
}

char* format_sf(format_t* f, const char* format, ...) {
  char* ret = NULL;
  va_list ap;
  f->pos++;
  va_start(ap, format);
  ret = format_vsf(f, format, ap);
  va_end(ap);
  return ret;
}

char *strf(char* buf, int64_t size, const char *f, ...) {
  va_list ap;
  va_start(ap, f);
  vsnprintf(buf, size, f, ap);
  va_end(ap);
  return buf;
}
