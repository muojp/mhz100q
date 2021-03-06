/* $Id: usb_writestring.c,v 1.6 2009/12/15 16:22:25 jrothwei Exp $ */
/* Copyright 2009 Joseph Rothweiler **
*************************************/
/* Joseph Rothweiler, Sensicomm LLC. Started 17Mar2009. */
/* Creating a .mex file to open a USB device using libusb.
** Usage:
**   usbdev = usb_writestring(usbdev,ep,string[,timeout[,do_int]])
** usbdev returns an integer value, which is the count
** of bytes actually sent.
** Values less than 0 indicate an error.
** FIXME: should also support INT8_T and UINT8_T strings.
*/

#include "mex.h"
#include <stdio.h>
#include <usb.h>

static double scalarcheck(
  const mxArray *matval,  /* The input array to check. */
  const int k,            /* Argument number, for debugging. */
  const int mytype        /* 0: numeric, 1: for usb_dev_handle. */
) {
	int mrows, ncols;

	if(mytype == 0) {
		if(!mxIsNumeric(matval)) {
			mexErrMsgIdAndTxt("USB:writestring","Argument %d must be numeric",k+1);
		}
	} else {
		if(!mxIsUint64(matval)) {
			mexErrMsgIdAndTxt("USB:writestring","Argument %d must be Uint64",k+1);
		}
	}
	if(mxIsComplex(matval)) {
		mexErrMsgIdAndTxt("USB:writestring","Argument %d must be noncomplex",k+1);
	}
	mrows = mxGetM(matval);
	ncols = mxGetN(matval);
	if(mrows!=1 || ncols != 1) {
			mexErrMsgIdAndTxt("USB:writestring","Argument %d must be scalar",k+1);
	}
	return mxGetScalar(matval);
}
/*****************************************************************************/
void mexFunction(
  int nlhs, mxArray *plhs[],
  int nrhs, const mxArray *prhs[]
) {
	int k, p;

	if( (nrhs<3) || (nrhs>5) ) {
		mexErrMsgIdAndTxt("USB:writestring","must have 3 or 4 arguments");
	}
	if(nlhs>1) {
		mexErrMsgIdAndTxt("USB:writestring","must have at most 1 result");
	}

	/**********************************************
	** Check the device. */
	int mrows, ncols;
	k=0;
	struct usb_dev_handle *usbhandle;
	UINT64_T hval;
	hval = scalarcheck(prhs[k],k,1);

	/* This assignment generates warnings about
	 * pointer/integer size mismatch. */
	usbhandle = (struct usb_dev_handle *)(hval);

	/**********************************************
	** Check the endpoint. */
	k=1;
	int ep;
	ep = scalarcheck(prhs[k],k,0);

	/**********************************************
	** Check the delay value. */

	int usbtimeout = 10;
	if(nrhs > 3) {
		k=3;
		usbtimeout = scalarcheck(prhs[k],k,0);
	}
	// mexPrintf("Setting timeout to %d ms\n",usbtimeout);

	/**********************************************
	** Check the bulk/int flag. */

	int do_int = 0;
	if(nrhs > 4) {
		k=4;
		do_int = scalarcheck(prhs[k],k,0);
	}
	if( (do_int!=0) && (do_int!=1) ) {
		mexErrMsgIdAndTxt("USB:writestring","Write type is %d. Must be 0(bulk) or 1(int)",do_int);
	}
	/**********************************************
	** Check and extract the string. Tricky:
	** Matlab uses 16-bit chars (UTF-16, with only
	** the 16-bit chars allowed? Essentially UCS-2? )
	** FIXME: mxChar is 8 bits in Octave, 16 bits in Matlab.
	*/
	k=2;
	mrows = mxGetM(prhs[k]);
	ncols = mxGetN(prhs[k]);
	// mexPrintf("rows %d cols %d\n",mrows,ncols);
	mxChar *mxch;
	mxch = mxGetChars(prhs[k]);
	// mexPrintf("size of mxChar is %d vs char %d\n",sizeof(mxChar),sizeof(char));
	// if(mxch==0) mexPrintf("mxch is NULL\n");
	// else {
	//	for(p=0;p<mrows*ncols;p++) mexPrintf("%d %4.4x %c %lc\n",
 	//		p,mxch[p],mxch[p],mxch[p]);
	// }
	if(mrows!=1) {
		mexErrMsgIdAndTxt("USB:writestring","usb_writestring: can only handle 1-row character arrays");
	}
	char *str;
	str = mxCalloc(ncols+1,sizeof(char));
	if(str==NULL) {
		mexErrMsgIdAndTxt("USB:writestring","usb_writestring: Trouble allocating internal memory");
	}
	/* Copy, possibly with a lossy conversion from 16 to 8 bits. */
	for(p=0;p<ncols;p++) str[p] = mxch[p];
	str[p] = '\0';  /* Not needed. */
	// mexPrintf("handle %x ep %d %x Output string %s\n",
	// 	usbhandle,ep,ep, str);
	int rtn;
	if(do_int==0) {
		rtn = usb_bulk_write(usbhandle,ep,str,ncols,usbtimeout); 
	} else {
		rtn = usb_interrupt_write(usbhandle,ep,str,ncols,usbtimeout); 
	}
	mxFree(str);

	/* If there's a return value, return rtn. **
	 * else, if rtn != size, throw an error.  */
	if(nlhs==1) {
		int ndims = 2;
		int dims[2] = {1,1};
		double *val;
		plhs[0] = mxCreateNumericArray(ndims,dims,mxDOUBLE_CLASS,mxREAL);
		val = mxGetData(plhs[0]);
		*val = rtn;
	} else if(rtn < 0) {
		mexErrMsgIdAndTxt("USB:writestring","write error code %d",rtn);
	} else if(rtn != ncols) {
		mexErrMsgIdAndTxt("USB:writestring","write size %d actual write was %d",ncols,rtn);
	}
}
