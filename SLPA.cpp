//============================================================================
// Name        : SLPA.cpp
// Author      : Jierui Xie (xiej2@rpi.edu)
// Date        : Nov. 5th, 2011
// Version     : v1.2
// Copyright   : All rights reserved.
// Description : SLPA algorithm for community detection.
// Web Site    : https://sites.google.com/site/communitydetectionslpa/
// Publication:
//             J. Xie, B. K. Szymanski and X. Liu, "SLPA: Uncovering Overlapping Communities in Social Networks via A Speaker-listener Interaction Dynamic Process", IEEE ICDM workshop on DMCCI 2011, Vancouver, CA.
//============================================================================

#include "SLPA.h"
#include "CommonFuns.h"
#include "CommonFuns_TMP.h"
#include "rndNumbers.h"
#include "fileOpts.h"

#include <pthread.h>

// modified by korchagin 
#include <sys/time.h>

/*
struct timeval start_time, end_time;

gettimeofday(&start_time, NULL);



*/


typedef std::tr1::unordered_map<int, int> UOrderedH_INT_INT;

bool SLPA::isDEBUG=false;

SLPA::SLPA(string inputFileName,vector<double> THRS,int maxRun,int maxT,string outputDir,bool isUseLargestComp,int numThreads) {
	//inputFileName: the full path
	//netName: short filename(non-suf)

	//---------------------------
	//Extract the fileName
	//---------------------------
	string a,b;
	fileName_net=inputFileName;
	extractFileName_FullPath(inputFileName,netName,a,b);

	networkPath="";
	net=new Net(networkPath,netName,fileName_net);

	//---------------------------
	//		GLPA parameters
	//---------------------------
	for(int i=0;i<THRS.size();i++)
		this->THRS.push_back(THRS[i]);   //why can not use [i]=.../???

	this->maxRun=maxRun;
	this->maxT=maxT;

	isSyn=false;
	this->isUseLargestComp=isUseLargestComp;
	//---------------------------
	//		more
	//---------------------------
	this->outputDir=outputDir;

	this->numThreads=numThreads;



	start();
}

SLPA::~SLPA() {
	delete net;
}

void SLPA::pre_initial_THRCS(){
	THRCS.clear();
	for(int i=0;i<THRS.size();i++){
		THRCS.push_back((int)myround(THRS[i]*maxT));
	}
}


void SLPA::start(){
	//---------------------------
	//  load network
	//---------------------------
	bool isSymmetrize=true; //symmetrize the edges

	net->readNetwork_EdgesList(fileName_net,isUseLargestComp,isSymmetrize);
	cout<<"Network info: N="<<net->N<< " M="<<net->M<<"(symmetric)"<<endl;
	cout<<"load "<<fileName_net<< " done.."<<endl;

	//net.showVertices();
	//net->showVertices_Table();

	//---------------------------
	//  convert thr to count_thr
	//---------------------------
	pre_initial_THRCS();

	//---------------------------
	//  	game
	//---------------------------
	for(int run=1;run<=maxRun;run++){
		//if(isDEBUG)
		cout<<" run="<<run<<"......"<<endl;

		//1.initial WQ and clear network
		initWQueue_more();

		//2.GLPA
		if(isSyn){
			//GLPA_syn();
		}
		else{
			if (numThreads == 0 || numThreads == 1)
			{
				GLPA_asyn_pointer();	
			}
			else
			{
				GLPA_asyn_pointer_pthread_v1();
				// GLPA_asyn_pointer_pthread_v2();
			}
		}

		//3.threshould and post-processing
		//a. create WQhistogram
		post_createWQHistogram_MapEntryList();


		//b. thresholding and output cpm
		for(int i=0;i<THRCS.size();i++){
			int thrc=THRCS[i];
			double thrp=THRS[i];

			time_t st=time(NULL);
			cout<<"Progress: Thresholding thr="<<thrp<<"......."<<endl;
			string fileName=outputDir+"SLPA_"+netName+"_run"+int2str(run)+"_r"+dbl2str(thrp)+ ".icpm";

			if(isDEBUG) cout<<"cpm="<<fileName<<endl;
			post_threshold_createCPM_pointer(thrc,fileName);

			//cout<<" Take :" <<difftime(time(NULL),st)<< " seconds."<<endl;
		}
	}
}

