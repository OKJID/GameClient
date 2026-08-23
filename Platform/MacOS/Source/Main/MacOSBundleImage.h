#pragma once

class Image;

// Loads a PNG shipped inside the application bundle and wraps it in an engine Image
// backed by a runtime texture. Results are cached; the caller does not own them.
const Image *MacOSGetBundleImage(const char *resourceName);
