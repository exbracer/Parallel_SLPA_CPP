//============================================================================
// Name        : SLPA.h
// Author      : Jierui Xie (xiej2@rpi.edu)
// Date        : Oct. 2011
// Version     :
// Copyright   : All rights reserved.
// Description : SLPA algorithm for community detection.
// Web Site    : https://sites.google.com/site/communitydetectionslpa/
// Publication:
//             J. Xie, B. K. Szymanski and X. Liu, "SLPA: Uncovering Overlapping Communities in Social Networks via A Speaker-listener Interaction Dynamic Process", IEEE ICDM workshop on DMCCI 2011, Vancouver, CA.
//============================================================================
#ifndef SLPA_H_
#define SLPA_H_

#include "Net.h"
#include "NODE.h"
#include <map>
#include <vector>
#include <utility>
#include <tr1/unordered_map>

#include "MersenneTwister.h"


//---------------------------
//		Multi-threading
//---------------------------
typedef std::tr1::unordered_map<int, int> UOrderedH_INT_INT;

struct thread_data{
	int  startind;
	int  endind;

	int *pIndicator;

	//expect to do sharro copy of the pointers
	vector<vector<int>* > cpm;
	vector<UOrderedH_INT_INT* > vectHTable;
};
/*
struct thread_data_evolution {

	int start_index;
	int end_index;

	SLPA * pThis;
};
*/

class SLPA {
public:
	//---------------------------
	//		network parameters
	//---------------------------
	Net* net;
	string netName;
	string fileName_net;
	string networkPath;

	bool isUseLargestComp; //***
	//---------------------------
	//		SLPA parameters
	//---------------------------
	vector<double> THRS;      //thr
	vector<int> THRCS; 		  //thr count
	bool isSyn;  			  //is synchronous version?
	int maxT;
	int maxRun;

	//---------------------------
	//		more
	//---------------------------
	string outputDir;

	MTRand mtrand1;
	MTRand mtrand2;

	SLPA(string, vector<double>,int ,int ,string ,bool,int );
	// SLPA(string, vector<double>, int, int, string, bool, int, int, int); // add rank & size
	virtual ~SLPA();

	void start();
	void pre_initial_THRCS();
	void initWQueue_more();

	//void GLPA_asyn();
	void GLPA_asyn_pointer();

	int ceateHistogram_selRandMax(const vector<int>& wordsList);
	void post_createWQHistogram_MapEntryList();
	void post_thresholding(vector<pair<int,int> >& pairList, int thrc, vector<int>& WS);

	//need to change
	//vector<vector<int> > post_sameLabelDisconnectedComponents(vector<vector<int> >& cpm);
	//static void show_cpm(vector<vector<int> >& cpm);
	static void sort_cpm(vector<vector<int> >& cpm);

	//cpm pointer function
	vector<vector<int>* > post_removeSubset_UorderedHashTable_cpmpointer(vector<vector<int>* >& cpm);
	static void sort_cpm_pointer(vector<vector<int>* >& cpm);

	void write2txt_CPM_pointer(string fileName,vector<vector<int>* >& cpm);
	void post_threshold_createCPM_pointer(int thrc,string fileName);

	void dothreshold_createCPM(int thrc,vector<vector<int> >& cpm);
	void dothreshold_createCPM_pointer(int thrc,vector<vector<int>* >& cpm);

	static bool isDEBUG;


	//---------------------------
	//		Multi-threading
	//---------------------------
	int numThreads;

	pthread_barrier_t barrier;


	void decomposeTasks(int numTasks,int numThd,int stInds[],int enInds[]);
	static void *removesubset_onethread(void *threadarg);
	vector<vector<int>* > post_removeSubset_UorderedHashTable_cpmpointer_MultiThd(vector<vector<int>* >& cpm);

	// added by korchagin
	void GLPA_asyn_pointer_pthread_v1(); // added by korchagin
	static void * GLPA_asyn_one_thread_v1(void *);
	void GLPA_asyn_pointer_pthread_v2();
	static void * GLPA_asyn_one_thread_v2(void *);

	//---------------------------
	//      MPI 
	//---------------------------

	// int proc_id;
	// int num_procs;
};

struct thread_data_evolution {

	int start_index;
	int end_index;
	MTRand mtrand_t;
	SLPA * pThis;
};
#endif /* SLPA_H_ */