void SLPA::initWQueue_more(){
	time_t st=time(NULL);
	cout<<"Progress: Initializing memory......."<<endl;

	//label is node id
	NODE *v;
	for(int i=0;i<net->N;i++){

		v=net->NODES[i];

		//label
		//v->WQueue.clear();
		v->WQueue.push_back(v->ID); //INITIALLY, there is **one**

		//other
		//v->WQHistMapEntryList.clear();


		//v->WQHistgram.clear();


	}

	cout<<" Take :" <<difftime(time(NULL),st)<< " seconds."<<endl;
}

void SLPA::GLPA_asyn_pointer(){
	//pointer version:
	//	 store the pointer of nb in *nbList_P*
	//   save time for retrieving hashTable
	time_t st=time(NULL);

	NODE *v,*nbv;
	int label;
	vector<int> nbWs;
	map<int,NODE *>::iterator mit;

	//t=1 because we initialize the WQ(t=0)
	cout<<"Start iteration:";

	for(int t=1;t<maxT;t++){
		//1.shuffle
		//cout<<"-------------t="<<t<<"---------------------"<<endl;
		cout<<"*"<<flush;
		srand (time(NULL)); // ***YOU need to use this, such that you can get a new one each time!!!!! seed the random number with the system clock
		random_shuffle (net->NODES.begin(), net->NODES.end());
		//net->showVertices();


		//2. do one iteration-asyn
		for(int i=0;i<net->N;i++){
			v=net->NODES[i];

			//a.collect labels from nbs
			nbWs.clear();

			for(int j=0;j<v->numNbs;j++){
				nbv=v->nbList_P[j];
				nbWs.push_back(nbv->WQueue[mtrand2.randInt(nbv->WQueue.size()-1)]);
			}

			//b.select one of the most frequent label
			label=ceateHistogram_selRandMax(nbWs);

			//c. update the WQ **IMMEDIATELY**
			v->WQueue.push_back(label);
		}

		//cout<<" Take :" <<difftime(time(NULL),st)<< " seconds."<<endl;
	}

	cout<<endl;
	cout<<"Iteration is over (takes "<<difftime(time(NULL),st)<< " seconds)"<<endl;
}

void SLPA::GLPA_asyn_pointer_pthread_v1() {

	// variables for
	NODE * v, * nbv;
	int label;
	vector<int> nbWs;
	map<int, NODE *>::iterator mit;

	// variables for timing
	struct timeval start_time, end_time;
	struct timeval time_1, time_2;
	float total_time_used;
	float time_used;

	// variables for pthread
	int start_index[numThreads];
	int end_index[numThreads];

	pthread_t threads[numThreads];
	pthread_attr_t attr;
	struct thread_data_evolution thread_data_array[numThreads];
	void * status;
	int rc;

	decomposeTasks(net->N, numThreads, start_index, end_index);

	gettimeofday(&start_time, NULL);

	cout << "Iteration starts" << endl;
	for (int t = 1; t < maxT; t ++ )
	{
		/*
		gettimeofday(&time_1, NULL);
		*/

		srand (time(NULL));
		random_shuffle(net->NODES.begin(), net->NODES.end());

		// pthread start
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		for (int i = 0; i < numThreads; i ++)
		{
			if (isDEBUG) 
			{
				cout << "creating threads" << endl;
			}

			thread_data_array[i].start_index = start_index[i];
			thread_data_array[i].end_index = end_index[i];
			thread_data_array[i].pThis = this;

			rc = pthread_create(&threads[i], NULL, GLPA_asyn_one_thread_v1, (void *) &thread_data_array[i]);
			if (rc) 
			{
				cout << "ERROR: return code from pthread_create() is " << rc << endl;
				exit(-1);

			}
		}
		//cout << "t = " << t << endl;	
		pthread_attr_destroy(&attr);
		for (int i = numThreads-1; i >= 0; i --)
		{
			//cout << "i = " << i << endl;
			//cout << threads[i] << endl;
			//printf("thread id = %u\n", threads[i]);
			//cout << "xx" << endl;
			rc = pthread_join(threads[i], &status);
			//cout << "ggg" << endl;
			if (rc) 
			{
				cout << "ERROR: return code from pthread_join() is " << rc << endl;
				exit(-1);
			}
			//cout << "fff" << endl;
		}
		//cout << "hello" << endl;
		// pthread end
		/*
		gettimeofday(&time_2, NULL);
		time_used = 1000000 * (time_2.tv_sec - time_1.tv_sec) + (time_2.tv_usec - time_1.tv_usec);
		time_used /= 1000000;
		cout << "Iteration " << t << " is over, time used is " << time_used << " seconds" << endl;
		*/
	}

	gettimeofday(&end_time, NULL);

	total_time_used = 1000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec);
	total_time_used /= 1000000;

	cout << endl << "Iteration is over (takes " << total_time_used << " seconds)" << endl;
}

