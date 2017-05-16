FINAL_CFLAGS += -I$(PIXILANG)

LIB_PIXILANG = \
    pixilang.o \
    pixilang_compiler.o \
    pixilang_symtab.o \
    pixilang_utils.o \
    pixilang_vm.o \
    pixilang_vm_audio.o \
    pixilang_vm_container.o \
    pixilang_vm_font.o \
    pixilang_vm_gfx.o \
    pixilang_vm_gfx_triangle.o \
    pixilang_vm_load_save.o \
    pixilang_vm_builtin_fns.o \
    pixilang_vm_builtin_fns_native.o \
    pixilang_vm_data_processing.o \
    pixilang_vm_code_analyzer.o \
    pixilang_vm_opengl.o
    
pixilang.o: $(PIXILANG)pixilang.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang.cpp

pixilang_compiler.o: $(PIXILANG)pixilang_compiler.cpp $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_compiler.cpp

pixilang_symtab.o: $(PIXILANG)pixilang_symtab.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_symtab.cpp

pixilang_utils.o: $(PIXILANG)pixilang_utils.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_utils.cpp

pixilang_vm.o: $(PIXILANG)pixilang_vm.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm.cpp

pixilang_vm_audio.o: $(PIXILANG)pixilang_vm_audio.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_audio.cpp

pixilang_vm_container.o: $(PIXILANG)pixilang_vm_container.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_container.cpp

pixilang_vm_font.o: $(PIXILANG)pixilang_vm_font.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_font.cpp

pixilang_vm_gfx.o: $(PIXILANG)pixilang_vm_gfx.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_gfx.cpp

pixilang_vm_gfx_triangle.o: $(PIXILANG)pixilang_vm_gfx_triangle.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_gfx_triangle.cpp

pixilang_vm_load_save.o: $(PIXILANG)pixilang_vm_load_save.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_load_save.cpp

pixilang_vm_builtin_fns.o: $(PIXILANG)pixilang_vm_builtin_fns.cpp $(PIXILANG)pixilang.h $(PIXILANG)pixilang_vm_builtin_fns.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_builtin_fns.cpp

pixilang_vm_builtin_fns_native.o: $(PIXILANG)pixilang_vm_builtin_fns_native.cpp $(PIXILANG)pixilang.h $(PIXILANG)pixilang_vm_builtin_fns.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_builtin_fns_native.cpp

pixilang_vm_data_processing.o: $(PIXILANG)pixilang_vm_data_processing.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_data_processing.cpp

pixilang_vm_code_analyzer.o: $(PIXILANG)pixilang_vm_code_analyzer.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_code_analyzer.cpp

pixilang_vm_opengl.o: $(PIXILANG)pixilang_vm_opengl.cpp $(PIXILANG)pixilang.h $(STD_INCS)
	$(CXX) $(FINAL_CFLAGS) -c $(PIXILANG)pixilang_vm_opengl.cpp
