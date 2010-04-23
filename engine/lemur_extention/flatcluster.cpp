// put in lemur-4.5/app/src
#include <fstream.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "IndexedReal.hpp"
#include "IndexManager.hpp"
// #include "BasicIndex.hpp"
//include "Index.hpp"

//#include "IndexCount.hpp"
#include "Param.hpp"
#include "String.hpp"

#include <math.h>
/// flat mixture model clustering

using namespace lemur::api;

namespace LocParam {
	static lemur::utility::String index;
	double lambda_b;
	int cNum; 
	
	int trial,it;
	void get() {
		index = ParamGetString("index");
		lambda_b = ParamGetDouble("lambdaB",0.4);
		cNum = ParamGetInt("cluster",5);
		trial = ParamGetInt("trial",20);
		it = ParamGetInt("it",50);
	}
};

void GetAppParam()
{
	LocParam::get();
}

inline double colPr(Index &ind, int t)
{
	return (1.0 + ind.termCount(t)) / (double)(ind.termCountUnique() + ind.termCount());
}

int AppMain(int argc, char *argv[]) {

	Index * ind = IndexManager::openIndex(LocParam::index);

	int wN = ind->termCountUnique();
	int cN = LocParam::cNum;
	int dN = ind->docCount();
	
	int k, i;
	
	double lambdaB = LocParam::lambda_b; // background
	
	int d;
	
	// double **pi = new (double *)[dN+1]; // class probability
	double **pi;
	pi = new double*[dN + 1];
	
	// double **c= new (double *)[cN]; // component models
	double **c;
	c = new double*[cN];
	
	// double **piEst = new (double *)[dN+1]; // class probability
	double **piEst;
	piEst = new double*[dN + 1];
	
	// double **cEst= new (double *)[cN]; // component models
	double **cEst;
	cEst = new double*[cN];
	
	for (k = 0; k < cN; k++) {
		c[k] = new double[wN + 1]; // reserve 0-index as normalizer
		cEst[k] = new double[wN + 1]; // reserve 0-index as normalizer
	}
	
	for (d = 1; d <= dN; d++) {
		pi[d] = new double[cN + 1];
		piEst[d] = new double[cN + 1];
	}
	
	int trial=0;
	
	double curFit, meanFit, bestFit;
	meanFit = 1e-40;
	
	double *z = new double[cN + 1];
	double *tmpZ = new double[cN + 1];
	
	//  do {
    trial++;
    for (k = 0; k < cN; k++) {
		cEst[k][0] = 0;
		for (i = 1; i <= wN; i++) {
			cEst[k][i] = 0.1 / wN + 0.9 * drand48();
			cEst[k][0] += cEst[k][i];
		}
    }
    
    for (d = 1; d <= dN; d++) {
		piEst[d][0] = 0;
		for (k = 1; k <= cN; k++) {
			piEst[d][k] = 0.9 / cN + 0.1 * drand48();
			piEst[d][0] += piEst[d][k];
		}
    }
	
    double sc = 0.0000001;
	
    int it = 0;
    meanFit = 1e-40;
    do { // EM
		it++;
		curFit = 0;
		//      cerr<< "---\n";
		// parameter estimation
		
		// class prob
		for (d = 1; d <= dN; d++) {
			for (k = 1; k <= cN; k++) {
				pi[d][k] = (sc + piEst[d][k]) / (sc * cN + piEst[d][0]);
				//  cout << "pi="<< pi[d][k] << " e:"<< piEst[d][k] << " sum:"<< piEst[d][0]<<endl;
				piEst[d][k] = 0;
			} 
			piEst[d][0] = 0;
		}
		
		// component model
		for (k = 0; k < cN; k++) {
			for (i = 1; i <= wN; i++) {
				c[k][i] = (cEst[k][i] + sc) / (sc * wN + cEst[k][0]);
				cEst[k][i] = 0;
			}
			cEst[k][0]=0;
		}
		
		TermInfoList *tList;
		
		// compute z
		double pw, zc, zs;
		TermInfo *info;
		double fq;
		int id;
		for (d = 1; d <= dN; d++) {
			tList = ind->termInfoList(d);
			tList->startIteration();
			
			double llnorm=0;
			while (tList->hasMore()) {
				info = tList->nextEntry();
				fq = info->count();
				//id = info->id();
				id = info->termID();
				double zsum=0;
				for (k = 0; k < cN; k++) {
					z[k] = (1 - lambdaB) * pi[d][k+1] * c[k][id];
					//   cout << "l="<<lambdaB << " pi="<< pi[d][k+1] <<" c="<< c[k][id]<<endl;
					zsum += z[k];
				}
				zsum += lambdaB * colPr(*ind, id);
				//  cout << zsum << endl; 
				curFit += fq * log(zsum);
				zsum = 1.0 / zsum;
				for (k = 0; k < cN; k++) {
					z[k] = z[k] * zsum;
				}
				
				for (k = 0;k < cN; k++) {
					piEst[d][k+1] += z[k];
					piEst[d][0] += z[k];
					cEst[k][id] += z[k] * fq;
					cEst[k][0] += z[k] * fq;
				}
			}
			delete tList;
		}
		meanFit = 0.1 * meanFit + 0.9 * curFit;
		cout << "*** current fit:" << curFit <<endl;
    } while (fabs((meanFit - curFit) / meanFit) > 0.0000001 && it < LocParam::it);
    if (trial == 1 || curFit > bestFit) bestFit = curFit;
    cout << "######### current: " <<curFit << "  best: "<< bestFit << endl;
    //  } while ( trial <LocParam::trial || curFit < bestFit );
	
	int n;
	IndexedRealVector temp;
	IndexedRealVector::iterator itr;
	
	for (k = 0; k < cN; k++) {
		double sum=0;
		for (d = 1; d <= dN; d++) {
			sum += pi[d][k+1];
		}
		cout << "****** cluster " << k <<  " docprob: " << sum / dN << " *********\n";
		temp.clear();
		for (i = 1; i <= wN; i++) {
			temp.PushValue(i, c[k][i]);
		}
		temp.Sort();
		itr=temp.begin();
		for (n = 1; n <= 50; n++) {
			if (itr == temp.end())
				break;
			cout <<  ind->term((*itr).ind) << " " << (*itr).val << endl;
			//      c[k][(*itr).ind]=-1;
			itr++;
		}
	}
	cout << "#### FINAL FIT: " << curFit << endl;
	
	for (d = 1; d <= dN; d++) {
		int maxC = 0;
		double maxP;
		for (i = 1; i <= cN; i++) {
			if (i == 1) {
				maxC = i;
				maxP = pi[d][i];
			} else if (pi[d][i] > maxP) {
				maxC = i;
				maxP = pi[d][i];
			}
		}
		cout << ind->document(d) << " cluster " << maxC << " " << maxP << endl;
	}
	cerr << "#### FINAL FIT: " << curFit << endl;
	
	return 0;
}