void * SLPA::GLPA_asyn_one_thread_v1(void * thread_args) {
	
	// cout << "hello" << endl;
	struct thread_data_evolution * temp_data;
	temp_data = (struct thread_data_evolution * ) thread_args;

	// variables for evolution
	NODE * v, * nbv;
	int label;
	vector<int> nbWs;
	map<int, NODE *>::iterator mit;

	SLPA * slpa_p = temp_data->pThis;
	Net * net_p = temp_data->pThis->net;


	for (int i = temp_data->start_index; i <= temp_data->end_index; i ++)
	{
		v = net_p->NODES[i];

		// a. collect lebels from nbs
		nbWs.clear();
		for (int j = 0; j < v->numNbs; j ++)
		{
			nbv = v->nbList_P[j];
			nbWs.push_back(nbv->WQueue[temp_data->mtrand_t.randInt(nbv->WQueue.size()-1)]);
		}

		// b. select one of the most freauent label
		label = slpa_p->ceateHistogram_selRandMax(nbWs);

		// c. update the WQ ** IMMEDIATELY **
		v->WQueue.push_back(label);
	}

	//cout << "single " << endl;
	pthread_exit(NULL);
}

void SLPA::GLPA_asyn_pointer_pthread_v2() {

	// variables for timing
	struct timeval start_time, end_time;
	struct timeval time_1, time_2;
	float total_time_used;
	float time_used;

	// variables for pthread
	int start_index[numThreads];
	int end_index[numThreads];

	pthread_t threads[numThreads];
	pthread_attr_t attr;
	struct thread_data_evolution thread_data_array[numThreads];
	void * status;
	int rc;
	// pthread_barrier_t barrier;


	decomposeTasks(net->N, numThreads, start_index, end_index);

	gettimeofday(&start_time, NULL);

	// pthread start
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_barrier_init(&barrier, NULL, numThreads);

	for (int i = 0; i < numThreads; i ++)
	{
		if (isDEBUG)
		{
			cout << "creating threads" << endl;
		}

		thread_data_array[i].start_index = start_index[i];
		thread_data_array[i].end_index = end_index[i];
		thread_data_array[i].pThis = this;

		rc = pthread_create(&threads[i], &attr, GLPA_asyn_one_thread_v2, (void * ) &thread_data_array[i]);
		if (rc)
		{
			cout << "ERROR: return code from pthread_create() is " << rc << endl;
			exit(-1);
		}
	}

	pthread_attr_destroy(&attr);

	for (int i = numThreads-1; i >= 0; i --)
	{
		rc = pthread_join(threads[i], &status);
		if (rc)
		{
			cout << "ERROR: return code from pthread_join() is " << rc << endl;
			exit(-1);
		}
	}
	pthread_barrier_destroy(&barrier);
	// pthread end

	gettimeofday(&end_time, NULL);

	total_time_used = 1000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec);
	total_time_used /= 1000000;

	cout << endl << "Iteration is over (takes " << total_time_used << " seconds)" << endl;
}

