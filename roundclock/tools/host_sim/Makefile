# See sim.cpp. Builds LVGL's C sources once into obj/, then links the sim.
LVGL ?= ../../.pio/libdeps/amoled143/lvgl
PROJ := ../..
CFLAGS := -O1 -I. -I$(PROJ)/include -I$(PROJ)/src -I$(LVGL) -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE -w
SRCS := $(shell find $(LVGL)/src -name '*.c' | grep -v -E '/(drivers|others/vg_lite_tvg|draw/(vg_lite|nxp|renesas|sdl|opengles|dma2d))/')
OBJS := $(patsubst $(LVGL)/%.c,obj/%.o,$(SRCS))
sim: $(OBJS) sim.cpp $(PROJ)/src/face.cpp $(PROJ)/src/ring_render.cpp
	g++ -std=c++17 $(CFLAGS) sim.cpp $(PROJ)/src/face.cpp $(PROJ)/src/ring_render.cpp $(OBJS) -lm -o $@
obj/%.o: $(LVGL)/%.c
	@mkdir -p $(dir $@); gcc $(CFLAGS) -c $< -o $@
clean:
	rm -rf obj sim *.raw *.png
