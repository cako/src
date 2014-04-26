/* POCS for 5D seismic data interpolation
*/
/*
  Copyright (C) 2014  Xi'an Jiaotong University, UT Austin (Pengliang Yang)

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
#include <rsf.h>
#include <complex.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "fftn.h"

int main(int argc, char* argv[])
{
    bool verb;
    int n1,n2,n3,n4,n5, num, i1,i2,i3,i4,i5, index, nthr, niter, iter;
    int n[5];
    float thr, pclip, m;		
    float *din, *mask, *dout;
    sf_complex *dobs,*drec,*dtmp;
    sf_file Fin,Fout, Fmask;/* mask and I/O files*/ 

    sf_init(argc,argv);	/* Madagascar initialization */
    omp_init(); 	/* initialize OpenMP support */

    /* setup I/O files */
    Fin=sf_input("in");	/* read the data to be interpolated */
    Fmask=sf_input("mask");  	/* read the 2-D mask for 3-D data */
    Fout=sf_output("out"); 	/* output the reconstructed data */
 
    if(!sf_getbool("verb",&verb)) 	verb=false;
    /* verbosity */
    if (!sf_getint("niter",&niter)) 	niter=100;
    /* total number of POCS iterations */
    if (!sf_getfloat("pclip",&pclip)) 	pclip=99.;
    /* starting data clip percentile (default is 99)*/

    /* Read the data size */
    if (!sf_histint(Fin,"n1",&n1)) sf_error("No n1= in input");
    if (!sf_histint(Fin,"n2",&n2)) sf_error("No n2= in input");
    if (!sf_histint(Fin,"n3",&n3)) sf_error("No n3= in input");
    if (!sf_histint(Fin,"n4",&n4)) sf_error("No n4= in input");
    if (!sf_histint(Fin,"n5",&n5)) sf_error("No n5= in input");


    num=n1*n2*n3*n4*n5;//total number of elements 
    n[0]=n1; n[1]=n2; n[2]=n3; n[3]=n4; n[4]=n5;

    /* allocate data and mask arrays */
    din=sf_floatalloc(num); 
    dout=sf_floatalloc(num);
    dobs=sf_complexalloc(num);
    drec=sf_complexalloc(num);
    dtmp=sf_complexalloc(num);

    sf_floatread(din,num,Fin);
    if (NULL != sf_getstring("mask")){
	mask=sf_floatalloc(n2*n3*n4*n5);
	sf_floatread(mask,n2*n3*n4*n5,Fmask);
    }
    for(i1=0; i1<num; i1++) 
    {
	dobs[i1]=sf_cmplx(din[i1],0);
	drec[i1]=dobs[i1];
    }
    fftn_init(5, n);

    for(iter=1; iter<=niter; iter++)
    {
	fftn_lop(true, false, num, num, dtmp, drec);

	// perform hard thresholding
#ifdef _OPENMP
#pragma omp parallel for default(none)	\
	private(i1)			\
	shared(dout,dtmp,num)
#endif
	for(i1=0; i1<num; i1++)	dout[i1]=cabsf(dtmp[i1]);

   	nthr = 0.5+num*(1.-0.01*pclip); 
    	if (nthr < 0) nthr=0;
    	if (nthr >= num) nthr=num-1;
	thr=sf_quantile(nthr,num,dout);
	thr*=powf(0.01,(iter-1.0)/(niter-1.0));

#ifdef _OPENMP
#pragma omp parallel for default(none)	\
	private(i1)			\
	shared(dtmp,num,thr)
#endif
	for(i1=0; i1<num; i1++) dtmp[i1]*=(cabsf(dtmp[i1])>thr?1.:0.);

	fftn_lop(false, false, num, num, dtmp, drec);
	
	/* d_rec = d_obs+(1-M)*A T{ At(d_rec) } */
#ifdef _OPENMP
#pragma omp parallel for collapse(5) default(none)	\
	private(i1,i2,i3,i4,i5,index,m)			\
	shared(mask,drec,dobs,n1,n2,n3,n4,n5)
#endif
	for(i5=0; i5<n5; i5++)	
	for(i4=0; i4<n4; i4++)
	for(i3=0; i3<n3; i3++)	
	for(i2=0; i2<n2; i2++)
	for(i1=0; i1<n1; i1++)
	{ 
		m=(mask[i2+i3*n2+i4*n2*n3+i5*n2*n3*n4])?1:0;
		index=i1+n1*i2+n1*n2*i3+n1*n2*n3*i4+n1*n2*n3*n4*i5;
		drec[index]=dobs[index]	+(1.-m)*drec[index];
	}
	if (verb)    sf_warning("iteration %d;",iter);
    }

    for(i1=0; i1<num; i1++) dout[i1]=creal(drec[i1]);
    sf_floatwrite(dout, num, Fout);

    fftn_close();
    sf_close();
    exit(0);
}
