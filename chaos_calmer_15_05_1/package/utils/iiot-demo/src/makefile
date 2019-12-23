IIOTDEMO = iiotdemo

all:
	$(CC) $(CFLAGS) $(INCDIR) -o $(IIOTDEMO) main.c -lm -lpthread -lmosquitto -lmodbus
	
clean:
	 rm $(TARGET) $(IIOTDEMO)
