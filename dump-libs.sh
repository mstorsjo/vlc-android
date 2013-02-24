#!/bin/sh

for i in stagefright media utils binder cutils ui gui; do
	nm -D --defined-only lib$i.so | awk '{print $3}' | grep -v ^__aeabi | grep '\(IInterface\|OMXObserver\|MemoryDealer\|Binder\|RefBase\|String\|OMXClient\|IMemory\|^str\|android_atomic\|GraphicBuffer\|SurfaceTexture\)' > lib$i.symbols
done

