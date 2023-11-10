#include <stdint.h>

typedef enum {
	MFI_SUCCESS = 0,
	MFI_ERR_BPP,
	MFI_ERR_FORMAT,
	MFI_ERR_MALLOC
} MfiResult;

MfiResult makeFixedImage(const XImage* img, char** outMemAddr) {
	if (img->bits_per_pixel != 32) return MFI_ERR_BPP;
	if (img->format != ZPixmap)    return MFI_ERR_FORMAT;

	int    bytesPerPixel = img->bits_per_pixel / 8;
	size_t imgsize       = img->width * img->height * bytesPerPixel;
	char*  fixedData     = malloc(imgsize);
	if (fixedData == NULL) return MFI_ERR_MALLOC;

	// copy data
	memcpy(fixedData, img->data, imgsize);

	// fix data
	for (size_t i = 0; i < imgsize; i += bytesPerPixel) {
		uint32_t* pix = (uint32_t*)(fixedData + i);
	
		// Convert from bgra (LSBFirst)
		if (img->byte_order == LSBFirst) {
			uint32_t color = *pix;
			uint32_t r = (color >> 16) & 0xff;
			uint32_t b = color & 0xff;
			*pix = (color & 0xff00ff00)
				 | (b << 16)
				 | r;
		// or argb (MSBFirst)
		} else {
			// TODO find a way to test this case
			uint32_t color = *pix;
			*pix = (color >> 24) | (color << 8);
		}
		// to rgba

		// 100% opacity if alpha is unused
		if (img->depth != 32)
			*pix |= 0xff << 24;
	}

	*outMemAddr = fixedData;
	return MFI_SUCCESS;
}

void writeImage(XImage* img, Tokens* t, bool overwrite) {
	char* fixedData      = NULL;
	const char* filename = NULL;
	int res;

	bool iOwnTheImage = false;
	if (img == NULL) {
		s.drawCmdBar = false;
		redraw();

		img = XGetImage(
			def.dis, def.win,
			0, 0,
			s.winW, s.winH,
			AllPlanes, ZPixmap
		);
		iOwnTheImage = true;
		s.drawCmdBar = true;
	}

	if (t->count <= 1) {
		strncpy(s.cmdbar, 
			"error: need a path or file name to write to", 
			CMDBAR_LEN
		);
		goto defer;
	} 

	// convert tokens of file name into one token
	for (size_t i = 1; i <= t->count-2; ++i) {
		char* terminator = t->items[i] + strlen(t->items[i]);
		*terminator = ' ';
	}
	filename = (const char*)strdup(t->items[1]);

	if (access(filename, F_OK) == 0 && !overwrite) {
		char* cmdname = strdup(t->items[0]);	
		snprintf(s.cmdbar, CMDBAR_LEN,
			"error: file %s exists; use :%s! to overwrite",
			filename, cmdname
		);
		free(cmdname);
		goto defer;
	}

	res = makeFixedImage(img, &fixedData);
	switch (res) {
	case MFI_ERR_BPP:
		strncpy(s.cmdbar,
			"error: only writing images with 32 bits per pixel"
			" is supported",
			CMDBAR_LEN
		);
		goto defer;
	case MFI_ERR_FORMAT:
		strncpy(s.cmdbar,
			"error: only writing images of format ZPixmap"
			" is supported",
			CMDBAR_LEN
		);
		goto defer;
	case MFI_ERR_MALLOC:
		strncpy(s.cmdbar,
			"error: could not allocate enough memory to convert"
			" the image to a proper format",
			CMDBAR_LEN
		);
		goto defer;
	}

	char* extension = strrchr(filename, '.');

	if (extension == NULL) {
		strncpy(s.cmdbar,
			"error: please give me a file name with an extension",
			CMDBAR_LEN
		);
		goto defer;
	}
	extension++;

	for (char* p = extension; *p; p++) *p = tolower(*p);

	int bytesPerPixel = img->bits_per_pixel / 8;
	if (strcmp(extension, "png") == 0) {
		res = stbi_write_png(filename, 
			img->width, img->height, bytesPerPixel,
			fixedData, img->bytes_per_line
		);
	} else if (
		strcmp(extension, "jpg") == 0 || 
		strcmp(extension, "jpeg") == 0
	) {
		res = stbi_write_jpg(filename,
			img->width, img->height, bytesPerPixel, 
			fixedData, 80
		);
	} else if (strcmp(extension, "bmp") == 0) {
		res = stbi_write_bmp(filename,
			img->width, img->height, bytesPerPixel, fixedData
		);
	} else if (strcmp(extension, "tga") == 0) {
		res = stbi_write_tga(filename,
			img->width, img->height, bytesPerPixel, fixedData
		);
	} else {
		snprintf(s.cmdbar, CMDBAR_LEN,
			"error: I don't know how to write extension \"%s\"",
			extension
		);
		goto defer;
	}

	if (!res) {
		snprintf(s.cmdbar, CMDBAR_LEN,
			"error: could not encode %s",
			filename
		);
		goto defer;
	}

	snprintf(s.cmdbar, CMDBAR_LEN, "Wrote to %s", filename);
defer:
	if (filename)            free((void*)filename);
	if (fixedData)           free(fixedData);
	if (iOwnTheImage && img) XDestroyImage(img);
}


void tokenize(Tokens* t, char* string, char* sep) {
	t->count = 0;

	char* token = strtok(string, sep);
	while (token != NULL) {
		nob_da_append(t, token);
		token = strtok(NULL, sep);
	}
}

void parse(Tokens* t) {
	if (t->count == 0) {
		// clear the buffer to prevent confusion that you're still
		// in command mode
		memset(s.cmdbar, 0, CMDBAR_LEN);
		return;
	}

	if (
		strcmp(t->items[0], "q") == 0 ||
		strcmp(t->items[0], "quit") == 0
	) {
		s.quit = true;
	} else if (
		strcmp(t->items[0], "w") == 0 ||
		strcmp(t->items[0], "write") == 0
	) {
		writeImage(def.img, t, false);
	} else if (
		strcmp(t->items[0], "w!") == 0 || 
		strcmp(t->items[0], "write!") == 0
	) {
		writeImage(def.img, t, true);
	} else if (
		strcmp(t->items[0], "wv") == 0 ||
		strcmp(t->items[0], "writeview") == 0
	) {
		writeImage(NULL, t, false);
	} else if (
		strcmp(t->items[0], "wv!") == 0 ||
		strcmp(t->items[0], "writeview!") == 0
	) {
		writeImage(NULL, t, true);
	} else if (strcmp(t->items[0], "pwd") == 0) {
		getcwd(s.cmdbar, CMDBAR_LEN);
	} else {
		char* cmdname = strdup(t->items[0]);
		snprintf(s.cmdbar, CMDBAR_LEN,
			"error: I don't know about command \"%s\"",
			cmdname
		);
		free(cmdname);
	}
}
