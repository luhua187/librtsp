#include "rtsp_os.h"
#include "base64.h"




int base64_encode(char* data, int dataSize, char* base64)
{
	const char base_64[128] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	int	padding;
	int	i = 0, j = 0,outSize = 0;
	char	*in, *out;

	int iOut = ((dataSize + 2) / 3) * 4;
	outSize = iOut += 2 * ((iOut / 60) + 1);
	out = (char *) malloc(outSize);

	in = data;

	if (outSize < (dataSize * 4 / 3)) 
        return 0;

	while (i < dataSize) 	{
		padding = 3 - (dataSize - i);
		if (padding == 2) {
			out[j] = base_64[in[i]>>2];
			out[j+1] = base_64[(in[i] & 0x03) << 4];
			out[j+2] = '=';
			out[j+3] = '=';
		} else if (padding == 1) {
			out[j] = base_64[in[i]>>2];
			out[j+1] = base_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = base_64[(in[i+1] & 0x0f) << 2];
			out[j+3] = '=';
		} else{
			out[j] = base_64[in[i]>>2];
			out[j+1] = base_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = base_64[((in[i+1] & 0x0f) << 2) | ((in[i+2] & 0xc0) >> 6)];
			out[j+3] = base_64[in[i+2] & 0x3f];
		}
		i += 3;
		j += 4;
	}

    
	out[j] = '\0';
    memcpy(base64, out, j+1);
	free(out);

    
	return j;
}