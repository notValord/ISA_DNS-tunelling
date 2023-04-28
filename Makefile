REC_DIR=receiver
REC=$(REC_DIR)/dns_receiver
REC_SOURCES := $(REC_DIR)/dns_receiver.c dns_coding.c dns_packet.c $(REC_DIR)/dns_receiver_events.c

SEND_DIR=sender
SEND=$(SEND_DIR)/dns_sender
SEND_SOURCES := $(SEND_DIR)/dns_sender.c dns_coding.c dns_packet.c $(SEND_DIR)/dns_sender_events.c

CC=gcc

REC_OBJECTS=$(REC_SOURCES:.c=.o)
SEND_OBJECTS=$(SEND_SOURCES:.c=.o)

all: $(REC_DIR) $(SEND_DIR)

$(REC_DIR): $(REC)

$(SEND_DIR): $(SEND)

$(SEND): $(SEND_OBJECTS)
		$(CC) $(SEND_OBJECTS) -o $@

$(SEND_DIR)/dns_coding.o: dns_coding.c dns_coding.h
		$(CC) -g -o $@ -c $<

$(SEND_DIR)/dns_packet.o: dns_packet.c dns_packet.h
		$(CC) -g -o $@ -c $<

$(SEND_DIR)/%.o: $(SEND_DIR)/%.c $(SEND_DIR)/%.h
		$(CC) -o $@ -c $<


$(REC): $(REC_OBJECTS)
		$(CC) $(REC_OBJECTS) -o $@

$(REC_DIR)/dns_coding.o: dns_coding.c dns_coding.h
		$(CC) -g -o $@ -c $<

$(REC_DIR)/dns_packet.o: dns_packet.c dns_packet.h
		$(CC) -g -o $@ -c $<

$(REC_DIR)/%.o: $(REC_DIR)/%.c $(REC_DIR)/%.h
		$(CC) -o $@ -c $<

clean: clean_send clean_rec

clean_send: 
		rm -f $(SEND_OBJECTS) $(SEND)

clean_rec: 
		rm -f $(REC_OBJECTS) $(REC)

.PHONY: all clean
