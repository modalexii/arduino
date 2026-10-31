// calibrate.h: Phase 1 hardware bring-up and calibration screen (CLOCK_PLAN.md §6, Phase 1).
// Enabled by CALIBRATE in main.cpp. Serial commands at 115200; type ? for help.
#pragma once

void calibrate_setup();
void calibrate_loop();
