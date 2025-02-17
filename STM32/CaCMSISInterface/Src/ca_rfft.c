#include <stdlib.h>
#include <ca_rfft.h>

struct CA_rfft_ctx_ {
    arm_rfft_instance_q15 rfftq15;
    q15_t* outTable;
};

CA_rfft_ctx* ca_rfft_init(uint16_t fftLen)
{
    CA_rfft_ctx* ctx = calloc(1, sizeof(CA_rfft_ctx));

    // Use Forward FFT and bit order normal.
    arm_status ret = arm_rfft_init_q15(&ctx->rfftq15, fftLen, 0, 1);
    if (ret == ARM_MATH_SUCCESS)
    {
        // Output buffer is double the size to enable support for bit shifting in FFT algorithms.
        ctx->outTable = (q15_t *)calloc(2*fftLen, sizeof(q15_t));
        if (ctx->outTable == NULL) {
            free(ctx);
            ctx = NULL;
        }
    }
    else {
        free(ctx);
        ctx = NULL;
    }
    return ctx;
}

q15_t* ca_rfft(CA_rfft_ctx* ctx, int16_t *pData, int noOfChannels, int noOfSamples, int chOffset)
{
    if (ctx == NULL)
        return NULL;

    q15_t inTable[noOfSamples];
    for (int i=0; i<noOfSamples; i++)
    {
        inTable[i] = pData[i*noOfChannels + chOffset];
    }
    arm_rfft_q15(&ctx->rfftq15, inTable, ctx->outTable);

    return ctx->outTable;
}

int ca_rfft_absmax(q15_t *table, size_t size, q15_t* x, q15_t* y)
{
    // Find max in data
    q15_t maxAmp;
    uint32_t  idx;
    arm_absmax_q15(table, size, &maxAmp, &idx);

    if (idx == 0 || idx == (size -1)) {
        *x = *y = 0;
        return -1; // No valid max;
    }
    *x = idx;
    *y = maxAmp;

    return idx;
}
