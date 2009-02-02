/*
	SuperCollider real time audio synthesis system
 Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com
 
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

//some UGens by Nick Collins 
//SLUGens released under the GNU GPL as extensions for SuperCollider 3, by Nick Collins, http://www.informatics.sussex.ac.uk/users/nc81/

#include "SC_PlugIn.h"

//#define SLUGENSRESEARCH 1


static InterfaceTable *ft; 

struct SortBuf : public Unit   //Karplus Strong with sorting algorithm on a buffer 
{
	uint32 mBufNum,mBufSize;
	int mOutStep;
	int mSorti, mSortj, mSortdone;
	
	float* mBufCopy;
};

struct GravityGrid : public Unit    
{
	float x[9],y[9],velx,vely,posx,posy;
	float * m_weights;
};


//max 25 masses
struct GravityGrid2 : public Unit    
{
	//float x[25],y[25],
	float velx,vely,posx,posy;
	//int nummasses;
	float * m_weights; //[25];
};


//bufnum, 0 capturetrigger 1, duration of repeat 2, ampdropout 3
struct Breakcore : public Unit    
{
	uint32 mBufNum,mBufSize;	
	float * mBuf;
	int captureon,captureneeds,repeatpos,capturepos;
	float m_prevtrig;
	//int duration;
};


//find maximum value within last x blocks
struct Max : public Unit {
	int m_blocks, m_counter,m_last;
	float m_max;     
	float* m_blockmaxes;
};

//print in lang a value every x blocks
struct PrintVal : public Unit {
	int m_count,m_block,m_id;
};

//from http://www.musicdsp.org/showone.php?id=97
struct EnvDetect : public Unit {
	float m_env; //m_atk,m_rel,
};

struct FitzHughNagumo : public Unit    
{
	float u,w;
};

struct DoubleWell : public Unit    
{
	float x,y,t;
};

struct DoubleWell2 : public Unit    
{
	float x,y,t;
};


struct DoubleWell3 : public Unit    
{
	float x,y,t;
};

struct WeaklyNonlinear : public Unit    
{
	float x,y,t;
};

struct WeaklyNonlinear2 : public Unit    
{
	float x,y,t;
};

struct TermanWang : public Unit    
{
	float x,y;
};

struct LTI : public Unit    
{	
	int sizea,sizeb;
	float * bufa; //feedback coeff
	float * bufb; //feedforward coeff
	float * mema, *memb;	
	int posa, posb;
};


struct NL : public Unit    
{	
	int sizea,sizeb;
	float * bufa; //feedback coeff
	float * bufb; //feedforward coeff
	float * mema, *memb;	
	
	int numasummands,numbsummands;
	
	int *aindices, *bindices;
	
	int posa, posb;
};


struct NL2 : public Unit    
{	
	int sizea,sizeb;
	//float * buf; //crossterm data instructions
	float * mema, *memb;	
	int posa, posb;
};


//initial draft, on single blocks only, would need to expand as multiblock for better spectral approximation (though with greater delay)
//do not use with block size greater than 64!
struct LPCError : public Unit    
{	
	int p;	//current accuracy - p poles in resynthesis, 1-20 at first allowed
			//float x0;
	float last[64]; 
	float coeff[64]; //LPC filter coeff
					 //float * calc1; //storage for calculations
	
	float R[65];//autocorrelation coeffs;
		float preva[65];
		float a[65];
		
};


struct KmeansToBPSet1 : public Unit    
{	
	int numdatapoints, maxmeans, nummeans;
	float * data;
	float * means, *newmeans;
	int * newmeancount;
	float * bpx, *bpy;
	int numbps;
	
	double mPhase; 
	float mFreqMul, mSpeed;
	
	int newmeanflag, newdataflag;
	
	//for non-random starting/reset data
	float * m_initdata;
	int m_meansindex;
};


struct Instruction : public Unit    
{
	uint32 mBufNum,mBufSize;	
	float * mBuf;
	int bufpos;
	//float lastlastAmp;
	float lastAmp, newAmp;
	int interpsteps, interpnow;
	float prob; 
	//float lastInstruction;
	
	//int duration;
};


//WaveTerrain(bufnum, x, y, xsize, ysize, wrapbehaviour)
struct WaveTerrain : public Unit    
{
	int m_size, m_xsize, m_ysize;	
	float * m_terrain;
};



//also VMScan for 1 dimensional version?
//also VMscanND?  
//VMScan2D(bufnum)
struct VMScan2D : public Unit    
{
	uint32 mBufNum,mBufSize;	
	float * mBuf;
	int bufpos;
	//float lastlastAmp;
	float lastx, lasty, newx, newy;
	int interpsteps, interpnow;
	float prob; 
	
	//instruction set leads to x and y position
};

//also VM for nonlinear filter equations; stability not guaranteed, so various strategies for recovery


extern "C" {  
	
	void SortBuf_next_k(SortBuf *unit, int inNumSamples);
	void SortBuf_Ctor(SortBuf* unit);
	//void SortBuf_Dtor(SortBuf* unit);
	
	void GravityGrid_next_k(GravityGrid *unit, int inNumSamples);
	void GravityGrid_Ctor(GravityGrid* unit);
	
	void GravityGrid2_next_k(GravityGrid2 *unit, int inNumSamples);
	void GravityGrid2_Ctor(GravityGrid2* unit);
	
	void Breakcore_next_k(Breakcore *unit, int inNumSamples);
	void Breakcore_Ctor(Breakcore* unit);
	
	void Max_next(Max *unit, int inNumSamples);
	void Max_Ctor(Max* unit);
	void Max_Dtor(Max* unit);
	
	void PrintVal_next(PrintVal *unit, int inNumSamples);
	void PrintVal_Ctor(PrintVal* unit);
	
	void EnvDetect_next(EnvDetect *unit, int inNumSamples);
	void EnvDetect_Ctor(EnvDetect* unit);
	
	void FitzHughNagumo_next_k(FitzHughNagumo *unit, int inNumSamples);
	void FitzHughNagumo_Ctor(FitzHughNagumo* unit);
	
	void DoubleWell_next_k(DoubleWell *unit, int inNumSamples);
	void DoubleWell_Ctor(DoubleWell* unit);
	
	void DoubleWell2_next_k(DoubleWell2 *unit, int inNumSamples);
	void DoubleWell2_Ctor(DoubleWell2* unit);
	
	void DoubleWell3_next_k(DoubleWell3 *unit, int inNumSamples);
	void DoubleWell3_Ctor(DoubleWell3* unit);
	
	void WeaklyNonlinear_next_k(WeaklyNonlinear *unit, int inNumSamples);
	void WeaklyNonlinear_Ctor(WeaklyNonlinear* unit);
	
	void WeaklyNonlinear2_next_k(WeaklyNonlinear2 *unit, int inNumSamples);
	void WeaklyNonlinear2_Ctor(WeaklyNonlinear2* unit);
	
	void TermanWang_next_k(TermanWang *unit, int inNumSamples);
	void TermanWang_Ctor(TermanWang *unit);

	//arbitrary potential well function polynomial leads to buffer coefficients/powers of up to order 10 passed in
	//void PotentialWell_next_a(PotentialWell *unit, int inNumSamples);
	//void PotentialWell_Ctor(PotentialWell* unit);
	
	void LTI_next_a(LTI *unit, int inNumSamples);
	void LTI_Ctor(LTI* unit);
	void LTI_Dtor(LTI* unit);
	
	void NL_next_a(NL *unit, int inNumSamples);
	void NL_Ctor(NL* unit);
	void NL_Dtor(NL* unit);
	
	void NL2_next_a(NL2 *unit, int inNumSamples);
	void NL2_Ctor(NL2* unit);
	void NL2_Dtor(NL2* unit);
	
	void LPCError_next_a(LPCError *unit, int inNumSamples);
	void LPCError_Ctor(LPCError* unit);
	//void LPCError_Dtor(LPCError* unit);
	
	void KmeansToBPSet1_next_a(KmeansToBPSet1 *unit, int inNumSamples);
	void KmeansToBPSet1_Ctor(KmeansToBPSet1* unit);
	void KmeansToBPSet1_Dtor(KmeansToBPSet1* unit);
	void MakeBPSet(KmeansToBPSet1 *unit);
	
	void Instruction_next_a(Instruction *unit, int inNumSamples);
	void Instruction_Ctor(Instruction* unit);
	void readinstruction(Instruction *unit, int command, float param);
	
	void WaveTerrain_next_a(WaveTerrain *unit, int inNumSamples);
	void WaveTerrain_Ctor(WaveTerrain* unit);
	
	void VMScan2D_next_a(VMScan2D *unit, int inNumSamples);
	void VMScan2D_Ctor(VMScan2D* unit);
	void readinstructionVMScan2D(VMScan2D *unit, int command, float param);

}



//include local buffer test in one place
SndBuf * SLUGensGetBuffer(Unit * unit, uint32 bufnum) {
	
	SndBuf *buf;
	World *world = unit->mWorld; 
	
	if (bufnum >= world->mNumSndBufs) {
		int localBufNum = bufnum - world->mNumSndBufs; 
		Graph *parent = unit->mParent; 
		if(localBufNum <= parent->localMaxBufNum) { 
			buf = parent->mLocalSndBufs + localBufNum;
		} else { 
			if(unit->mWorld->mVerbosity > -1){ Print("SLUGens buffer number error: invalid buffer number: %i.\n", bufnum); }
			SETCALC(*ClearUnitOutputs);
			unit->mDone = true; 
			return NULL; 
		}
	} else {
		buf = world->mSndBufs + bufnum; 
	}
	
	return buf;
}





void SortBuf_Ctor( SortBuf* unit ) {
	
	SETCALC(SortBuf_next_k);
	
	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(0);
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) { 
	
	unit->mBufNum=bufnum;
	
	//printf("%d \n",bufnum);			
	///SndBuf *buf = world->mSndBufs + bufnum; 
	
	unit->mBufSize = buf->samples;
	
	unit->mBufCopy= buf->data; //(float*)RTAlloc(unit->mWorld, unit->mBufSize * sizeof(float));
	
	//initialise to copy
	//int i=0;
	//	for(i=0; i<unit->mBufSize;++i) 
	//		unit->mBufCopy[i]=buf->data[i];
	//	
	unit->mSorti=unit->mBufSize - 1;
	unit->mSortj=1;
	unit->mSortdone=0;
	unit->mOutStep=1; //allows one run through before it sorts
	
	}
}

//void SortBuf_Dtor(SortBuf *unit)
//{
//	//RTFree(unit->mWorld, unit->mBufCopy);
//}
	
void SortBuf_next_k( SortBuf *unit, int inNumSamples ) {
	
	float *out = ZOUT(0);
	
	int sortrate= (int)ZIN0(1);
	float reset= (float)ZIN0(2);
	//float freq= (float)ZIN0(3);
	
	//printf("%d \n",unit->mWorld->mBufCounter);			
	
	int outstep=unit->mOutStep;
	
	//output, doing sorts if requested
	int bufsize=unit->mBufSize;
	
	float * data= unit->mBufCopy;
	
	int sorti, sortj, sortdone; 
	float temp;
	sorti=unit->mSorti;
	sortj=unit->mSortj;
	sortdone=unit->mSortdone;
	
	//printf("sortparams %d %d %d \n",sorti,sortj,sortdone);
	
	if(reset>0.5 && (unit->mWorld->mBufCounter>10)) {
		//		//printf("reset!\n");
		reset=0.0;
		//SndBuf *buf = unit->mWorld->mSndBufs + unit->mBufNum; 
		//float *source=buf->data;
		
		//for(int i=0; i<bufsize;++i)
		//	data[i]=source[i];
		
		sorti=(bufsize - 1);
		sortj=1;
		sortdone=0;
	}
	
	
	for (int j=0; j<inNumSamples;++j)
	{
		
		if (outstep ==0) { //SORT
			
			int sorttodo=sortrate;
			
			//printf("sort! %d \n", sorttodo);
			
			while((sorttodo>0) && (sortdone==0)){
				
				if (data[sortj-1] > data[sortj]) {
					temp = data[sortj-1];
					data[sortj-1] = data[sortj];
					data[sortj] = temp;
				}
				
				++sortj;
				
				if(sortj>sorti) {
					--sorti;
					sortj=1;
					if(sorti<0)
					{
						sortdone=1;
						//printf("sort finished %d \n",sortdone);			
						
					}
					
				}
				
				sorttodo--;
			}
			
		}
		
		//printf("outstep %d bufsize %d \n",outstep,bufsize);
		
		//else output
		ZXP(out) = data[outstep];
		
		outstep= (outstep+1)%bufsize;
	}
	
	unit->mSorti=sorti;
	unit->mSortj=sortj;
	unit->mSortdone=sortdone;
	
	unit->mOutStep=outstep;
}


void GravityGrid_Ctor(GravityGrid* unit) {
	int i;
	
	//World *world = unit->mWorld;
    
	for(i=0;i<9;++i) {
		
		unit->x[i]=(i%3)-1;
		
		unit->y[i]=1-(i/3);
		
		//printf("i %d x %f y %f \n",i,unit->x[i],unit->y[i]);
		
	}
	
	unit->velx=0.0;
	unit->vely=0.0;
	unit->posx=0.0;
	unit->posy=0.0;
	
	//for (i=0;i<9;++i)
	//	unit->m_weights[i]= g_weights[i];
	unit->m_weights=NULL; 
	
	//must create int because have -1 as indicator of no buffer! 
	int bufnum= (int)ZIN0(4);
	//uint32 bufnum = (uint32)ZIN0(4);
	
	if (bufnum>=0) {

	SndBuf * buf= SLUGensGetBuffer(unit, (uint32)bufnum);
	
	if (buf) {
	
		if(buf->samples==9) {
			unit->m_weights= buf->data;
			
		}
		
		}
		else
		{
		unit->mDone = true;
		}
	
	//if (!(bufnum > world->mNumSndBufs || bufnum<0)) {
//		SndBuf *buf = world->mSndBufs + bufnum; 
//		
//		if(buf->samples==9) {
//			unit->m_weights= buf->data;
//			
//		}
	
	}
	
	SETCALC(GravityGrid_next_k);
	
}


//can't have independent x and y, causes trouble- zero has a pusher at it with infinite size! 
//must use euclidean distances
void GravityGrid_next_k(GravityGrid *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	float accelx,accely, xdiff,ydiff, rdiff; //hyp;
	
	int reset= (int)ZIN0(0);
	float rate= (float)ZIN0(1);
	
	float velx, vely, posx, posy; 
	
	velx= unit->velx;
	vely=unit->vely;
	posx=unit->posx;
	posy=unit->posy;
	
	if(reset) {
		//RGen& rgen = *unit->mParent->mRGen;
		
		//printf("reset! \n");
		
		posx= (float)ZIN0(2); //0.8*(2*rgen.frand() - 1.0); //never right near edge
		posy= (float)ZIN0(3); //0.8*(2*rgen.frand() - 1.0);
		
		velx=0.0;
		vely=0.0;
		
		if (posx>0.99) posx=0.99;
		if (posx<-0.99) posx=-0.99;
		if (posy<-0.99) posy=-0.99;
		if (posy>0.99) posy=0.99;
		
		
	}
	
	float *x,*y, *w;
	
	x=unit->x;
	y=unit->y;
	w=unit->m_weights;
	
	if (w) {
		for (int j=0; j<inNumSamples;++j) {
			
			accelx=0.0;
			accely=0.0;
			
			//could use this to decide on run function but can't be bothered, negligible on loop performance and 
			//keeps code in one place for edits
			
			for(int i=0;i<9;++i) {
				
				if(i!=4) {
					
					xdiff= ((posx)-(x[i]));
					ydiff= ((y[i])-(posy));
					
					//hyp= 1.0/sqrt(xdiff*xdiff+ydiff*ydiff);
					
					rdiff= w[i]*0.0001*sqrt(xdiff*xdiff+ydiff*ydiff); //(0.0001/(xdiff*xdiff+ydiff*ydiff));
					
					accelx+= rdiff*xdiff; //rdiff*(xdiff*hyp);
					
					accely+= rdiff*ydiff; //rdiff*(ydiff*hyp);
					
				}
				
			}
			
			
			velx=velx+accelx;
			vely=vely+accely;
			
			posx=posx+(rate*(velx));
			posy=posy+(rate*(vely));
			
			//assumes fmod works correctly for negative values
			if ((posx>1.0) || (posx<-1.0)) posx=fabs(fmod((posx-1.0),4.0)-2.0)-1.0;
			if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
			
			//correction: (better with the distorted version above!)
			//if ((posx>1.0) || (posx<-1.0)) posx=sc_fold(posx, -1.0f, 1.0f);
			//if ((posy>1.0) || (posy<-1.0)) posy=sc_fold(posy, -1.0f, 1.0f);
			
			
			
			
			//printf("%f %f %f %f %f %f %f \n",accelx, accely, unit->velx,unit->vely,unit->posx,unit->posy, unit->posy*unit->posy+ unit->posx*unit->posx);
			
			float sign; 
			if (fabs(posx) < 0.0000001) sign= 1;
			else
				sign= (posx/fabs(posx));
			
			ZXP(out) = sign*0.5*(posy*posy+ posx*posx);
			
			//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
		}
	}
	else {
		
		for (int j=0; j<inNumSamples;++j) {
			
			accelx=0.0;
			accely=0.0;
			
			//could use this to decide on run function but can't be bothered, negligible on loop performance and 
			//keeps code in one place for edits
			
			for(int i=0;i<9;++i) {
				
				if(i!=4) {
					
					xdiff= ((posx)-(x[i]));
					ydiff= ((y[i])-(posy));
					
					rdiff= 0.0001*sqrt(xdiff*xdiff+ydiff*ydiff);
					accelx+= rdiff*xdiff; 
					accely+= rdiff*ydiff; 
					
				}
				
			}
			
			velx=velx+accelx;
			vely=vely+accely;
			
			posx=posx+(rate*(velx));
			posy=posy+(rate*(vely));
			
			//assumes fmod works correctly for negative values
			if ((posx>1.0) || (posx<-1.0)) posx=fabs(fmod((posx-1.0),4.0)-2.0)-1.0;
			if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
			
				
			//if ((posx>1.0) || (posx<-1.0)) posx=sc_fold(posx, -1.0f, 1.0f);
			//if ((posy>1.0) || (posy<-1.0)) posy=sc_fold(posy, -1.0f, 1.0f);
			
			
			//printf("%f %f %f %f %f %f %f \n",accelx, accely, unit->velx,unit->vely,unit->posx,unit->posy, unit->posy*unit->posy+ unit->posx*unit->posx);
			
			
			//this also gives a discontinuity!
			float sign; 
			if (fabs(posx) < 0.0000001) sign= 1;
			else
				sign= (posx/fabs(posx));
			
			ZXP(out) = sign*0.5*(posy*posy+ posx*posx);
			
			//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
		}
		
	}
	
	
	unit->velx=velx;
	unit->vely=vely;
	unit->posx=posx;
	unit->posy=posy;
	
	
}



void GravityGrid2_Ctor(GravityGrid2* unit) {
	//int i;
	
	//World *world = unit->mWorld;
    
	//get buffer which contains initial number of masses (max of 25), positions and weights
	
	//uint32 bufnum = (uint32)ZIN0(4);
	
	//int num;
	int bufnum= (int)ZIN0(4);
	//uint32 bufnum = (uint32)ZIN0(4);
	
	if (bufnum>=0) {
	
	SndBuf * snd= SLUGensGetBuffer(unit,(int)bufnum);
	
	if (snd) {
	
	//if (!(bufnum > world->mNumSndBufs || bufnum<0)) {
		//SndBuf *snd = world->mSndBufs + bufnum; 
		float *buf = snd->data;
		
		//num= (int)(buf[0]+0.001f);
		//unit->nummasses = num;
		
		unit->m_weights= buf;
		
		//for (i=0; i<num; ++i) {
//		
//		int pos= i*3+1;
//		
//		unit->x[i]=buf[pos];
//		unit->y[i]=buf[pos+1];
//		unit->m_weights[i]=buf[pos+2]; 
//		}
		
	} else
	{unit->mDone=false;}
	
	}

	unit->velx=0.0;
	unit->vely=0.0;
	unit->posx=0.0;
	unit->posy=0.0;
	
	SETCALC(GravityGrid2_next_k);
	
}

float gg_lookupsin[100];
float gg_lookupcos[100];


//can't have independent x and y, causes trouble- zero has a pusher at it with infinite size! 
//must use euclidean distances
void GravityGrid2_next_k(GravityGrid2 *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	float accelx,accely, xdiff,ydiff, rdiff, hyp, theta;
	int lookupindex;
	float tmp;
	
	int reset= (int)ZIN0(0);
	float rate= (float)ZIN0(1);
	
	float velx, vely, posx, posy; 
	
	velx= unit->velx;
	vely=unit->vely;
	posx=unit->posx;
	posy=unit->posy;
	
	//reset resets velocity and position
	if(reset) {
		//RGen& rgen = *unit->mParent->mRGen;
		
		//printf("reset! \n");
		
		posx= (float)ZIN0(2); //0.8*(2*rgen.frand() - 1.0); //never right near edge
		posy= (float)ZIN0(3); //0.8*(2*rgen.frand() - 1.0);
		
		velx=0.0;
		vely=0.0;
		
		if (posx>0.99) posx=0.99;
		if (posx<-0.99) posx=-0.99;
		if (posy<-0.99) posy=-0.99;
		if (posy>0.99) posy=0.99;
		
	}
	
	//float *x,*y, 
	float *w;
	
	//x=unit->x;
	//y=unit->y;
	w=unit->m_weights;
	
	int num= (int)(w[0]+0.001f);

		for (int j=0; j<inNumSamples;++j) {
			
			accelx=0.0;
			accely=0.0;
			
			//could use this to decide on run function but can't be bothered, negligible on loop performance and 
			//keeps code in one place for edits
			
			for(int i=0;i<num;++i) {
				
				int index= i*3;
		
					//xdiff= ((posx)-(x[i]));
					//ydiff= ((y[i])-(posy));
					
					xdiff= ((posx)-(w[index]));
					ydiff= ((w[index+1])-(posy));
					
					
					if((xdiff < 0.01) && (xdiff>(-0.00001))) xdiff= 0.01;
					if((ydiff < 0.01) && (ydiff>(-0.00001))) ydiff= 0.01;
					if((xdiff > (-0.01)) && (xdiff<(0.0))) xdiff= -0.01;
					if((ydiff > (-0.01)) && (ydiff<(0.0))) ydiff= -0.01;
					
					theta= atan2(ydiff, xdiff);
					//result is between -pi and pi
					
					lookupindex= (int)(((theta/twopi)*99.999)+50); //gives index into lookup tables
					
					//lookup tables for cos and sin
					
					//hyp= 1.0/sqrt(xdiff*xdiff+ydiff*ydiff);
					
					hyp = (xdiff*xdiff+ydiff*ydiff);
					
					//if(hyp < 0.01) hyp =0.01;
					
					//need lookup table
					
					//force of gravity
					//w[i]
					rdiff= w[index+2]*0.0001*(1.0/hyp); //*sqrt(xdiff*xdiff+ydiff*ydiff); //(0.0001/(xdiff*xdiff+ydiff*ydiff));
					
					tmp= rdiff*(gg_lookupcos[lookupindex]);
					
					accelx+= tmp; //rdiff*(xdiff*hyp);
					
					tmp= rdiff*(gg_lookupsin[lookupindex]);
					
					accely+= tmp; //rdiff*ydiff; //rdiff*(ydiff*hyp);
					
			}
			
			
			velx=velx+accelx;
			vely=vely+accely;
			
			//constraints on vel to avoid runaway speeds
			if ((velx>1.0) || (velx<-1.0)) velx=sc_fold(velx, -1.0f, 1.0f);
			if ((vely>1.0) || (vely<-1.0)) vely=sc_fold(vely, -1.0f, 1.0f);
			
			posx=posx+(rate*(velx));
			posy=posy+(rate*(vely));
			
			//WRONG assumes fmod works correctly for negative values
			//if ((posx>1.0) || (posx<-1.0)) posx=fabs(fmod((posx-1.0),4.0)-2.0)-1.0;
			//if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
			
			//correction: (better with the distorted version above!)
			if ((posx>1.0) || (posx<-1.0)) posx=sc_fold(posx, -1.0f, 1.0f);
			if ((posy>1.0) || (posy<-1.0)) posy=sc_fold(posy, -1.0f, 1.0f);
			
			//could also be wrap
			
			//printf("%f %f %f %f %f %f %f \n",accelx, accely, unit->velx,unit->vely,unit->posx,unit->posy, unit->posy*unit->posy+ unit->posx*unit->posx);
			
			
			//removed because discontinuous
			//float sign; 
//			if (fabs(posx) < 0.0000001) sign= 1;
//			else
//				sign= (posx/fabs(posx));
//			
			ZXP(out) = (posy*posy+ posx*posx)*2.0f-1.0f; //posx; //sign*0.5*(posy*posy+ posx*posx);
			
			//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
		}
	
	
	unit->velx=velx;
	unit->vely=vely;
	unit->posx=posx;
	unit->posy=posy;
	
	
}





void Breakcore_Ctor( Breakcore* unit ) {
	
	SETCALC(Breakcore_next_k);
	
	World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(0);
	if (bufnum >= world->mNumSndBufs) bufnum = 0;
	unit->mBufNum=bufnum;
	
	//printf("Breakcore activated with bufnum %d \n",bufnum);			
	
	SndBuf *buf = world->mSndBufs + bufnum; 
	
	unit->mBufSize = buf->samples;
	
	unit->mBuf = buf->data;
	
	unit->captureon= 0;
	unit->captureneeds= 1000;
	unit->capturepos= 0;
	unit->repeatpos=0;
	unit->m_prevtrig=0;
	
	//unit->duration=1000;
}


//bufnum, 0 capturein 1, capturetrigger 2, duration of repeat 3, ampdropout 4

void  Breakcore_next_k(  Breakcore *unit, int inNumSamples ) {
	
	float *out = ZOUT(0);
	
	float curtrig= ZIN0(2);
	//int duration= unit->duration; //(int)ZIN0(3);
	//float ampdropout= (float)ZIN0(3);
	float * in = ZIN(1);
	
	//printf("duration %d curtrig %f\n",duration, curtrig);
	
	
	//int bufsize=unit->mBufSize;
	
	float * data= unit->mBuf;
	   
	
	
	if (unit->m_prevtrig <= 0.f && curtrig > 0.f){
		
		//printf("capture \n");
		
		unit->captureon=1;
		unit->captureneeds=(int)ZIN0(3);
		
		//unit->duration=(int)ZIN0(3);
		//duration= unit->duration; 
		//unit->captureneeds=duration; //can't be less than 64  
		unit->capturepos=0;
		unit->repeatpos=0;
	} 
	
	int captureon,captureneeds,repeatpos,capturepos;
	
	captureon= unit->captureon;
	captureneeds= unit->captureneeds;
	capturepos= unit->capturepos;
	repeatpos= unit->repeatpos;
	
	for (int j=0; j<inNumSamples;++j)
	{
		
		if(captureon==1) {
			
			data[capturepos]=in[j];
			
			capturepos++;
			
			if(capturepos==captureneeds) {
				unit->capturepos=0;
				unit->captureon=0;
				captureon=0;
			}
		};
		
		
		ZXP(out)=data[repeatpos];
		
		repeatpos= (repeatpos+1)%captureneeds;
		
	}
	
	unit->capturepos= capturepos;
	unit->repeatpos= repeatpos;
	
	unit->m_prevtrig = curtrig;
	
}



void Max_Ctor( Max* unit ) {
	
	SETCALC(Max_next);
	
	int msamp= (int) ZIN0(1);
	//integer division
	unit->m_blocks= sc_max(msamp/(unit->mWorld->mFullRate.mBufLength),1);
	
	//printf("%d \n",unit->m_blocks);
	
	unit->m_blockmaxes= (float*)RTAlloc(unit->mWorld, unit->m_blocks * sizeof(float));
	//initialise to zeroes
	for(int i=0; i<unit->m_blocks; ++i)
		unit->m_blockmaxes[i]=0.f;
	
	unit->m_counter=0;
	unit->m_last=unit->m_blocks-1;   
	unit->m_max=0.0;
	
	ZOUT0(0) = ZIN0(0);
	
}

void Max_Dtor(Max *unit) {
	RTFree(unit->mWorld, unit->m_blockmaxes);
}

void Max_next( Max *unit, int inNumSamples ) {
	
	int j;
	float *in = ZIN(0);
	//float *out = ZOUT(0);
	float max=0;
	
	//only to be used at .kr
	//printf("samp to calc %d", inNumSamples);
	
	int ksamps = unit->mWorld->mFullRate.mBufLength;
	
	//find max this block
	for(j=0; j<ksamps; ++j) {
		float next= fabs(ZXP(in)); 
		//printf("next %f\t",next);
		if(next>max) max=next;
	}
	
	//printf("max %f  unit->m_max  %f \t",max, unit->m_max);
	
	//now check max over last m_blocks
	
	if(max>(unit->m_max)) {
		
		unit->m_max=max;
		unit->m_last=unit->m_counter;
		unit->m_blockmaxes[unit->m_counter]=max;
	}
	else {
		
		unit->m_blockmaxes[unit->m_counter]=max;
		
		//if max being lost
		if(unit->m_counter==unit->m_last) {//calc new max position
			max=0;
			int last=0;
			
			for(j=0; j<unit->m_blocks; ++j){
				float test= unit->m_blockmaxes[j]; 
				
				if(test>max) {max=test; last=j;}
			}
			
			unit->m_max=max;
			unit->m_last=last;
			
		}
		
	}
	
	max=unit->m_max;
	
	unit->m_counter=(unit->m_counter+1)%(unit->m_blocks);
	
	//control rate, just one out value
	//output max
	//for(j=0; j<inNumSamples; ++j) {
	ZOUT0(0)=max; 
	//}
	
}

void PrintVal_Ctor(PrintVal* unit) {
	SETCALC(PrintVal_next);
	
	unit->m_block= (int) ZIN0(1);
	
	unit->m_id= (int) ZIN0(2);
	
	unit->m_count=0;
}


void PrintVal_next(PrintVal *unit, int inNumSamples) {
	
	if(unit->m_count==0) {printf("%d  %f\n",unit->m_id,ZIN0(0));}
	
	unit->m_count=(unit->m_count+1)%(unit->m_block);
}


void EnvDetect_Ctor(EnvDetect* unit) {
	SETCALC(EnvDetect_next);
	
	//	unit->m_atk= (float) exp(-1/(SAMPLERATE*ZIN0(1)));
	//	
	//	unit->m_rel= (float) exp(-1/(SAMPLERATE*ZIN0(2)));
	//	
	//printf("atk %f rel %f\t",unit->m_atk,unit->m_rel);
	
	unit->m_env=0.0;
}


void EnvDetect_next(EnvDetect *unit, int inNumSamples) {
	
	float *in = ZIN(0);
	float *out = ZOUT(0);
	
	//int ksamps = unit->mWorld->mFullRate.mBufLength;
	
	float envelope= unit->m_env;
	//float ga= unit->m_atk;
	//	float gr= unit->m_rel;
	//	
	
	float ga= (float) exp(-1/(SAMPLERATE*ZIN0(1)));
	float gr= (float) exp(-1/(SAMPLERATE*ZIN0(2)));
	
	//find max this block
	for(int j=0; j<inNumSamples; ++j) {
		float next= fabs(ZXP(in)); 
		//printf("next %f\t",next);
		
		if(envelope<next){
			envelope*=ga;
			envelope+= (1-ga)*next;
		}
		else {
			envelope*=gr;
			envelope+= (1-gr)*next;
		}
		
		ZXP(out)= envelope;
		
	}
	
	unit->m_env=envelope;
	
}




void FitzHughNagumo_Ctor(FitzHughNagumo* unit) {
	
	unit->u=0.0;
	unit->w=0.0;
	
	SETCALC(FitzHughNagumo_next_k);
	
}


void FitzHughNagumo_next_k(FitzHughNagumo *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	int reset= (int)ZIN0(0);
	float urate= (float)ZIN0(1);
	float wrate= (float)ZIN0(2);
	float b0= (float)ZIN0(3);
	float b1= (float)ZIN0(4);
	
	float u,w; 
	
	u= unit->u;
	w=unit->w;
	
	if(reset) {
		
		u= (float)ZIN0(5);
		w= (float)ZIN0(6);
	}
	
	for (int j=0; j<inNumSamples;++j) {
		
		//the naive Euler update, could compare Runge-Kutta later
		float dudt= urate*(u-(0.33333*u*u*u)-w);
		float dwdt= wrate*(b0+b1*u-w);
		
		u+=dudt;
		w+=dwdt;
		
		//assumes fmod works correctly for negative values
		if ((u>1.0) || (u<-1.0)) u=fabs(fmod((u-1.0),4.0)-2.0)-1.0;
		//if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
		
		ZXP(out) = u;
		
		//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
	}
	
	unit->u=u;
	unit->w=w;
	
}







void DoubleWell_Ctor(DoubleWell* unit) {
	
	unit->x=0.0;
	unit->y=0.0;
	unit->t=0.0;
	
	SETCALC(DoubleWell_next_k);
	
}


void DoubleWell_next_k(DoubleWell *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	int reset= (int)ZIN0(0);
	float xrate= (float)ZIN0(1);
	float yrate= (float)ZIN0(2);
	float F= (float)ZIN0(3);
	float w= (float)ZIN0(4);
	float delta= (float)ZIN0(5);
	
	float x,y,t; 
	
	x= unit->x;
	y=unit->y;
	t=unit->t;
	
	if(reset) {
		
		x= (float)ZIN0(6);
		y= (float)ZIN0(7);
		t=0;
	}
	
	//Runge Kutta? would require four cos calls
	for (int j=0; j<inNumSamples;++j) {
		
		//the naive Euler update, could compare Runge-Kutta later
		float dxdt= xrate*y;
		
		//could make Fcos term another ar or kr input itself? 
		//float dydt= yrate*(-delta*y+ x - x*x*x + F*cos(w*t));
		
		//Runge-Kutta
		float temp,z,k1,k2,k3,k4;
		temp= x - x*x*x + F*cos(w*t);
		z= y;
		k1= yrate*(-delta*z+ temp);
		z=y+0.5*k1;
		k2= yrate*(-delta*z+ temp);
		z=y+0.5*k2;
		k3= yrate*(-delta*z+ temp);
		z=y+k3;
		k4= yrate*(-delta*z+ temp);
		
		float dydt= 0.166667*(k1+2*k2+2*k3+k4);
		
		t=t+1;
		x+=dxdt;
		y+=dydt;
		
		
		//assumes fmod works correctly for negative values- which it doesn't- this is erroneous code, but kept for backwards compatability and
		//because it can make some curious sounds!
		if ((x>1.0) || (x<-1.0)) x=fabs(fmod((x-1.0),4.0)-2.0)-1.0;
//		//if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
//		
		ZXP(out) = x;
//		
								
		//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
	}
	
	unit->x=x;
	unit->y=y;
	unit->t=t;
}






void DoubleWell2_Ctor(DoubleWell2* unit) {
	
	unit->x=0.0;
	unit->y=0.0;
	unit->t=0.0;
	
	SETCALC(DoubleWell2_next_k);
	
}


void DoubleWell2_next_k(DoubleWell2 *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	int reset= (int)ZIN0(0);
	float xrate= (float)ZIN0(1);
	float yrate= (float)ZIN0(2);
	float F= (float)ZIN0(3);
	float w= (float)ZIN0(4);
	float delta= (float)ZIN0(5);
	
	float x,y,t; 
	
	x= unit->x;
	y=unit->y;
	t=unit->t;
	
	if(reset) {
		
		//printf("reset %d \n",reset);
		
		x= (float)ZIN0(6);
		y= (float)ZIN0(7);
		t=0;
	}
	
	for (int j=0; j<inNumSamples;++j) {
		
		//improved Euler update using trial (information not just from left derivative)
	
		//could make Fcos term another ar or kr input itself? 
		//float dydt= yrate*(-delta*y+ x - x*x*x + F*cos(w*t));
		
		//Improved Euler Method
		float trialx, dy, dytrial, dydt, dxdt, lasty;

		trialx = x+(y*xrate);
		
		dy= F*cos(w*(t*yrate)) + (x) - (x*x*x) - delta*y;
		
		dytrial= F*cos(w*(t*yrate)) + (trialx) - (trialx*trialx*trialx) - delta*y;
		
		dydt= (dy+dytrial)*0.5*yrate;
		
		lasty=y;
		
		y+=dydt;
		
		dxdt=((y+lasty)*0.5)*xrate;
		
		t=t+1;
		x+=dxdt;

		//previous equation was wrong
		if ((x>3.0) || (x<-3.0)) { 
		
		//printf("folding, x %f calc %f \n",x, sc_fold(x, -3.0f, 3.0f));
		x=sc_fold(x, -3.0f, 3.0f);
		
		}
		
		//printf("checkx %f\n",x);
		ZXP(out) = 0.33*x;
		
		//scaling to keep it within bounds
		//if ((x>3.0) || (x<-3.0)) x=fabs(fmod((x-3.0),12.0)-6.0)-3.0;
		
		//ZXP(out) = 0.33*x;
		
		//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
	}
	
	unit->x=x;
	unit->y=y;
	unit->t=t;
}





void DoubleWell3_Ctor(DoubleWell3* unit) {
	
	unit->x=0.0;
	unit->y=0.0;
	unit->t=0.0;
	
	SETCALC(DoubleWell3_next_k);
	
}


void DoubleWell3_next_k(DoubleWell3 *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	int reset= (int)ZIN0(0);
	float rate= (float)ZIN0(1);
	float *forcing= ZIN(2); //audio rate forcing input
	float delta= (float)ZIN0(3);
	
	
	float x,y,t; 
	
	x= unit->x;
	y=unit->y;
	t=unit->t;
	
	if(reset) {
		
		x= (float)ZIN0(4);
		y= (float)ZIN0(5);
		t=0;
	}
	
	//Runge Kutta? would require four cos calls
	for (int j=0; j<inNumSamples;++j) {
		
		//the naive Euler update, could compare Runge-Kutta later
		float dxdt= rate*y;
		
		//could make Fcos term another ar or kr input itself? 
		//float dydt= yrate*(-delta*y+ x - x*x*x + F*cos(w*t));
		
		//Runge-Kutta
		float temp,z,k1,k2,k3,k4;
		temp= x - x*x*x + ZXP(forcing);
		z= y;
		k1= rate*(-delta*z+ temp);
		z=y+0.5*k1;
		k2= rate*(-delta*z+ temp);
		z=y+0.5*k2;
		k3= rate*(-delta*z+ temp);
		z=y+k3;
		k4= rate*(-delta*z+ temp);
		
		float dydt= 0.166667*(k1+2*k2+2*k3+k4);
		
		t=t+1;
		x+=dxdt;
		y+=dydt;
		
		if ((x>3.0) || (x<-3.0)) x=sc_fold(x, -3.0f, 3.0f);
		ZXP(out) = 0.33*x;
						
	}
	
	unit->x=x;
	unit->y=y;
	unit->t=t;
}





void WeaklyNonlinear_Ctor(WeaklyNonlinear* unit) {
	
	unit->x=0.0;
	unit->y=0.0;
	unit->t=0.0;
	
	SETCALC(WeaklyNonlinear_next_k);
	
}


void WeaklyNonlinear_next_k(WeaklyNonlinear *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	float *in = ZIN(0);
	
	int reset= (int)ZIN0(1);
	float xrate= (float)ZIN0(2);
	float yrate= (float)ZIN0(3);
	float w0= (float)ZIN0(4);
	w0=w0*twopi/(SAMPLERATE); //convert frequency in Hertz to angular frequency
	w0=w0*w0; //constant needed for equation
			  //float eta= (float)ZIN0(5);
	
	float x,y,t; 
	
	x= unit->x;
	y=unit->y;
	t=unit->t;
	
	if(reset) {
		
		x= (float)ZIN0(5);
		y= (float)ZIN0(6);
		t=0;
	}
	
	//alpha*(x^xexponent*-beta)*y^yexponent 
	float alpha=(float)ZIN0(7);
	float xexponent=(float)ZIN0(8);
	float beta=(float)ZIN0(9);
	float yexponent=(float)ZIN0(10);
	
	//Runge Kutta? would require four cos calls
	for (int j=0; j<inNumSamples;++j) {
		
		//the naive Euler update
		float dxdt= xrate*y;
		
		float nonlinear= 0.0;
		
		if(alpha>0.000001 || alpha<(-0.000001)) {
			nonlinear= alpha * (pow(x,xexponent)+beta)*(pow(y,yexponent));
		}
		
		float dydt= yrate*(ZXP(in) - w0*x + nonlinear);
		
		//t=t+1;
		x+=dxdt;
		y+=dydt;
		
		//assumes fmod works correctly for negative values
		if ((x>1.0) || (x<-1.0)) x=fabs(fmod((x-1.0),4.0)-2.0)-1.0;
		//if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
		
		ZXP(out) = x;
		
		//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
	}
	
	unit->x=x;
	unit->y=y;
	unit->t=t;
	
}




void WeaklyNonlinear2_Ctor(WeaklyNonlinear2* unit) {
	
	unit->x=0.0;
	unit->y=0.0;
	unit->t=0.0;
	
	SETCALC(WeaklyNonlinear2_next_k);
	
}

void WeaklyNonlinear2_next_k(WeaklyNonlinear2 *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	float *in = ZIN(0);
	
	int reset= (int)ZIN0(1);
	float xrate= (float)ZIN0(2);
	float yrate= (float)ZIN0(3);
	float w0= (float)ZIN0(4);
	w0=w0*twopi/(SAMPLERATE); //convert frequency in Hertz to angular frequency
	w0=w0*w0; //constant needed for equation
			  //float eta= (float)ZIN0(5);
	
	float x,y,t; 
	
	x= unit->x;
	y=unit->y;
	t=unit->t;
	
	if(reset) {
		
		x= (float)ZIN0(5);
		y= (float)ZIN0(6);
		t=0;
	}
	
	//alpha*(x^xexponent*-beta)*y^yexponent 
	float alpha=(float)ZIN0(7);
	float xexponent=(float)ZIN0(8);
	float beta=(float)ZIN0(9);
	float yexponent=(float)ZIN0(10);
	
	//Runge Kutta? would require four cos calls
	for (int j=0; j<inNumSamples;++j) {
		
		//the naive Euler update
		float dxdt= xrate*y;
		
		float nonlinear= 0.0;
		
		if(alpha>0.000001 || alpha<(-0.000001)) {
			nonlinear= alpha * (pow(x,xexponent)+beta)*(pow(y,yexponent));
		}
		
		float dydt= yrate*(ZXP(in) - w0*x + nonlinear);
		
		//t=t+1;
		x+=dxdt;
		y+=dydt;
		
		//assumes fmod works correctly for negative values
		//if ((x>1.0) || (x<-1.0)) x=fabs(fmod((x-1.0),4.0)-2.0)-1.0;
		//if ((posy>1.0) || (posy-1.0)) posy=fabs(fmod((posy-1.0),4.0)-2.0)-1.0;
		
		if ((x>1.0) || (x<-1.0)) x=sc_fold(x, -1.0f, 1.0f);
		ZXP(out) = x;
		
		
		//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
	}
	
	unit->x=x;
	unit->y=y;
	unit->t=t;
	
}





void TermanWang_Ctor(TermanWang* unit) {
	
	unit->x=0.0;
	unit->y=0.0;
	
	SETCALC(TermanWang_next_k);
	
}

void TermanWang_next_k(TermanWang *unit, int inNumSamples) {
	
	float *out = ZOUT(0);
	
	float *in = ZIN(0);
	
	int reset= (int)ZIN0(1);
	float xrate= (float)ZIN0(2);
	float yrate= (float)ZIN0(3);
	float alpha= (float)ZIN0(4);
	float beta= (float)ZIN0(5);
	float eta= (float)ZIN0(6);
	
	float x,y; 
	
	x= unit->x;
	y=unit->y;
	
	if(reset) {
		x= (float)ZIN0(7);
		y= (float)ZIN0(8);
	}
	
	for (int j=0; j<inNumSamples;++j) {
		
		//the naive Euler update
		float dxdt= xrate* ( (3.0*x) + (x*x*x)  -y + (ZXP(in)));  //+ 2 left out since can be returned via input
		
		float dydt= yrate*(eta * ((alpha*(1.0+(tanh(x*beta)))) -y));
		
		x+=dxdt;
		y+=dydt;
		
		if ((x>1.0) || (x<-1.0)) x=sc_fold(x, -1.0f, 1.0f);
		
		//control on y too? 
		
		ZXP(out) = x;
		
		//printf("%f \n",(unit->posx/fabs(unit->posx))*0.5*(unit->posy*unit->posy+ unit->posx*unit->posx));
	}
	
	unit->x=x;
	unit->y=y;
	
}



void LTI_Ctor( LTI* unit ) {
	
	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(1);
	
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	//SndBuf *buf = world->mSndBufs + bufnum; 
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {
	
	unit->sizea = buf->samples;
	
	unit->bufa = buf->data;
	
	bufnum = (uint32)ZIN0(2);
	
	buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {
	
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	//buf = world->mSndBufs + bufnum; 
	
	unit->sizeb = buf->samples;
	
	unit->bufb = buf->data;
	
	unit->mema= (float*)RTAlloc(unit->mWorld, unit->sizea * sizeof(float));
	
	//initialise to zeroes
	for(int i=0; i<unit->sizea; ++i)
		unit->mema[i]=0.f;
	
	unit->posa=0; 
	
	unit->memb= (float*)RTAlloc(unit->mWorld, unit->sizeb * sizeof(float));
	
	//initialise to zeroes
	for(int i=0; i<unit->sizeb; ++i)
		unit->memb[i]=0.f;
	
	unit->posb=0; 
	
	SETCALC(LTI_next_a);
	
	}
	}
	
}

void LTI_Dtor(LTI *unit) {
	RTFree(unit->mWorld, unit->mema);
	RTFree(unit->mWorld, unit->memb);
}

//bufnum, 0 capturein 1, capturetrigger 2, duration of repeat 3, ampdropout 4
void  LTI_next_a(LTI *unit, int inNumSamples ) {
	int i,j, pos;
	
	float total;
	
	float *out = ZOUT(0);
	float *in = ZIN(0);
	
	float * bufa= unit->bufa, *bufb= unit->bufb, *mema= unit->mema,*memb= unit->memb;
	int sizea=unit->sizea, posa= unit->posa;
	int sizeb= unit->sizeb, posb= unit->posb;
	
	for (j=0; j<inNumSamples;++j) {
		total=0.0;
		
		//sum last x inputs
		memb[posb]= ZXP(in);
		
		for (i=0; i<sizeb; ++i) {
			pos= (posb+sizeb-i)%sizeb;
			total+= memb[pos]*bufb[i];
		}
		
		//update x memory
		posb=(posb+1)%sizeb;
		
		//sum last y outputs
		
		for (i=0; i<sizea; ++i) {
			pos= (posa+sizea-i)%sizea;
			
			total+= mema[pos]*bufa[i];
		}
		
		//update y memory
		posa=(posa+1)%sizea;
		mema[posa]= total;
		
		//output total
		ZXP(out) = total;
		
		//printf("%f \n",);
	}
	
	unit->posa=posa;
	unit->posb=posb;
	
}









void NL_Ctor( NL* unit ) {
	
	int i;
	//int maxindex; 
	
	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(1);
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {
	
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	//SndBuf *buf = world->mSndBufs + bufnum; 
	
	if (buf->samples%3!=0) printf("feedback data input format wrong, not multiple of 3 size\n");
	
	unit->numasummands = buf->samples/3;
	
	unit->aindices= (int*)RTAlloc(unit->mWorld, unit->numasummands * sizeof(int));
	
	for(i=0; i<unit->numasummands; ++i)
	unit->aindices[i] = (int)(buf->data[3*i]+0.01); //rounding carefully
		
	unit->sizea = unit->aindices[unit->numasummands-1] + 1; //last index must be maximal
	
	unit->bufa = buf->data;
	
	bufnum = (uint32)ZIN0(2);
	
	buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {
	
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	//buf = world->mSndBufs + bufnum; 
	
	if (buf->samples%3!=0) printf("feedforward data input format wrong, not multiple of 3 size\n");
	
	unit->numbsummands = buf->samples/3;
	
	unit->bindices= (int*)RTAlloc(unit->mWorld, unit->numbsummands * sizeof(int));
	
	for(i=0; i<unit->numbsummands; ++i)
	unit->bindices[i] = (int)(buf->data[3*i]+0.01); //rounding carefully
		
	unit->sizeb = unit->bindices[unit->numbsummands-1] + 1; //last index must be maximal
	
	unit->bufb = buf->data;
	
	//printf("numa %d sizea %d numb %d sizeb %d \n", unit->numasummands, unit->sizea, unit->numbsummands, unit->sizeb);
	
	unit->mema= (float*)RTAlloc(unit->mWorld, unit->sizea * sizeof(float));
	
	//initialise to zeroes
	for(i=0; i<unit->sizea; ++i)
		unit->mema[i]=0.f;
	
	unit->posa=0; 
	
	unit->memb= (float*)RTAlloc(unit->mWorld, unit->sizeb * sizeof(float));
	
	//initialise to zeroes
	for(i=0; i<unit->sizeb; ++i)
		unit->memb[i]=0.f;
	
	unit->posb=0; 
	
	SETCALC(NL_next_a);
	
	}
	
	}
	
}

void NL_Dtor(NL *unit) {
	RTFree(unit->mWorld, unit->mema);
	RTFree(unit->mWorld, unit->memb);
	RTFree(unit->mWorld, unit->aindices);
	RTFree(unit->mWorld, unit->bindices);
}


void  NL_next_a(NL *unit, int inNumSamples ) {
	int i,j, pos;
	
	float total;
	int index;
	float exponent, coefficient; 
	float val; 
	
	float *out = ZOUT(0);
	float *in = ZIN(0);
	
	float guard1 = ZIN0(3);
	float guard2 = ZIN0(4);
	
	float * bufa= unit->bufa, *bufb= unit->bufb, *mema= unit->mema,*memb= unit->memb;
	int * aindices = unit->aindices, *bindices = unit-> bindices;
	int numasummands= unit->numasummands, numbsummands= unit->numbsummands;
	int sizea=unit->sizea, posa= unit->posa;
	int sizeb= unit->sizeb, posb= unit->posb;
	
	for (j=0; j<inNumSamples;++j) {
		total=0.0;
		
		//sum last x inputs
		memb[posb]= ZXP(in);
		
		//CAN'T TAKE POWER OF NEGATIVE NUMBER IF EXPONENT FRACTIONAL!
		for (i=0; i<numbsummands; ++i) {
			
			index= 3*i;
			exponent= bufb[index+2];
			coefficient= bufb[index+1];
			index= bindices[i]; 
			
			pos= (posb+sizeb-index)%sizeb;
			
			val= memb[pos]; 
			
			if ((val < 0.0))
			total+= (pow(fabs(val),exponent))*(-1.0*coefficient);
			else
			total+= (pow(val,exponent))*coefficient;
			
			//printf("index %d totalb %f change %f %f %f \n", index, total,val,exponent,coefficient);
		
			
		}
		
		//update x memory
		posb=(posb+1)%sizeb;
		
		//sum last y outputs
		//printf("post b total! g1 %f %f %f g2 %f %f \n", total, fabs(total), guard1, fabs(total- mema[posa]), guard2);
		
		
		for (i=0; i<numasummands; ++i) {
			
			index= 3*i;
			exponent= bufa[index+2];
			coefficient= bufa[index+1];
			index= aindices[i]; 
			
			pos= (posa+sizea-index)%sizea;
			
			val= mema[pos]; 
			
			if ((val < 0.0))
			total+= (pow(fabs(val),exponent))*(-1.0*coefficient);
			else
			total+= (pow(val,exponent))*coefficient;
			
			
			//printf("index %d totala %f change %f %f %f \n", index, total,val,exponent,coefficient);
		
		}
		
		//printf("post a total! g1 %f %f %f g2 %f %f \n", total, fabs(total), guard1, fabs(total- mema[posa]), guard2);
		
		//check for blow-ups! 
		if ((fabs(total)>guard1) || (fabs(total- mema[posa])>guard2)) {
			
			//printf("blowup! g1 %f %f %f g2 %f %f \n", total, fabs(total), guard1, fabs(total- mema[posa]), guard2);
			for (i=0; i<sizea; ++i) {
				mema[i]=0.0;
			}
			total=0.0;
		}
		
		
		//update y memory
		posa=(posa+1)%sizea;
		mema[posa]= total;
		
		//output total
		ZXP(out) = total;
		
		//printf("%f \n",);
	}
	
	unit->posa=posa;
	unit->posb=posb;
	
}








void NL2_Ctor( NL2* unit ) {
	
	int i;
	//int maxindex; 
	
	
	unit->sizea = (int)(ZIN0(2)+0.01); //last index must be maximal
	
	unit->sizeb = (int)(ZIN0(3)+0.01); //last index must be maximal

	//printf("numa %d sizea %d numb %d sizeb %d \n", unit->numasummands, unit->sizea, unit->numbsummands, unit->sizeb);
	
	unit->mema= (float*)RTAlloc(unit->mWorld, unit->sizea * sizeof(float));
	
	//initialise to zeroes
	for(i=0; i<unit->sizea; ++i)
		unit->mema[i]=0.f;
	
	unit->posa=0; 
	
	unit->memb= (float*)RTAlloc(unit->mWorld, unit->sizeb * sizeof(float));
	
	//initialise to zeroes
	for(i=0; i<unit->sizeb; ++i)
		unit->memb[i]=0.f;
	
	unit->posb=0; 
	
	SETCALC(NL2_next_a);
	
}

void NL2_Dtor(NL2 *unit) {
	RTFree(unit->mWorld, unit->mema);
	RTFree(unit->mWorld, unit->memb);
}


void  NL2_next_a(NL2 *unit, int inNumSamples ) {
	int i,j,k, pos;
	
	float total;
	int index;
	float exponent; //, coefficient; 
	float val; 
	
	float *out = ZOUT(0);
	float *in = ZIN(0);
	
	float guard1 = ZIN0(4);
	float guard2 = ZIN0(5);
	
	//Dynamic buffer checks
	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(1);
	
	SndBuf * sndbuf= SLUGensGetBuffer(unit,bufnum);
	
	if (sndbuf) {
	
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	//SndBuf *sndbuf = world->mSndBufs + bufnum; 
	
	float *buf = sndbuf->data;
	//int bufsize= sndbuf->samples;
	int bufdone = 0;
	int numcrossterms;
	
	float *mema= unit->mema,*memb= unit->memb;

	int sizea=unit->sizea, posa= unit->posa;
	int sizeb= unit->sizeb, posb= unit->posb;
	
	for (j=0; j<inNumSamples;++j) {
		total=0.0;
		
		//sum last x inputs
		memb[posb]= ZXP(in);
		
		//bool check= true; 
		
		numcrossterms = (int)buf[0]; 
		
		bufdone= 1; 
		
		for (k= 0; k<numcrossterms; ++k) {
		
		float product = buf[bufdone]; 
		
		int numbinproduct = (int)buf[bufdone+1];
		
		bufdone +=2; 
		
		for (i=0; i<numbinproduct; ++i) {
		
			index= (int)buf[bufdone];
			//coefficient= buf[bufdone+1];
			exponent= buf[bufdone+1];
	
			pos= (posb+sizeb-index)%sizeb;
			
			val= memb[pos]; 
			
			if ((val < 0.0))
			product*= (pow(fabs(val),exponent))*(-1.0); //(-1.0*coefficient);
			else
			product*= (pow(val,exponent)); //*coefficient;

			bufdone += 2; 
		
		}
		
		int numainproduct = (int)buf[bufdone];
		
		bufdone +=1; 
		
		for (i=0; i<numainproduct; ++i) {
		
			index= (int)buf[bufdone];
			//coefficient= buf[bufdone+1];
			exponent= buf[bufdone+1];
	
			pos= (posa+sizea-index)%sizea;
			
			val= mema[pos]; 
			
			if ((val < 0.0))
			product*= (pow(fabs(val),exponent))*(-1.0); //(-1.0*coefficient);
			else
			product*= (pow(val,exponent)); //*coefficient;

			bufdone += 2; 
		
		}
		
		total += product; 
			
		};
		
		
		
		//update x memory
		posb=(posb+1)%sizeb;
		
		//sum last y outputs
		//printf("post b total! g1 %f %f %f g2 %f %f \n", total, fabs(total), guard1, fabs(total- mema[posa]), guard2);
		
		//check for blow-ups! 
		if ((fabs(total)>guard1) || (fabs(total- mema[posa])>guard2)) {
			
			//printf("blowup! g1 %f %f %f g2 %f %f \n", total, fabs(total), guard1, fabs(total- mema[posa]), guard2);
			for (i=0; i<sizea; ++i) {
				mema[i]=0.0;
			}
			total=0.0;
		}
		
		//update y memory
		posa=(posa+1)%sizea;
		mema[posa]= total;
		
		//output total
		ZXP(out) = total;
		
		//printf("%f \n",);
	}
	
	unit->posa=posa;
	unit->posb=posb;
	
	}
}




void LPCError_Ctor(LPCError* unit) {
	
	//unit->x0= 0;
	unit->p=1;
	
	for (int i=0; i<64;++i) {
		unit->coeff[i]=0.0;
		unit->last[i]=0.0;
	}
	
	printf("SETUP LPCError \n");
	
	SETCALC(LPCError_next_a);
	
}


//efficiency, place as globals the storage arrays for calculations
void LPCError_next_a(LPCError *unit, int inNumSamples) {
	
	int i,j;
	float sum; //, previous[20];
	int prevcount, pos;
	
	float *out = OUT(0);
	
	float *in = IN(0);
	
	//	for (j=0;j<64;++j)
	//	printf("%f ", in[j]);
	//	
	//	printf("\n");
	//	
	//	for (j=0;j<64;++j)
	//	printf("%f ", out[j]);
	//	
	//	printf("\n");
	
	prevcount=0;
	//	
	//	for(i=0; i<20; ++i) {
	//		previous[i]=0.0;
	//	}
	//	
	//	previous[0]= unit->x0;
	//	
	float * coeff=unit->coeff;
	float * last=unit->last;
	
	int p= unit->p;
	
	//printf("first p? %d",p);
	
	for (j=0; j<p;++j) {
		out[j]= last[j];
	}
	
	prevcount=p-1;
	
	//output first, using coeffs from last time, which is why this is a Non Alias Unit
	for (j=p; j<inNumSamples;++j) {
		
		sum=0.0;
		
		for(i=0; i<p; ++i) {
			pos= (prevcount+p-i)%p;
			
			//where is pos used?
			sum += last[pos]*coeff[i];
		}
		
		prevcount=(prevcount+1)%p;
		
		last[prevcount]=(-sum);
		
		//ZXP(out)=
		out[j]= (-sum);
	}
	
	//unit->x0= in[1];
	
	
	
	p= (int)ZIN0(1); //user selected p value from 1 to 64
	
	//safety
	if(p<1) p=1;
	if(p>64) p=64;
	
	//printf("p? %d",p);
	
	unit->p=p;
	
	//ready for next time
	for (j=0; j<p;++j) {
		last[j]= in[j];
	}
	
	
	//calculate new LPC filter coefficients following (Makhoul 1975) autocorrelation, deterministic signal, Durbin iterative matrix solver
	
	//float R[21];//autocorrelation coeffs;
	//float preva[21];
	//float a[21];
	float E, k; 
	
	float * R= unit->R; 
	float * preva=unit->preva;
	float * a= unit->a; 
	
	for(i=0; i<=p; ++i) {
		sum=0.0;
		
		for (j=0; j<= 63-i; ++j)
			sum+= in[j]*in[j+i];
		
		R[i]=sum;
	}
	
	E= R[0];
	k=0;
	
	for(i=0; i<=(p+1); ++i) {
		a[i]=0.0;
		preva[i]=0.0; //CORRECTION preva[j]=0.0;
	}
	
	if(E<0.0000001) {
		
		//zero power, so zero all coeff
		
		for (i=0; i<p;++i)
			unit->coeff[i]=0.0;
		
		//printf("early return %f\n", E);	
		return;
		
	};
	
	for(i=1; i<=p; ++i) {
		
		sum=0.0;
		
		for(j=1;j<i;++j)
			sum+= a[j]*R[i-j];
		
		k=(-1.0*(R[i]+sum))/E; 
		
		a[i]=k;
		
		for(j=1;j<=(i-1);++j)
			a[j]=preva[j]+(k*preva[i-j]);
		
		for(j=1;j<=i;++j)
			preva[j]=a[j];
		
		E= (1-k*k)*E;
		
	}
	
	
	//solution is the final set of a
	for(i=0; i<p; ++i) {
		coeff[p-1-i]=a[i+1];
		
		//printf("a %f R %f",a[i+1], R[i]);	
	}
	
	//	printf("\n");
	//	
	//	for (j=0;j<64;++j)
	//	printf("%f ", in[j]);
	//	
	//	printf("\n");
	//	
	//	for (j=0;j<64;++j)
	//	printf("%f ", out[j]);
	//	
	//	printf("\n\n");
	
	
}


void KmeansToBPSet1_Ctor(KmeansToBPSet1* unit) {
	
	unit->mFreqMul = unit->mRate->mSampleDur;
	unit->mPhase = 1.f;	//should immediately decide on new target 
	unit->mSpeed = 100*unit->mFreqMul; 
	
	unit->numdatapoints= (int) ZIN0(1);	
	unit->maxmeans= (int) ZIN0(2);	
	
		//World *world = unit->mWorld;
    
	//get buffer containing data
	uint32 bufnum = (uint32)(ZIN0(7)+0.001);
	
	unit->m_initdata=NULL;
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {
	
	//if (!(bufnum > world->mNumSndBufs || bufnum<0)) {
	//	SndBuf *buf = world->mSndBufs + bufnum; 
		
		if(buf->samples == ((2*unit->numdatapoints) + (2*unit->maxmeans))) {
		//must be of the form 2*numdatapoints then 2*maxmeans
		unit->m_initdata=buf->data;
		unit->m_meansindex= 2*unit->numdatapoints;
		}
	} else
	//don't cancel performance of UGen, but pointer to buf was NULL
	{unit->mDone = false; }
	
	unit->data= (float*)RTAlloc(unit->mWorld, 2*unit->numdatapoints * sizeof(float));
	unit->means= (float*)RTAlloc(unit->mWorld, 2*unit->maxmeans * sizeof(float));
	unit->newmeans= (float*)RTAlloc(unit->mWorld, 2*unit->maxmeans * sizeof(float));
	unit->newmeancount= (int*)RTAlloc(unit->mWorld, unit->maxmeans * sizeof(int));
	
	//two extra for guard elements, (0,0) and (1,0)
	unit->bpx= (float*)RTAlloc(unit->mWorld, (unit->maxmeans+2) * sizeof(float));
	unit->bpy= (float*)RTAlloc(unit->mWorld, (unit->maxmeans+2) * sizeof(float));
	
	//initialise random data
	RGen& rgen = *unit->mParent->mRGen;
	int i=0;
	
	
	
	if(unit->m_initdata) {
				
				for(i=0; i<(2*unit->numdatapoints);++i) {
					unit->data[i]=unit->m_initdata[i];
				}
				
				for(i=0; i<(2*unit->maxmeans);++i) {
					unit->means[i]=unit->m_initdata[unit->m_meansindex+i];
				}
				
				} else {
				
				for(i=0; i<(2*unit->numdatapoints);++i) {
					unit->data[i]=rgen.frand();//x
								   //unit->data[2*i+1]=rgen.frand(); //y 2*rgen.frand() - 1.0; easier to keep these calcs in 0.0-1.0 range
				}		
				
				for(i=0; i<(2*unit->maxmeans);++i) {
					unit->means[i]=rgen.frand();
				}	
				
				}
	
	
	for(i=0; i<(unit->maxmeans);++i) {
		//unit->means[2*i]=rgen.frand();
		//unit->means[2*i+1]=rgen.frand();
		unit->newmeans[2*i]=0.0;
		unit->newmeans[2*i+1]=0.0;
		unit->newmeancount[i]=0;
	}		
	
	unit->nummeans=1;
	unit->numbps=2;
	
	unit->bpx[0]=0.0; unit->bpy[0]=0.0;
	unit->bpx[1]=1.0; unit->bpy[1]=0.0;
	
	//MakeBPSet(unit);
	
	unit->newmeanflag=0;
	unit->newdataflag=0;
	
	SETCALC(KmeansToBPSet1_next_a);
}

void KmeansToBPSet1_Dtor(KmeansToBPSet1* unit) {
	
	RTFree(unit->mWorld, unit->data);
	RTFree(unit->mWorld, unit->means);
	RTFree(unit->mWorld, unit->newmeans);
	RTFree(unit->mWorld, unit->newmeancount);
	RTFree(unit->mWorld, unit->bpx);
	RTFree(unit->mWorld, unit->bpy);
	
}


//convert current means into a breakpoint set
void MakeBPSet(KmeansToBPSet1 *unit) {
	
	int i;
	
	float * means=unit->means;
	float * bpx=unit->bpx;
	float * bpy=unit->bpy;
	int nummeans=unit->nummeans;
	
	//make final BPs (sort)
	
	//sort by x, count as you go
	
	int numsorted=1;
	float last, minmatch, match, xnow;
	int lastindex;
	
	//first guard
	bpx[0]=0.0; bpy[0]=0.0;
	
	last=0.0;
	
	lastindex=0;
	
	while (lastindex>=0) {
		
		lastindex=-1;
		minmatch=1.0;
		
		for(i=0; i<(nummeans);++i) {
			
			xnow=means[2*i];
			
			if(xnow>last) {
				
				match=xnow-last;
				
				if(match<minmatch) {minmatch=match; lastindex=i;} 
				
			}
			
		}
		
		if(lastindex>=0) {
			
			bpx[numsorted]=means[2*lastindex]; 
			bpy[numsorted]=2*means[2*lastindex+1]-1; //2x-1 to convert [0,1] to [-1,1]
			last=bpx[numsorted];
			numsorted+=1;
		}
		
	}
	
	
	bpx[numsorted]=1.0; bpy[numsorted]=0.0;
	numsorted+=1;
	
	unit->numbps= numsorted;
	
}

void KmeansToBPSet1_next_a(KmeansToBPSet1 *unit, int inNumSamples) {
	
	int i,j,k;
	
	float *out = ZOUT(0);
	
	int newnummeans= (int) ZIN0(3);	
	if (newnummeans> unit->maxmeans) newnummeans= unit->maxmeans;
	if (newnummeans<1) newnummeans= 1;
	
	int nummeans=unit->nummeans;
	
	int newmeanflag=unit->newmeanflag;
	int newdataflag=unit->newdataflag;
	
	if((int)ZIN0(5)) newmeanflag=1;
	if((int)ZIN0(4)) newdataflag=1;
	
	//phase gives proportion for linear interpolation automatically
	double phase = unit->mPhase;
	
	float speed= unit->mSpeed;
	
	RGen& rgen = *unit->mParent->mRGen;
	//linear distribution 0.0 to 1.0 using rgen.frand()
	
	float * data=unit->data;
	float * means=unit->means;
	float * newmeans=unit->newmeans;
	int * newmeancount=unit->newmeancount;
	float * bpx=unit->bpx;
	float * bpy=unit->bpy;
	int numbps= unit->numbps;
	
	int numdata= unit->numdatapoints;
	
	float soft = ZIN0(6);
	
	//have to be careful to avoid making invalid breakpoint sets. 
	if(soft<0.0) soft=0.0;
	if(soft>1.0) soft=1.0;
	
	float soft2= 1.0-soft;
	
	float freq = ZIN0(0);
			 
	//phase duration of a sample
	float minphase=unit->mFreqMul;
			 
	speed= freq*minphase;
	
	for (k=0; k<inNumSamples;++k) { 
		float z;
		
		if (phase >= 1.f) {
			phase -= 1.f;
			
			if (newdataflag) {
				
				if(unit->m_initdata) {
				
				for(i=0; i<(2*numdata);++i) {
					unit->data[i]=unit->m_initdata[i];
				}
				
				} else
				
				for(i=0; i<(2*numdata);++i) {
					unit->data[i]=rgen.frand();
				}	
				
				newdataflag=0;
			}
			
			if (newmeanflag || (newnummeans != nummeans)) {
				
				nummeans= newnummeans;
				unit->nummeans=nummeans;
		
				if(unit->m_initdata) {
				
				for(i=0; i<(2*nummeans);++i) {
					means[i]=unit->m_initdata[unit->m_meansindex+i];
				}
				
				} else {
				
				for(i=0; i<(2*nummeans);++i) {
					means[i]=rgen.frand();
					//means[2*i]=rgen.frand();
					//means[2*i+1]=rgen.frand();
				}		
				
				}
				
				newmeanflag=0;
				
				//make final BPs
				MakeBPSet(unit);
				numbps= unit->numbps;
				
			}
			
			else {  //update means, else stick with new ones first time
				
				for(i=0; i<(nummeans);++i) {
					newmeans[2*i]=0.0;
					newmeans[2*i+1]=0.0;
					newmeancount[i]=0;
				}		
				
				//do calculations
				
				int closest=0;
				float lowscore=1000.0;
				float x,y, dist, tmp;
				
				for(i=0; i<numdata;++i) {
					
					x=data[2*i];
					y=data[2*i+1];
					
					closest=0;
					lowscore=1000.0;
					
					//find closest
					for(j=0; j<nummeans;++j) {
						
						tmp= x-means[2*j];
						dist= y-means[2*j+1]; 
						
						dist=tmp*tmp+dist*dist;
						
						if(dist<lowscore) {
							closest=j;
							lowscore= dist;
						}
						
					}
					
					newmeans[2*closest]+= x;
					newmeans[2*closest+1]+= y;
					
					newmeancount[closest]+=1;
				}	
				
				for(i=0; i<(nummeans);++i) {
					
					if(newmeancount[i]) {
						tmp= 1.0/((float)newmeancount[i]);
						
						means[2*i]=(soft2*means[2*i])+(soft*(newmeans[2*i])*tmp);
						means[2*i+1]=(soft2*means[2*i+1])+(soft*(newmeans[2*i+1])*tmp);
					}
					
				}	
				
				MakeBPSet(unit);
				numbps= unit->numbps;
			}
			
			
			
		} 
		
		//use BPs, find bounding BPs from phase position, linear interpolation		
		
		int ind= 0;
		
		float dist;
		float mindist=1.0;
		
		for(i=1; i<(numbps);++i) {
			
			if(bpx[i]<=phase) {
				dist= phase-bpx[i];
				
				if(dist<mindist) {
					mindist=dist; ind=i;
				}
				
			}
			
		}	
		
		if(ind==(numbps-1)) ind= numbps-2;
		
		float prop= (phase- bpx[ind])/(bpx[ind+1]-bpx[ind]);
		
		//search bpx for enclosing elements
		
		//printf("ind %d bpy1 %f bpy2 %f prop %f\n",ind,bpy[ind],bpy[ind+1],prop);
		
		z = ((1.0-prop)*bpy[ind])+(prop*bpy[ind+1]);
		
		phase +=  speed;
		ZXP(out) = z;
	}
	
	unit->mPhase = phase;
	unit->mSpeed = speed;
	
	unit->newmeanflag=newmeanflag;
	unit->newdataflag=newdataflag;
	unit->nummeans=nummeans;
	
	
	
	
}



void Instruction_Ctor(Instruction* unit) {
	
	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(0);
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {

	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	unit->mBufNum=bufnum;
	
	//SndBuf *buf = world->mSndBufs + bufnum; 
	
	unit->mBufSize = buf->samples;
	
	if(unit->mBufSize%2==1) printf("Not multiple of 2 size buffer \n");			
	
	unit->mBuf= buf->data; 
	
	unit->bufpos=0;
	
	//unit->lastlastAmp=0.0;
	unit->lastAmp=0.0;
	unit->newAmp=0.0;
	unit->interpsteps=10; 
	unit->interpnow=10;
	//unit->lastInstruction=0;
	unit->prob=1.0;
	
	SETCALC(Instruction_next_a);
	}
}


void readinstruction(Instruction *unit, int command, float param){
	
	bool newbreakpoint=false;
	float tmp= unit->newAmp;
	int steps, pos;
	
	RGen& rgen = *unit->mParent->mRGen;
	
	//return early if instruction 8 had lowered probability		
	if (rgen.frand()>unit->prob) {
		unit->prob=1.0;
		return;		
	}
				
	switch (command){
		
		case 0: //interpolate, param is how long for
			
			unit->interpnow=1;
			
			steps= (int)(param*500+0.5); // test larger than 0;
			
			//printf("%d \n",steps);
			
			if (steps<1) steps=1;
				if(steps>5000) steps=5000;
					unit->interpsteps=steps; 
			
			break;
			
		case 1: //new random breakpoint
			
			tmp= param*((2*rgen.frand() - 1.0)); //amp degree from param
			
			newbreakpoint=true;
			
			break;
			
		case 2: //perturb
			
			tmp= (param*(2*rgen.frand() - 1.0))+tmp; //amp degree from param
			
			if(tmp>1.0) tmp=2.0-tmp;
				if(tmp<(-1.0)) tmp=(-2.0)-tmp;
					
					newbreakpoint=true;
			
			break;
		case 3: //invert
			
			tmp= (1-param)*tmp+param*(-tmp); //interpolate to inverse
			newbreakpoint=true;
			
			break;
			
		case 4: //interpolate last two
			
			tmp= ((1-param)*unit->lastAmp)+(param*tmp); //usually 0.5
			newbreakpoint=true;
			
			break;	
			
		case 5: //damp
			
			tmp= (tmp)*param;
			newbreakpoint=true;
			
			break;		
			
		case 6: //damp
			
			tmp= param;
			newbreakpoint=true;
			
			break;		
			
			
			
		case 8: //do next command with probability 
			
			unit->prob= param;
			
			break;		
			
		case 9: //buffer pointer back to start (or to param)
			pos= (int)(param+0.5); // test larger than 0;
			
			if(pos%2==1) --pos;
				
				if(pos<0) pos=0;	
					
					unit->bufpos= pos%(unit->mBufSize);
			
			break;	
			
		default: //if get to here, unknown instruction, step on through buffer is automatic
			
			break;
	}	
	
	
	
	if(newbreakpoint) {
		//unit->lastlastAmp=unit->lastAmp;
		unit->lastAmp=unit->newAmp;
		unit->newAmp=tmp;
	}
	
	
}


void Instruction_next_a(Instruction *unit, int inNumSamples) {
	
	int multicalltest=0;
	
	float *out = ZOUT(0);
	
	//if interpolating, keep interpolating, else read next instruction
	
	float t;
	int command;
	float param;
	int interpnow=unit->interpnow;
	int interpsteps= unit->interpsteps;
	
	bool cancontinue=true;	
	
	for(int j=0; j<inNumSamples; ++j) {
		
		//interpolating, output samples
		if (interpnow<=interpsteps) {
			
			t= (float)interpnow/((float)interpsteps);
			
			ZXP(out)= (1-t)*unit->lastAmp+ (t*unit->newAmp);
			
			interpnow=interpnow+1;
		}
		else //run instructions until new interpolation or unsafe
		{
			
			cancontinue=true;
			
			while (cancontinue && (multicalltest<inNumSamples)) {
				
				command= (int) (unit->mBuf[unit->bufpos]+ 0.5);
				param= unit->mBuf[unit->bufpos+1]; //for parameter control, PLUS UPDATE BELOW
				
				//increment buffer position safely
				unit->bufpos= (unit->bufpos+2)%(unit->mBufSize);
				
				if(unit->bufpos%2==1)
					unit->bufpos= 0; //reset if troublesome movement
				
				readinstruction(unit,command, param);
				
				if (command==0) {
					
					cancontinue=false;
					interpnow=unit->interpnow;
					interpsteps= unit->interpsteps;
				}
				
				++multicalltest; //safety in case you get stuck
			}
			
			//safety, set up interpolation through the remaining samples
			if (multicalltest==inNumSamples) {
				
				interpnow=1;
				interpsteps= inNumSamples-j;
				
			}
			
		}
		
		
		
	}
	
	unit->interpnow=interpnow;
	unit->interpsteps=interpsteps;
	
}




void WaveTerrain_Ctor(WaveTerrain* unit) {
	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(0);
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {

	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	//unit->m_bufnum=bufnum;
	//SndBuf *buf = world->mSndBufs + bufnum; 
	
	unit->m_size = buf->samples;

	unit->m_xsize = (int)(ZIN0(3)+0.0001); //safety on round down
	
	unit->m_ysize = (int)(ZIN0(4)+0.0001);

	if((unit->m_xsize * unit->m_ysize)!= unit->m_size) { printf("size mismatch! \n"); return;}			
	
	unit->m_terrain= buf->data; 
	
	SETCALC(WaveTerrain_next_a);
	}
}
	
void WaveTerrain_next_a(WaveTerrain *unit, int inNumSamples) {

	float * terrain = unit->m_terrain; 
	
	int xsize= unit->m_xsize; 
	int ysize= unit->m_ysize;
	//int size= unit->m_size;
	
	float *xin = IN(1);
	float *yin = IN(2);
	
	float *out = ZOUT(0);
	float x, y, xindex, yindex; 
	int xfloor, yfloor, xnext, ynext;
	float xprop,yprop; 
	
	float vll,vlr,vul,vur;
	
	for (int j=0; j<inNumSamples;++j) {

	x= xin[j]; //0.0 to 1.0
	y= yin[j];

	//safety
	x= sc_wrap(x, 0.0f, 1.0f); 
	y= sc_wrap(y, 0.0f, 1.0f); 
		
	xindex= x*xsize;
	yindex= y*ysize;
	
	//these are guaranteed in range from wrap above give or take floating point error on round down? 
	xfloor= (int)xindex;
	yfloor= (int)yindex;
	
	//these aren't; needs further wrap
	xnext= (xfloor+1)%xsize;  
	ynext= (yfloor+1)%ysize; 
	
	xprop= xindex-xfloor;
	yprop= yindex-yfloor;

	//now have to look up in table and interpolate; linear within the 4 vertices of a square cell for now, cubic over 16 vertices maybe later
	
	//format for terrain should be rows of xsize, indexed from lower left
	vll= terrain[(xsize*yfloor)+ xfloor];
	vlr= terrain[(xsize*yfloor)+ xnext];
	vul= terrain[(xsize*ynext)+ xfloor];
	vur= terrain[(xsize*ynext)+ xnext];
	
	ZXP(out) = (1.0-xprop)*(vll+(yprop*(vul-vll))) + (xprop*(vlr+(yprop*(vur-vlr))));
		
		//printf("%f \n",);
	}
	
}

	
void VMScan2D_Ctor(VMScan2D* unit) {

	//World *world = unit->mWorld;
    
	uint32 bufnum = (uint32)ZIN0(0);
	
	SndBuf * buf= SLUGensGetBuffer(unit,bufnum);
	
	if (buf) {
	
	//if (bufnum >= world->mNumSndBufs) bufnum = 0;
	unit->mBufNum=bufnum;
	
	///SndBuf *buf = world->mSndBufs + bufnum; 
	
	unit->mBufSize = buf->samples;
	
	if(unit->mBufSize%2==1) printf("Not multiple of 2 size buffer \n");			
	
	unit->mBuf= buf->data; 
	
	unit->bufpos=0;
	
	//unit->lastlastAmp=0.0;
	unit->lastx=0.0;
	unit->newx=0.0;
	unit->lasty=0.0;
	unit->newy=0.0;
	unit->interpsteps=10; 
	unit->interpnow=11;
	//unit->lastInstruction=0;
	unit->prob=1.0;
	
	SETCALC(VMScan2D_next_a);
	}
}



void VMScan2D_next_a(VMScan2D *unit, int inNumSamples) {

	int multicalltest=0;
	
	//float *out = ZOUT(0);
	
	float * outx= OUT(0);
	float * outy= OUT(1);
	
	//if interpolating, keep interpolating, else read next instruction
	
	float t;
	int command;
	float param;
	int interpnow=unit->interpnow;
	int interpsteps= unit->interpsteps;
	
	bool cancontinue=true;	
	
	for(int j=0; j<inNumSamples; ++j) {
		
		//interpolating, output samples
		if (interpnow<=interpsteps) {
			
			t= (float)interpnow/((float)interpsteps);
			
			//ZXP(out)= (1-t)*unit->lastAmp+ (t*unit->newAmp);
			
			outx[j]=(1-t)*unit->lastx+ (t*unit->newx); 
			outy[j]=(1-t)*unit->lasty+ (t*unit->newy);  
			
			++interpnow; //=interpnow+1;
		}
		else //run instructions until new interpolation or unsafe
		{
			
			cancontinue=true;
			
			while (cancontinue && (multicalltest<inNumSamples)) {
				
				command= (int) (unit->mBuf[unit->bufpos]+ 0.5);
				param= unit->mBuf[unit->bufpos+1]; //for parameter control, PLUS UPDATE BELOW
				
				//increment buffer position safely
				unit->bufpos= (unit->bufpos+2)%(unit->mBufSize);
				
				if(unit->bufpos%2==1)
					unit->bufpos= 0; //reset if troublesome movement
				
				readinstructionVMScan2D(unit,command, param);
				
				if (command==0) {
					
					cancontinue=false;
					interpnow=unit->interpnow;
					interpsteps= unit->interpsteps;
				}
				
				++multicalltest; //safety in case you get stuck
			}
			
			//safety, set up interpolation through the remaining samples
			if (multicalltest>=inNumSamples) {
				
				interpnow=1;
				interpsteps= inNumSamples-j;
				
			}
			
		}
		
		
		
	}
	
	unit->interpnow=interpnow;
	unit->interpsteps=interpsteps;

}

void readinstructionVMScan2D(VMScan2D *unit, int command, float param) {

	bool newbreakpoint=false;
	float tmpx= unit->newx;
	float tmpy= unit->newy;
	int steps, pos;
	
	RGen& rgen = *unit->mParent->mRGen;
	
	//return early if instruction 8 had lowered probability		
	if (rgen.frand()>unit->prob) {
		unit->prob=1.0;
		return;		
	}
				
	switch (command){
		
		case 0: //interpolate, param is how long for
			
			unit->interpnow=1;
			
			steps= (int)(param*500+0.5); // test larger than 0;
			
			//printf("%d \n",steps);
			
			if (steps<1) steps=1;
			if(steps>5000) steps=5000;
			unit->interpsteps=steps; 
			
			break;
			
		case 1: //new random breakpoint
			
			tmpx= param*(rgen.frand()); //degree from param
			tmpy= param*(rgen.frand());
			
			newbreakpoint=true;
			
			break;
			
		case 2: //perturb
			
			tmpx+= (param*(2*rgen.frand() - 1.0)); // degree from param
			tmpy+= (param*(2*rgen.frand() - 1.0));
			 
			if(tmpx>1.0) tmpx=1.0-tmpx;
				if(tmpx<(0.0)) tmpx= (-tmpx);
				
			if(tmpy>1.0) tmpy=1.0-tmpy;
				if(tmpy<(0.0)) tmpy= (-tmpy);			
					
			newbreakpoint=true;
			
			break;
		case 3: //invert
			
			tmpx += param*(1.0-(2*tmpx));
			tmpy += param*(1.0-(2*tmpy));
			
			newbreakpoint=true;
			
			break;
			
		case 4: //interpolate last two
			
			tmpx = ((1.0-param)*unit->lastx)+(param*tmpx);
			tmpy = ((1.0-param)*unit->lasty)+(param*tmpy);
			
			newbreakpoint=true;
			
			break;	
			
		case 5: //damp
			
			tmpx= (tmpx)*param;
			tmpy= (tmpy)*param;
			
			newbreakpoint=true;
			
			break;		
			
		case 6: //set x
			unit->lastx=unit->newx;
			unit->newx=param;
			break;		
			
		case 7: //set y
			unit->lasty=unit->newy;
			unit->newy=param;
			
			break;		
			
		case 8: //do next command with probability 
			
			unit->prob= param;
			
			break;		
			
		case 9: //buffer pointer back to start (or to param)
			pos= (int)(param+0.5); // test larger than 0;
			
			if(pos%2==1) --pos;
				
				if(pos<0) pos=0;	
					
					unit->bufpos= pos%(unit->mBufSize);
			
			break;	
			
		default: //if get to here, unknown instruction, step on through buffer is automatic
			
			break;
	}	
	
	
	
	if(newbreakpoint) {
		unit->lastx=unit->newx;
		unit->lasty=unit->newy;
		unit->newx=tmpx;
		unit->newy=tmpy;
	}
	


}



void preparelookuptables() {

float theta;

for (int i=0; i<100; ++i) {

theta= twopi*(i/99.0)-pi;

gg_lookupsin[i]=sin(theta);
gg_lookupcos[i]=cos(theta);

}

}


#ifdef SLUGENSRESEARCH
extern void initSLUGensResearch(InterfaceTable *);
#endif

extern "C" void load(InterfaceTable *inTable) {
	
	ft = inTable;
	
	DefineSimpleUnit(SortBuf);
	DefineSimpleUnit(GravityGrid);
	DefineSimpleUnit(GravityGrid2);
	DefineSimpleCantAliasUnit(Breakcore);
	DefineDtorUnit(Max);
	DefineSimpleUnit(PrintVal);
	DefineSimpleUnit(EnvDetect);
	DefineSimpleUnit(FitzHughNagumo);
	DefineSimpleUnit(DoubleWell);
	DefineSimpleUnit(DoubleWell2);
	DefineSimpleUnit(DoubleWell3);
	DefineSimpleCantAliasUnit(WeaklyNonlinear);
	DefineSimpleCantAliasUnit(WeaklyNonlinear2);
	DefineSimpleUnit(TermanWang);
	DefineDtorUnit(LTI);
	DefineDtorUnit(NL);
	DefineDtorUnit(NL2);
	DefineSimpleCantAliasUnit(LPCError);
	DefineDtorUnit(KmeansToBPSet1);
	DefineSimpleUnit(Instruction);
	DefineSimpleUnit(WaveTerrain);
	DefineSimpleUnit(VMScan2D);
	
	#ifdef SLUGENSRESEARCH
	initSLUGensResearch(inTable);
	#endif
	
	//printf("preparing SLUGens lookup tables\n");
	preparelookuptables();
	//printf("SLUGens ready\n");
	
//	printf("tests!\n");
//	
//	float tests[12]= {1.0,1.5,2.0,-1.0,-1.5,-2.0, -3, -4, -5, 3, 4, 17};
//	
//	for (int i=0; i<12; ++i) {
//	float x= tests[i];
//	
//	
//	printf("tests! %f %f \n", x, sc_fold(x,-1.0f,1.0f));
//	
//	}
	
	
	
}


