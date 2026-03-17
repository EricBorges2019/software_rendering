## 2025-03-10 - [PPM Framebuffer IO Optimization]
**Learning:** `std::fwrite` has overhead for every call. Writing 3 bytes per pixel individually for an 800x600 image causes 480,000 library calls per frame. Buffering an entire row of pixels (or the whole image) and making one `fwrite` call per row (or per image) provides a massive performance boost (nearly 2x speedup for the offline demo application).
**Action:** Always batch I/O operations. When writing image files or any large binary data, buffer the data in memory and write in large chunks rather than making many small `fwrite` calls.

## 2025-03-17 - [Renderer Dynamic Vector Allocation Avoidance]
**Learning:** `std::vector` allocations in `Renderer::drawMesh` per draw call cause large memory overhead when called thousands of times per frame. Using class members instead and just calling `.resize()` helps reuse their capacity.
**Action:** Always allocate vectors as members in render loops and prefer `.resize()` instead of local instantiations when the vector sizes match vertex or index counts.
