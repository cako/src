/* Frequency-domain filtering. */
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>

#include "freqfilt.h"

#include "_bool.h"
#include "c99.h"
/*^*/

#include "c99.h"
#include "alloc.h"
#include "error.h"
#include "adjnull.h"
#include "kiss_fftr.h"

static int nfft, nw;
static float complex *cdata, *shape;
static float *tmp;
static kiss_fftr_cfg forw, invs;

void sf_freqfilt_init(int nfft1 /* time samples (possibly padded) */, 
		      int nw1   /* frequency samples */)
/*< Initialize >*/
{
    nfft = nfft1;
    nw = nw1;

    cdata = sf_complexalloc(nw);
    tmp = sf_floatalloc(nfft);
    forw = kiss_fftr_alloc(nfft,0,NULL,NULL);
    invs = kiss_fftr_alloc(nfft,1,NULL,NULL);
    if (NULL == forw || NULL == invs) 
	sf_error("%s: KISS FFT allocation problem",__FILE__);
}

void sf_freqfilt_set(float *filt /* frequency filter [nw] */)
/*< Initialize filter (zero-phase) >*/
{
    int iw;
    
    for (iw=0; iw < nw; iw++) {
	shape[iw] = filt[iw];
    }
}

#ifndef __cplusplus
/*^*/

void sf_freqfilt_cset(float complex *filt /* frequency filter [nw] */)
/*< Initialize filter >*/
{
    shape = filt;
}

#endif
/*^*/

void sf_freqfilt_close(void) 
/*< Free allocated storage >*/
{
    free(cdata);
    free(tmp);
    free(forw);
    free(invs);
}

void sf_freqfilt(int nx, float* x)
/*< Filtering in place >*/
{
    int iw;

    for (iw=0; iw < nx; iw++) {
	tmp[iw] = x[iw];
    }
    for (iw=nx; iw < nfft; iw++) {
	tmp[iw] = 0.;
    }

    kiss_fftr(forw, tmp, (kiss_fft_cpx *) cdata);
    for (iw=0; iw < nw; iw++) {
	cdata[iw] *= shape[iw];
    }
    kiss_fftri(invs,(const kiss_fft_cpx *) cdata, tmp);

    for (iw=0; iw < nx; iw++) {
	x[iw] = tmp[iw];
    } 
}

void sf_freqfilt_lop (bool adj, bool add, int nx, int ny, float* x, float* y) 
/*< Filtering as linear operator >*/
{
    int iw;

    sf_adjnull(adj,add,nx,ny,x,y);

    for (iw=0; iw < nx; iw++) {
	tmp[iw] = adj? y[iw] : x[iw];
    }
    for (iw=nx; iw < nfft; iw++) {
	tmp[iw] = 0.;
    }

    kiss_fftr(forw, tmp, (kiss_fft_cpx *) cdata);
    for (iw=0; iw < nw; iw++) {
	cdata[iw] *= shape[iw];
    }
    kiss_fftri(invs,(const kiss_fft_cpx *) cdata, tmp);

    for (iw=0; iw < nx; iw++) {	    
	if (adj) {
	    x[iw] += tmp[iw];
	} else {
	    y[iw] += tmp[iw];
	}
    } 
}

/* 	$Id$	 */
