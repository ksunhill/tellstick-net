
void rfMessageBegin();
void rfMessageBeginRaw();
void rfMessageEnd(unsigned char type);

void rfMessageAddByte(const char *key, unsigned char value);
void rfMessageAddLong(const char *key, unsigned long value);
void rfMessageAddString(const char *key, const char *value);
void rfMessageAddHexString(const char *key, const char *value, const unsigned char length);
