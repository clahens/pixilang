FINAL_CFLAGS += -I$(DSP)

LIB_DSP = \
    dsp_tables.o \
    dsp_functions.o
    
dsp_tables.o: $(DSP)dsp_tables.cpp $(DSP)dsp.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(DSP)dsp_tables.cpp

dsp_functions.o: $(DSP)dsp_functions.cpp $(DSP)dsp.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(DSP)dsp_functions.cpp