void * SLPA::GLPA_asyn_one_thread_v2(void * thread_args) {

	struct thread_data_evolution * temp_data;
	temp_data = (struct thread_data_evolution *) thread_args;

	//variables for evolution
	NODE * v, * nbv;
	int label;
	vector<int> nbWs;
	map<int, NODE *>::iterator mit;
	SLPA * slpa_p = temp_data->pThis;
	Net * net_p = temp_data->pThis->net;

	vector<NODE *>::iterator start_iter = net_p->NODES.begin() + temp_data->start_index;
	vector<NODE *>::iterator end_iter = net_p->NODES.begin() + temp_data->end_index + 1;

	for (int t = 1; t < slpa_p->maxT; t ++)
	{
		cout << "t = " << t << endl;
		srand(time(NULL));
		random_shuffle(start_iter, end_iter);

		//2. do one iteration-asyn
		for(int i = temp_data->start_index; i <= temp_data->end_index; i++ )
		{
			v=net_p->NODES[i];

			//a.collect labels from nbs
			nbWs.clear();

			for(int j=0;j<v->numNbs;j++){
				nbv=v->nbList_P[j];
				nbWs.push_back(nbv->WQueue[temp_data->mtrand_t.randInt(nbv->WQueue.size()-1)]);
			}

			//b.select one of the most frequent label
			label=slpa_p->ceateHistogram_selRandMax(nbWs);

			//c. update the WQ **IMMEDIATELY**
			v->WQueue.push_back(label);
		}


		// synchronization
		// pthread_barrier_wait(&slpa_p->barrier);
	}
}

int SLPA::ceateHistogram_selRandMax(const vector<int>& wordsList){
	int label;
	map<int,int> hist;
	map<int,int>::iterator mit;
	//------------------------------------------
	//	    1. create the histogram
	//------------------------------------------
	//count the number of Integer in the wordslist
	createHistogram(hist, wordsList);

	//------------------------------------------
	//2. randomly select label(key) that corresponding to the max *values*.
	//	    sort the key-value pairs, find the multiple max
	//		randomly select one and return the label(key)
	//------------------------------------------
	//***list is int he decreasing order of value.
	vector<pair<int,int> > pairlist;
	sortMapInt_Int(hist, pairlist);

	//for(Map.Entry en : list) {
	//	System.out.printf("  %-8s%d%n", en.getKey(), en.getValue());
	//}
	//cout<<"-------------------"<<endl;
	//for(int i=0;i<pairlist.size();i++){
	//	cout<<"w="<<pairlist[i].first<<" count="<<pairlist[i].second<<endl;
	//}

	int maxv=pairlist[0].second;
	int cn=1;

	//cout<<"maxv="<<maxv<<endl;
	for(int i=1;i<pairlist.size();i++){    //start from the **second**
		if(pairlist[i].second==maxv)       //multiple max
			cn++;
		else
			break; //stop when first v<maxv
	}


	if(cn==1)
		label=pairlist[0].first;         //key-label
	else{
		//generator.nextInt(n); 0~n-1
		//int wind=rndDblBtw0Nminus1(cn);
		int wind=mtrand1.randInt(cn-1); //**[0~n]
		//cout<<"*****wind="<<wind<<endl;


		label=pairlist[wind].first;
	}
	//cout<<"cn="<<cn<<endl;
	//cout<<"label="<<label<<endl;


	return label;
}


