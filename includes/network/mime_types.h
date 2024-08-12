#pragma once

#include <string>

namespace mime_types {

	struct mapping {
		const char* extension;
		const char* mime_type;
	} mappings[] = {
        {".htm", "text/html"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"}, 
        {".jpe", "image/jpeg"}, 
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"}, 
        {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"},
		{ 0, 0 } // end of list.
	};

	std::string extension_to_mime_type(const std::string& extension) {
        std::string _extension  = extension;
        std::transform(
            _extension.begin(), 
            _extension.end(), 
            _extension.begin(),
            [](unsigned char c){ return std::tolower(c); }
        );

		for (auto m = mappings; m->extension; ++m) {
			if (m->extension == _extension) {
				return m->mime_type;
			}
		}

		return "application/octet-stream";
	}

} // namespace mime_types