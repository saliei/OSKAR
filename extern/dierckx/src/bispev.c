#include "extern/dierckx/bispev.h"
#include "extern/dierckx/fpbisp.h"

#ifdef __cplusplus
extern "C" {
#endif

void bispev(const float *tx, int nx, const float *ty, int ny,
    const float *c, int kx, int ky, const float *x, int mx, const float *y,
    int my, float *z, float *wrk, int lwrk, int *iwrk, int kwrk, int *ier)
{
    /* Local variables */
    int i, iw, lwest;

    /* before starting computations a data check is made. if the input data
     * are invalid control is immediately repassed to the calling program. */
    *ier = 10;
    lwest = (kx + 1) * mx + (ky + 1) * my;
    if (lwrk < lwest) return;
    if (kwrk < mx + my) return;
    if (mx < 1) return;
    if (my < 1) return;
    for (i = 1; i < mx; ++i)
    {
        if (x[i] < x[i - 1]) return;
    }
    for (i = 1; i < my; ++i)
    {
        if (y[i] < y[i - 1]) return;
    }
    *ier = 0;
    iw = mx * (kx + 1);
    fpbisp(tx, nx, ty, ny, c, kx, ky, x, mx, y, my,
        z, wrk, &wrk[iw], iwrk, &iwrk[mx]);
}

#ifdef __cplusplus
}
#endif