void SLPA::post_createWQHistogram_MapEntryList(){
	NODE *v;
	map<int,int> WQHistgram;

	for(int i=0;i<net->N;i++){
		v=net->NODES[i];

		//use WQueue to create histogram
		WQHistgram.clear();
		createHistogram(WQHistgram, v->WQueue);

		//list is in the ***decreasing*** order of value(count).
		//use histogram to create a pair list
		v->WQHistMapEntryList.clear();
		sortMapInt_Int(WQHistgram, v->WQHistMapEntryList);

		//MEMORY::remove v->WQueue to
		v->WQueue.clear();
	}

	//cout<<"Progress: Created mapEntryList ......."<<endl;
}

void SLPA::dothreshold_createCPM_pointer(int thrc,vector<vector<int>* >& cpm){
	time_t st=time(NULL);

	//the map of distinct label and community id
	map<int,int> w_comidTable;
	map<int,int>::iterator mit;

	int comid=-1; //**-1, such that we can access via vector[comid]

	NODE *v;
	int ov_cn=0;
	for(int i=0;i<net->N;i++){
		v=net->NODES[i];

		//1.get the world list after threshoulding
		vector<int> WS;  //w list that beyond the thrc
		post_thresholding(v->WQHistMapEntryList,thrc,WS); //***TO IMP

		if(WS.size()<1) cout<<"ERROR:empty WS"<<endl;
		if(WS.size()>1) ov_cn++;

		//2. create CPM:put each membership to a community
		for(int j=0;j<WS.size();j++){
			int label=WS[j];

			//------------
			if(w_comidTable.count(label)==0){//not in yet
				comid++;
				w_comidTable.insert(pair<int,int>(label, comid)); //**

				//cpm.push_back(vector<int>());  //copy to the (an empty vector)

				//***CPMPP
				vector<int>* avector=new vector<int>();  //TO REMOVE
				cpm.push_back(avector);  //copy to the (an empty vector)
			}

			//------------
			mit=w_comidTable.find(label);
			int v_comid=mit->second;

			//cpm[v_comid].push_back(v->ID);  //add one id
			cpm[v_comid]->push_back(v->ID);  //add one id
		}
	}

	//cout<<"Creating CPM takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;
}


void SLPA::post_threshold_createCPM_pointer(int thrc,string fileName){
	bool isDEBUG=false;
	time_t st;

	//CPM: the index is the **commID(0~k-1)**, and vales is node **ID**
	vector<vector<int>* > cpm; //***CPMP, TO REMOVE

	//=========================================
	//1.threshold + createCPM
	//=========================================
	dothreshold_createCPM_pointer(thrc,cpm);


	//=========================================
	//2.***post process*** the communities CPM
	//=========================================
	//a. reassign sameLabel disconnected subcomponent
	//(handle the same label disconnected components problems in general)

	/*if(false) {
		if(isDEBUG) cout<<"---before reassign---"<<endl;
		if(isDEBUG) printVectVect_PRIMITIVE<int>(cpm);
		cpm=post_sameLabelDisconnectedComponents(cpm); //**TO IMPROVE
		if(isDEBUG) cout<<"---After reassign---"<<endl;
		if(isDEBUG) printVectVect_PRIMITIVE<int>(cpm);
	}*/

	//---------------------------
	//b. remove subset
	//---------------------------
	//if(isDEBUG) cout<<"---before---"<<endl;
	//if(isDEBUG) printVectVect_PRIMITIVE<int>(cpm);

	st=time(NULL);
	//cpm=post_removeSubset(cpm);                    //**TO IMPROVE
	//cpm=post_removeSubset_HashTable(cpm);
	//cpm=post_removeSubset_UorderedHashTable(cpm);

	//**working single thread version
	if(numThreads==0)
		cpm=post_removeSubset_UorderedHashTable_cpmpointer(cpm);
	else	// multi threads
		cpm=post_removeSubset_UorderedHashTable_cpmpointer_MultiThd(cpm);

	cout<<"removeSubset takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;

	//if(isDEBUG) cout<<"---After---"<<endl;
	//if(isDEBUG) printVectVect_PRIMITIVE<int>(cpm);

	//---------------------------
	//4. save cpm
	//---------------------------
	st=time(NULL);
	sort_cpm_pointer(cpm);  //sort each com by increasing ID for; and by decrasing community size
	//cout<<"sorting takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;

	st=time(NULL);
	write2txt_CPM_pointer(fileName,cpm);
	//cout<<"write2tx takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;

	//if(isDEBUG)  show_cpm(cpm);


	//---------------------------
	//release memory
	//---------------------------
	for(int i=0;i<cpm.size();i++)
		delete cpm[i];

}


