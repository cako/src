/* Velocity continuation with semblance computation.
*/
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

#include <rsf.h>

#include "fint1.h"
#include "cosft.h"

int main(int argc, char* argv[])
{
    fint1 str, istr;
    int i1,i2, n1,n2,n3, ix,iv,ih, ib, ie, nb, nx,nv,nh, nw, next;
    float d1,o1,d2,o2, eps, w,x,k, v0,v2,v,v1,dv, dx, h0,dh,h, num, den, t;
    float *trace, *strace, ***stack, ***stack2, ***cont, **image;
    float complex *ctrace, *ctrace0;
    sf_file in, out;

    sf_init (argc,argv);
    in = sf_input("in");
    out = sf_output("out");

    if (!sf_histint(in,"n1",&n1)) sf_error("No n1= in input");
    if (!sf_histint(in,"n2",&nx)) sf_error("No n2= in input");
    if (!sf_histint(in,"n3",&nh)) sf_error("No n3= in input");

    if (!sf_getint("nb",&nb)) nb=2;
    if (!sf_getfloat("eps",&eps)) eps=0.01;
    if (!sf_getint("pad",&n2)) n2=n1;
    if (!sf_getint("pad2",&n3)) n3=n2;

    n3 = sf_npfar(n3);
    nw = n3/2+1;

    if (!sf_histfloat(in,"o1",&o1)) o1=0.;  
    o2 = o1*o1;
 
    if(!sf_histfloat(in,"d1",&d1)) sf_error("No d1= in input");
    d2 = o1+(n1-1)*d1;
    d2 = (d2*d2 - o2)/(n2-1);

    if (!sf_getint("nv",&nv)) sf_error("Need nv=");
    if (!sf_getfloat("dv",&dv)) sf_error("Need dv=");
    if (!sf_getfloat("v0",&v0) && 
	!sf_histfloat(in,"v0",&v0)) sf_error("Need v0=");

    if(!sf_histfloat(in,"o3",&h0)) sf_error("No o2= in input");
    if(!sf_histfloat(in,"d3",&dh)) sf_error("No d2= in input");
    if(!sf_histfloat(in,"d2",&dx)) sf_error("No d3= in input");
    
    sf_putfloat(out,"o3",v0+dv);
    sf_putfloat(out,"d3",dv);
    sf_putint(out,"n3",nv);

    sf_putstring(out,"label3","Velocity (km/s)");

    dx = 2.*SF_PI/(sf_npfar(2*(nx-1))*dx);

    stack = sf_floatalloc3(n1,nx,nv);
    stack2 = sf_floatalloc3(n1,nx,nv);
    cont = sf_floatalloc3(n1,nx,nv);
    image = sf_floatalloc2(n1,nx);
    trace = sf_floatalloc(n1);
    strace = sf_floatalloc(n3);
    ctrace = sf_complexalloc(nw);
    ctrace0 = sf_complexalloc(nw);

    if (!sf_getint("extend",&next)) next=4;
    /* trace extension */
    str = fint1_init(next,n1);
    istr = fint1_init(next,n2);

    for (i1=0; i1 < n1*nx*nv; i1++) {
	stack[0][0][i1] = 0.;
	stack2[0][0][i1] = 0.;
    }

    cosft_init(nx);

    for (ih=0; ih < nh; ih++) {
	sf_warning("offset %d of %d",ih+1,nh);

	h = h0 + ih*dh;
	h *= h;

	sf_floatread(image[0],n1*nx,in);

	for (i1=0; i1 < n1; i1++) {
	    cosft_frw(image[0],i1,n1);
	}

	for (ix=0; ix < nx; ix++) {
	    x = ix*dx; 
	    x *= x;

	    k = x * 0.25 * 0.25 * 0.5;

	    fint1_set(str,image[ix]);

	    for (i2=0; i2 < n2; i2++) {
		t = o2+i2*d2;
		t = sqrtf(t);
		t = (t-o1)/d1;
		i1 = t;
		if (i1 >= 0 && i1 < n1) {
		    strace[i2] = fint1_apply(str,i1,t-i1,false);
		} else {
		    strace[i2] = 0.;
		}
	    }
	    
	    for (i2=n2; i2 < n3; i2++) {
		strace[i2] = 0.;
	    }
		
	    sf_pfarc(1,n3,strace,ctrace0);

	    for (iv=0; iv < nv; iv++) {
		v = v0 + (iv+1)* dv;

		v1 = h * (1./(v*v) - 1./(v0*v0)) * 8.;
		v2 = k * ((v0*v0) - (v*v));

		ctrace[0]=0.; /* dc */

		for (i2=1; i2 < nw; i2++) {
		    w = i2*SF_PI/(d2*n3);
 
		    ctrace[i2] = ctrace0[i2] * cexpf(-I*(v2/w+(v1-o2)*w));
		} /* w */

		sf_pfacr(-1,n3,ctrace,strace);
		
		fint1_set(istr,strace);
		
		for (i1=0; i1 < n1; i1++) {
		    t = o1+i1*d1;
		    t = t*t;
		    t = (t-o2)/d2;
		    i2 = t;
		    if (i2 >= 0 && i2 < n2) {
			cont[iv][ix][i1] = fint1_apply(istr,i2,t-i2,false);
		    } else {
			cont[iv][ix][i1] = 0.;
		    }
		}
	    } /* v */
	} /* x */

	for (iv=0; iv < nv; iv++) {
	    for (i1=0; i1 < n1; i1++) {
		cosft_inv(cont[0][0],i1+iv*nx*n1,n1);
	    }
	}

	for (iv=0; iv < nv; iv++) {
	    for (ix=0; ix < nx; ix++) {
		for (i1=0; i1 < n1; i1++) {	
		    t = cont[iv][ix][i1];
		    stack[iv][ix][i1] += t;
		    stack2[iv][ix][i1] += t*t;
		} /* i1 */
	    } /* x */
        } /* v */
    } /* h */

    for (iv=0; iv < nv; iv++) {
	for (ix=0; ix < nx; ix++) {
	    for (i1=0; i1 < n1; i1++) {
		ib = i1-nb > 0? i1-nb: 0;
		ie = i1+nb+1 < n1? i1+nb+1: n1;

		num = 0.;
		den = 0.;

		for (i2=ib; i2 < ie; i2++) {
		    t = stack[iv][ix][i2];
		    num += t*t;
		    den += stack2[iv][ix][i2];
		}

		den *= nh;

		trace[i1] = den > 0.? num/den: 0.;
	    }
	    sf_floatwrite (trace,n1,out);
	}
    }

    exit(0);
}

/* 	$Id: Mfourvc2.c,v 1.8 2004/07/02 11:54:47 fomels Exp $	 */
