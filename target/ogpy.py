import ctypes

og = ctypes.CDLL("./ogRacer.so")
og.mainF()