void SLPA::post_thresholding(vector<pair<int,int> >& pairList, int thrc, vector<int>& WS){
	//For label with count<=THRESHOULD **COUNT**,
	//we remove it from the hist(here is represented by WQHistMapEntryList) then
	//some nodes may become ***unlabeled***.if a node becomes unlabeled,
	//   keep the most frequent label in its list
	//   RETURN: the labels after threshoulding
	int label;

	//cout<<"------------------thrc="<<thrc<<endl;
	//for(int i=0;i<pairList.size();i++)
	//	cout<<"  "<< pairList[i].first<<" cout="<< pairList[i].second<<endl;

	//*****list MUST BE already ordered in **decreasing count order.****

	int maxv=pairList[0].second; //first one is max count

	if(maxv<=thrc){//keep one label to avoid unlabeled node randomly
		// collect the max count
		int cn=1;
		for(int i=1;i<pairList.size();i++){              //start from the **second**
			if(pairList[i].second==maxv)//multiple max
				cn++;
			else
				break; //stop when first v<maxv
		}

		// handle the multiple max counts
		if(cn==1)
			label=pairList[0].first;  //key
		else{
			//generator.nextInt(n); 0~n-1
			int wind=mtrand2.randInt(cn-1);
			//cout<<"wind="<<wind<<endl;
			label=pairList[wind].first;  //key randInt->[0~n]
		}

		//add one
		WS.push_back(label);

	}
	else{
		//go down the list until below the thrc
		for(int i=0;i<pairList.size();i++){              //start from the **first**
			if(pairList[i].second>thrc){                 //cout**Threshold**
				label=pairList[i].first;				 //key
				WS.push_back(label);
			}
			else
				break;									//stop when first v<thrc
		}
	}
}

void SLPA::sort_cpm_pointer(vector<vector<int>* >& cpm){
	//inside each com, sort ID by **increasing** order
	for(int i=0;i<cpm.size();i++){
		//sort(cpm[i].begin(),cpm[i].end(),sort_INT_INC());
		sort(cpm[i]->begin(),cpm[i]->end(),sort_INT_INC());
	}

	//each com, sort **decreasing** community size
	sortVecVec_bySize_pointer<int>(cpm);
}


vector<vector<int>* > SLPA::post_removeSubset_UorderedHashTable_cpmpointer(vector<vector<int>* >& cpm){
	time_t st;
	//bool isDEBUG=true;
	//if(isDEBUG) cout<<"removeSubset (Unordered HASH)............."<<endl;

	vector<vector<int>* > newcpm;


	//1. ***sort cpm by the community size(***decreasing***)
	st=time(NULL);
	sort_cpm_pointer(cpm);  //***CMPP

	//cout<<"sort_cpm takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;

	//2.check the subset relationship
	//cout<<"***before cpm.sie="<<cpm.size()<<endl;
	st=time(NULL);

	//2.1 vector of map corresponding to the sorted cpm(decreasing)
	vector<UOrderedH_INT_INT* > vectHTable;

	for(int i=0;i<cpm.size();i++){
		UOrderedH_INT_INT* H=new UOrderedH_INT_INT;  //**
		for(int j=0;j<cpm[i]->size();j++)            //***CMPP
			//H->insert(pair<int,int>(cpm[i][j],cpm[i][j])); //id as both key and value
			H->insert(pair<int,int>((*cpm[i])[j],(*cpm[i])[j])); //id as both key and value

		vectHTable.push_back(H);
	}

	//0.the indicator
	//vector<int> indicators;
	//for(int i=0;i<cpm.size();i++)
	//	indicators.push_back(1);  //1-default ok
	int indicators[cpm.size()];
	for(int i=0;i<cpm.size();i++)
		indicators[i]=1;  		  //1-default ok

	//2.2 find the subset (compare smaller cpmi to largest H(j) first)
	bool issubset;
	for(int i=cpm.size()-1;i>0;i--){
		for(int j=0;j<i;j++){
			//visit all coms(in HASH) that are LARGER than it
			//check if cpm(i) is a subset of H(j)
			issubset=true;
			for(int k=0;k<cpm[i]->size();k++)
				if(vectHTable[j]->count((*cpm[i])[k])==0){//not found
					issubset=false;
					break;
				}

			//issubset=issubset_cpm_hash(cpm[i],vectHTable[j]);

			if(issubset){  //remove this cpm
				indicators[i]=0;   //**change i**
				break;
			}
		}
	}

	//3.newcpm
	for(int i=0;i<cpm.size();i++){
		if(indicators[i]==0) continue;

		newcpm.push_back(cpm[i]);
	}

	//release memory
	for(int i=0;i<vectHTable.size();i++)
		delete vectHTable[i];

	return newcpm;
}

