#!/bin/sh

for i in stagefright media utils binder cutils ui; do
	nm -D --defined-only lib$i.so | awk '{print $3}' | grep -v ^__aeabi | grep '\(IInterface\|OMXObserver\|MemoryDealer\|Binder\|RefBase\|String\|OMXClient\|IMemory\|^str\|android_atomic\|GraphicBuffer\)' > lib$i.symbols
done

