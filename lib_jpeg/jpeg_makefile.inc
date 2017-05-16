FINAL_CFLAGS += -I$(JPEG)

ifeq ($(TARGET_OS),palmos)
    LIB_JPEG = \
	jpeg_decoder.o \
	simple_jpeg_decoder.o
else
    LIB_JPEG = \
	jpeg_decoder.o \
	jpeg_encoder.o \
	jpgd.o \
	jpge.o
endif

jpeg_decoder.o: $(JPEG)jpeg_decoder.cpp $(JPEG)jpeg_decoder.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)jpeg_decoder.cpp

jpeg_encoder.o: $(JPEG)jpeg_encoder.cpp $(JPEG)jpeg_encoder.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)jpeg_encoder.cpp

jpgd.o: $(JPEG)jpgd.cpp $(JPEG)jpgd.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)jpgd.cpp

jpge.o: $(JPEG)jpge.cpp $(JPEG)jpge.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)jpge.cpp

simple_jpeg_decoder.o: $(JPEG)simple_jpeg_decoder.cpp $(JPEG)simple_jpeg_decoder.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)simple_jpeg_decoder.cpp