void SLPA::write2txt_CPM_pointer(string fileName,vector<vector<int>* >& cpm) {
	vector<string> data;

	for(int i=0;i<cpm.size();i++){
		//vector<int>& oneComm=cpm[i]; //ref
		vector<int>& oneComm=*cpm[i]; //**CPMP ref

		string line;
		for(int j=0;j<oneComm.size();j++){
			line+=int2str(oneComm[j]);
			line+=" ";
		}
		data.push_back(line);
	}

	//fileOpts.writeToTxt(fileName, false, data);// **false:the first one
	writeToTxt(fileName, false, data);
}

//------------------------------------------------
//			org cpm functions
//------------------------------------------------
void SLPA::sort_cpm(vector<vector<int> >& cpm){
	//inside each com, sort ID by **increasing** order
	for(int i=0;i<cpm.size();i++){
		sort(cpm[i].begin(),cpm[i].end(),sort_INT_INC());
	}

	//each com, sort **decreasing** community size
	sortVecVec_bySize<int>(cpm);
}

//------------------------------------------------
//			Multi-threading
//------------------------------------------------
void SLPA::decomposeTasks(int numTasks,int numThd,int stInds[],int enInds[]){
	int rem=numTasks%numThd;
	int step=(numTasks-rem)/numThd;  //**TO IMPROVE

	for(int i=0;i<numThd;i++){
		stInds[i]=i*step;
		enInds[i]=(i+1)*step-1;
	}
	enInds[numThd-1]+=rem;

	if(false){
		cout<<"----------------decomposeTasks---------------"<<endl;
		cout<<"rem="<<rem<<" step="<<step<<endl;
		for(int i=0;i<numThd;i++){
			cout<<stInds[i]<<" "<<enInds[i]<<endl;
		}
	}

}
void *SLPA::removesubset_onethread(void *threadarg){
	// set the corresponding element in indicators
	// and return the pointer

	//We use pointers:
	//***ASSUMING the my_data->cpm do the shallow copy(**pointers**) from the original one
	//   then we can the following

	struct thread_data *my_data;
	my_data = (struct thread_data *) threadarg;

	if(false) cout<<"startind="<<my_data->startind<<" endind="<<my_data->endind<<endl;

	//----------------------
	//references of the **shallow** copy in thread_data
	vector<vector<int>* >& cpm=my_data->cpm;
	vector<UOrderedH_INT_INT* >& vectHTable=my_data->vectHTable;
	int *indicators=my_data->pIndicator;    //*the array name as pointer


	//2.2 find the subset (compare smaller cpmi to largest H(j) first)
	bool issubset;
	//for(int i=cpm.size()-1;i>0;i--){
	//**ONLY in stinds~eninds**
	for(int z=my_data->endind;z>=my_data->startind;z--){
		int i=z;

		//------same as before----
		for(int j=0;j<i;j++){
			//visit all coms(in HASH) that are LARGER than it
			//check if cpm(i) is a subset of H(j)
			issubset=true;
			for(int k=0;k<cpm[i]->size();k++)
				if(vectHTable[j]->count((*cpm[i])[k])==0){//not found
					issubset=false;
					break;
				}

			//issubset=issubset_cpm_hash(cpm[i],vectHTable[j]);

			if(issubset){          //remove this cpm
				indicators[i]=0;   //**change i**
				break;
			}
		}
	}

	//----------------------
	//for(int i=my_data->startind;i<=my_data->endind;i++)
	//		my_data->pIndicator[i]=i;

	pthread_exit(NULL);
}



