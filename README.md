# intel_device_lost
Repro example for Intel device lost

Got eDeviceLost on queue::submit at second blit command

Crashes on configuration:
* Win 10, Intel 620, driver 26.20.100.6860 (machine #1)

Works fine on:
* Win 10, Intel 630, driver 25.20.100.6471 (machine #2)
* Win 10, Intel 640, driver 25.20.100.6471 (machine #3)
* NVidia (machine #1, eGPU)
* AMD (machine #1, eGPU)
