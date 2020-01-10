# intel_device_lost
Repro example for Intel device lost

Got eDeviceLost on queue::submit at second blit command

Crashes on configuration:
* Win 10, Intel 620, driver 26.20.100.6860
Works fine on:
* Win 10, Intel 640, driver 25.20.100.6471
* NVidia
* AMD