vector<vector<int>* > SLPA::post_removeSubset_UorderedHashTable_cpmpointer_MultiThd(vector<vector<int>* >& cpm){
	time_t st;
	bool isDEBUG=false;
	cout<<"removeSubset (Multiple threads)............."<<endl;

	vector<vector<int>* > newcpm;


	//1. ***sort cpm by the community size(***decreasing***)
	st=time(NULL);
	sort_cpm_pointer(cpm);  //***CMPP

	//cout<<"sort_cpm takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;

	//2.check the subset relationship
	//cout<<"***before cpm.sie="<<cpm.size()<<endl;
	st=time(NULL);

	//2.1 vector of map corresponding to the sorted cpm(decreasing)
	vector<UOrderedH_INT_INT* > vectHTable;

	for(int i=0;i<cpm.size();i++){
		UOrderedH_INT_INT* H=new UOrderedH_INT_INT;  //**
		for(int j=0;j<cpm[i]->size();j++)            //***CMPP
			//H->insert(pair<int,int>(cpm[i][j],cpm[i][j])); //id as both key and value
			H->insert(pair<int,int>((*cpm[i])[j],(*cpm[i])[j])); //id as both key and value

		vectHTable.push_back(H);
	}

	//===========================================
	int indicators[cpm.size()];
	for(int i=0;i<cpm.size();i++)
		indicators[i]=1;  		  //1-default ok

	int numTasks=cpm.size();
	int numThd=numThreads;       //****

	int stInds[numThd];
	int enInds[numThd];

	decomposeTasks(numTasks, numThd, stInds, enInds);
	//------------------------------------------------
	struct thread_data thread_data_array[numThd];

	pthread_t threads[numThd];    //**
	pthread_attr_t attr;
	void *status;

	// Initialize and set thread detached attribute
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int rc;
	long t;
	for( t=0; t<numThd; t++){
		if(isDEBUG) cout<<"creating thread "<<t<<endl;
		thread_data_array[t].startind=stInds[t];
		thread_data_array[t].endind=enInds[t];
		thread_data_array[t].pIndicator=indicators;   //**TO change in function**
		thread_data_array[t].cpm=cpm;                //**shallow copy**
		thread_data_array[t].vectHTable=vectHTable;   //**shallow copy**

		rc = pthread_create(&threads[t], NULL, removesubset_onethread, (void *) &thread_data_array[t]);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	// Free attribute and wait for the other threads
	pthread_attr_destroy(&attr);

	//**This determines the order
	for(t=numThd-1; t>=0; t--) {
		rc=pthread_join(threads[t], &status);
		if (rc) {
			cout<<"ERROR; return code from pthread_join() is "<<rc<<endl;
			exit(-1);
		}
	}

	//------------------------------------------------
	if(isDEBUG) for(int i=0;i<cpm.size();i++)
		cout<<"indicator["<<i<<"]="<<indicators[i]<<endl;

	//===========================================
	//3.newcpm
	for(int i=0;i<cpm.size();i++){
		if(indicators[i]==0) continue;

		newcpm.push_back(cpm[i]);
	}

	//release memory
	for(int i=0;i<vectHTable.size();i++)
		delete vectHTable[i];

	return newcpm;
}
