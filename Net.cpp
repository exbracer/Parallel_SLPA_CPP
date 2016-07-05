//============================================================================
// Name        : Net.cpp
// Author      : Jierui Xie (xiej2@rpi.edu)
// Date        : Oct. 2011
// Version     :
// Copyright   : All rights reserved.
// Description : SLPA algorithm for community detection.
// Web Site    : https://sites.google.com/site/communitydetectionslpa/
// Publication:
//             J. Xie, B. K. Szymanski and X. Liu, "SLPA: Uncovering Overlapping Communities in Social Networks via A Speaker-listener Interaction Dynamic Process", IEEE ICDM workshop on DMCCI 2011, Vancouver, CA.
//============================================================================

#include "Net.h"
#include "fileOpts.h"
#include "CommonFuns.h"
#include "CommonFuns_TMP.h"
#include <cstdlib>
#include <exception>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>

#include "SLPA.h"
//#include <tr1/unordered_set>

//typedef std::tr1::unordered_set<int> UOrderedSet_INT;
typedef std::tr1::unordered_set<NODE *> UOrderedSet_NODEptr;


struct sortIDs {
	bool operator() (int i,int j) { return (i<j);} //increasing
} sortIDs_inc;


Net::Net(string path,string name,string fname){
	networkPath=path;
	netName=name;

	fileName=fname;
}


Net::~Net() {
	while (!NODES.empty()) {
		delete NODES.back();
		NODES.pop_back();
	}
}

int Net::getNumberofEdges(){
	int m=0;
	for(int i=0;i<NODES.size();i++)
		m+=NODES[i]->numNbs;

	return m;
}

void Net::readNet(string fileName,bool isSymmetrize){
	//read the pairs file: 3 or 2 col, the w is IGNORED
	// Lines not starting with a number are ignored
	// *1.remove self loop
	// *2.auto symmetrize (no matter if it is already is)

	string oneLine, whiteSpace=" \t\n";
	fstream fp;
	fp.open(fileName.c_str(),fstream::in); //|fstream::out|fstream::app

	if(fp.is_open()){//if it does not exist, then failed

		// repeat until all lines is read
		while ( fp.good()){
			getline (fp,oneLine);

			//--------------------------
			//skip empty line
			if(oneLine.find_first_not_of(whiteSpace)==string::npos) continue;

			//skip any line NOT starting with digit number
			if(!isdigit(oneLine.at(oneLine.find_first_not_of(whiteSpace)))) continue;

			//cout<<"Line:"<<oneLine<<endl;
			//--------------------------
			int fromID,toID;
			double w=1.0;    //default value

			stringstream linestream(oneLine);
			if((linestream>>fromID) && (linestream>>toID)){
				//if(linestream>>w)
				//	cout<<"w="<<w<<endl;

				if(fromID==toID) continue; 				//**remove selfloop

				pre_ReadInOneEdge(fromID,toID);
				if(isSymmetrize)
					pre_ReadInOneEdge(toID,fromID);     //**symmetrize
			}
		} //while

		fp.close();
	}
	else{
		cout<<"open failed"<<endl;
		exit(1);
	}
}

void Net::readNetwork_EdgesList(string fileName, bool isUseLargestComp, bool isSymmetrize){
	//string(isFileExist(fileName.c_str()))

	//---------------------------------------
	if(!isFileExist(fileName.c_str())){
		cout<< fileName << " not found!" <<endl;
		exit(1);
	}

	NODES.clear();
	NODESTABLE.clear();

	//---------------------------------------
	// a. read in 2 or 3 col network file
	//    initialize NODES,NODESTABLE,v->nbSet
	//---------------------------------------
	time_t st=time(NULL);

	readNet(fileName,isSymmetrize);

	//------------------------------------
	//  b. initialize nbList using nbset (remove duplicated nbs)
	//------------------------------------
	pre_convert_nbSet2_nbList();
	N=NODES.size();
	M=getNumberofEdges();

	cout<<"Reading in the network takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;
	//------------------------------------
	//   b. Using only the largest connected component
	//      do not use this for very large network
	//------------------------------------
	if(isUseLargestComp){
		cout<<"Using largest connected component only.......\n";
		//post_UseLargestComponent_UnorderSet_cpmpointer();
		pre_findGiantComponents();
	}
}

void Net::pre_ReadInOneEdge(int fromID,int toID){
	map<int,NODE *>::iterator mit;
	NODE *vp;

	if (NODESTABLE.count(fromID)>0){//add nb
		mit=NODESTABLE.find(fromID);
		vp=mit->second;

		vp->nbSet.insert(toID);
	}
	else{//add new node
		vp=new NODE(fromID);  //create
		vp->nbSet.insert(toID);

		NODES.push_back(vp);
		NODESTABLE.insert(pair<int,NODE *>(fromID,vp));
	}
}

void Net::showVertices(){
	cout<<"-----------------------"<<endl;
	cout<<NODES.size()<<endl;
	for(int i=0;i<NODES.size();i++){
		NODE * vp=NODES[i];
		cout<< vp->ID<<endl;
	}
}

void Net::showVertices_Table(){
	cout<<"-----------------------"<<endl;
	cout<<NODESTABLE.size()<<endl;

	map<int,NODE *>::iterator it;
	for(it=NODESTABLE.begin();it!=NODESTABLE.end();it++){
		NODE * v=it->second;
		cout<< v->ID<<endl;

		for(int j=0;j<v->numNbs;j++)
			cout<< "->" << v->nbList_P[j]->ID<<endl;
	}
}

void Net::pre_convert_nbSet2_nbList(){
	//now, we can use both nbList_P and nbSet
	//*use nbset rather that nbList_P is after symmetrized (not check),remove duplicated nb
	//  nbList is NOT available
	//
	UOrderedSet_INT::iterator sit;
	NODE * v;
	NODE * nbv;

	//create nbList-pointer version: ASSUMING the TABLES is ready
	for(int i=0;i<NODES.size();i++){
		v=NODES[i];

		v->nbList_P.clear();
		for(sit=v->nbSet.begin();sit!=v->nbSet.end();sit++){
			nbv=NODESTABLE.find(*sit)->second;
			v->nbList_P.push_back(nbv);  //pointer
		}
		v->numNbs=v->nbList_P.size();
	}
}




/*
void Net::pre_convert_nbSet2_nbList_org(){
	//now, we can use both nbList_P and nbSet
	//*use nbset rather that nbList_P is after symmetrized (not check),remove duplicated nb
	//  nbList is NOT available
	//
	set<int>::iterator sit;
	NODE * v;
	NODE * nbv;

	//create nbList-pointer version: ASSUMING the TABLES is ready
	for(int i=0;i<NODES.size();i++){
		v=NODES[i];

		v->nbList_P.clear();
		for(sit=v->nbSet.begin();sit!=v->nbSet.end();sit++){
			nbv=NODESTABLE.find(*sit)->second;
			v->nbList_P.push_back(nbv);  //pointer
		}
		v->numNbs=v->nbList_P.size();
	}
}*/

/*
vector<vector<int> >  Net::pre_findAllConnectedComponents_InOneCluster_CPM(vector<vector<int> >& cpm){
	//***The cluster could be a sub cluster or the whole network***
	//INPUT: CPM(line contains one cluster wiht node ids). if a com consists of separate components, they become
	//       separate comms.
	// Trick: using set instead of list

	//OUTPUT: CPM sorted in **decreasing** cluster size order
	map<int,NODE *>::iterator mit;
	NODE *v;

	SLPA::sort_cpm(cpm);

	vector<vector<int> > newcpm;

	for(int i=0;i<cpm.size();i++){
		//for each community
		set<int> Com(cpm[i].begin(),cpm[i].end());  //copy one com

		while(Com.size()>0){
			//for each component
			set<int>  exploredSet;
			set<int>  unexploredSet;

			//first node
			int vid=getFirstElemnet_Set_PRIMITIVE<int>(Com);

			//set<int> nbSet=NODESTABLE.get(vid).nbSet;
			mit=NODESTABLE.find(vid);
			v=mit->second;
			set<int> nbSet=v->nbSet;


			//Key**: confined to one cluster
			set<int> newnbSet=mySet_Intersect_PRIMITIVE<int>(nbSet,Com);       //CollectionFuns.interSet(nbSet,Com);
			unexploredSet.insert(newnbSet.begin(),newnbSet.end());  //unexploredSet.addAll(newnbSet);
			Com=mySet_Diff_PRIMITIVE<int>(Com,newnbSet);                        //Com.removeAll(newnbSet);

			exploredSet.insert(vid);   //exploredSet.add(vid);
			Com.erase(vid);            //Com.remove(vid);


			while(unexploredSet.size()>0){
				//first node
				vid=getFirstElemnet_Set_PRIMITIVE<int>(unexploredSet);  //vid=getFirstElemnetInSet(unexploredSet);
				mit=NODESTABLE.find(vid);					 //nbSet=NODESTABLE.get(vid).nbSet;
				v=mit->second;
				nbSet.clear();
				nbSet=v->nbSet;

				//***Key: confined to one cluster
				newnbSet.clear();
				newnbSet=mySet_Intersect_PRIMITIVE<int>(nbSet,Com);       //CollectionFuns.interSet(nbSet,Com);
				unexploredSet.insert(newnbSet.begin(),newnbSet.end());  //unexploredSet.addAll(newnbSet);
				Com=mySet_Diff_PRIMITIVE<int>(Com,newnbSet);                        //Com.removeAll(newnbSet);

				unexploredSet.erase(vid); //unexploredSet.remove(vid);
				exploredSet.insert(vid);  //exploredSet.add(vid);
			}

			//get a connected component
			vector<int> oneComponent(exploredSet.begin(),exploredSet.end());
			newcpm.push_back(oneComponent);
		}
	}

	//------------------------
	//sorting
	SLPA::sort_cpm(newcpm);   //	Collections.sort(newcpm,ComCPMsizedec);

	//------------------------
	if(newcpm.size()!=cpm.size()){
		cout<<"before K="<<cpm.size()<<" after post_sameLabelDisconnectedComponents() K="<<newcpm.size()<<endl;
		//System.out.println("------------before----------");
		//show_cpm(cpm);
		//System.out.println("------------after----------");
		//show_cpm(newcpm);
	}

	return newcpm;
}
void Net::post_UseLargestComponent(){
	//***suppose we can use both nbList and nbSet***
	//com contains node id
	time_t st=time(NULL);

	vector<int> largestCom;

	//1. put all nodes in one big com
	vector<vector<int> > cpm;
	vector<int> oneCom;

	for(int i=0;i<NODES.size();i++){
		oneCom.push_back(NODES[i]->ID);
	}
	cpm.push_back(oneCom);

	cout<<"all nodes----------"<<endl;
	//SLPA::show_cpm(cpm);
	//2.find all components(sorted)
	vector<vector<int> > allcomponents_cpm=pre_findAllConnectedComponents_InOneCluster_CPM(cpm);

	cout<<"all connected----------"<<endl;
	//SLPA::show_cpm(allcomponents_cpm);

	//***Key:sorting and get largest
	SLPA::sort_cpm(allcomponents_cpm); //Collections.sort(allcomponents_cpm,ComCPMsizedec);
	largestCom=allcomponents_cpm[0];

	//Trick
	set<int> largestCom_Set(largestCom.begin(),largestCom.end());

	//3.reset the network as only the largest largestCom
	//change NODES,NODESTABLE and N
	vector<NODE *> newNODES;   //ArrayList<NODE> newNODES=new ArrayList<NODE>();
	//map<int,NODE *> NODESTABLE;

	for(int i=0;i<NODES.size();i++){
		NODE* v=NODES[i];
		int vid=v->ID;

		if(isSetMember_PRIMITIVE<int>(largestCom_Set,vid)){
			newNODES.push_back(v);
		}else{
			NODESTABLE.erase(vid);
			delete v;                  //**
		}

	}

	NODES.clear();

	NODES=newNODES;
	N=NODES.size();
	M=getNumberofEdges();

	//4.**assuming that we do not need to handle the nbList and nbSet

	cout<<"Finding largest component in the network takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;
}*/

/*
vector<vector<int> >  Net::pre_findAllConnectedComponents_InOneCluster_CPMpointer_UnorderSet(vector<vector<int> >& cpm){
	//***The cluster could be a sub cluster or the whole network***
	//INPUT: CPM(line contains one cluster wiht node ids). if a com consists of separate components, they become
	//       separate comms.
	// Trick: using set instead of list
	time_t st=time(NULL);

	cout<<"88888888888888888888888888888888888"<<endl;

	bool isWholeNet=false;
	if(cpm.size()==1) {
		cout<<"processing whole network"<<endl;
		isWholeNet=true; //if only one cluster
	}

	//OUTPUT: CPM sorted in **decreasing** cluster size order
	//map<int,NODE *>::iterator mit;
	NODE *v;

	SLPA::sort_cpm(cpm);

	vector<vector<int> > newcpm;

	for(int i=0;i<cpm.size();i++){
		//for each community
		UOrderedSet_INT Com(cpm[i].begin(),cpm[i].end());  //copy one com

		while(Com.size()>0){
			//for each component
			UOrderedSet_INT  exploredSet;
			UOrderedSet_INT  unexploredSet;

			//first node
			//int vid=getFirstElemnet_unorderSet_PRIMITIVE(Com);
			int vid=(*Com.begin());

			//mit=NODESTABLE.find(vid);
			//v=mit->second;
			//UOrderedSet_INT nbSet=v->nbSet;
			UOrderedSet_INT nbSet=NODESTABLE[vid]->nbSet;//*beware of []

			//Key**: confined to one cluster
			UOrderedSet_INT newnbSet;
			newnbSet=mySet_Intersect_PRIMITIVE(nbSet,Com);       //CollectionFuns.interSet(nbSet,Com);

			unexploredSet.insert(newnbSet.begin(),newnbSet.end());  //unexploredSet.addAll(newnbSet);
			Com=mySet_Diff_PRIMITIVE(Com,newnbSet);                        //Com.removeAll(newnbSet);

			exploredSet.insert(vid);   //exploredSet.add(vid);
			Com.erase(vid);            //Com.remove(vid);

			while(unexploredSet.size()>0){
				//first node
				//vid=getFirstElemnet_unorderSet_PRIMITIVE(unexploredSet);
				vid=(*unexploredSet.begin());

				//mit=NODESTABLE.find(vid);					 //nbSet=NODESTABLE.get(vid).nbSet;
				//v=mit->second;
				nbSet.clear();
				//nbSet=v->nbSet;
				nbSet=NODESTABLE[vid]->nbSet;

				//***Key: confined to one cluster
				newnbSet.clear();
				newnbSet=mySet_Intersect_PRIMITIVE(nbSet,Com);

				unexploredSet.insert(newnbSet.begin(),newnbSet.end());  //unexploredSet.addAll(newnbSet);
				Com=mySet_Diff_PRIMITIVE(Com,newnbSet);

				unexploredSet.erase(vid); //unexploredSet.remove(vid);
				exploredSet.insert(vid);  //exploredSet.add(vid);
			}

			//get a connected component
			vector<int> oneComponent(exploredSet.begin(),exploredSet.end());
			newcpm.push_back(oneComponent);
		}
	}

	cout<<"xxxxxxxxxxxxxxxxxxxxxxxx take: "<<difftime(time(NULL),st)<< " seconds."<<endl;
	//------------------------
	//sorting
	SLPA::sort_cpm(newcpm);   //	Collections.sort(newcpm,ComCPMsizedec);

	//------------------------
	if(newcpm.size()!=cpm.size()){
		cout<<"before K="<<cpm.size()<<" after post_sameLabelDisconnectedComponents() K="<<newcpm.size()<<endl;
		//System.out.println("------------before----------");
		//show_cpm(cpm);
		//System.out.println("------------after----------");
		//show_cpm(newcpm);
	}
	return newcpm;
}
*/
/*
void Net::post_UseLargestComponent_UnorderSet_cpmpointer(){
	//***suppose we can use both nbList and nbSet***
	//com contains node id
	time_t st=time(NULL);

	vector<int> largestCom;

	//1. put all nodes in one big com
	vector<vector<int> > cpm;
	vector<int> oneCom;

	for(int i=0;i<NODES.size();i++){
		oneCom.push_back(NODES[i]->ID);
	}
	cpm.push_back(oneCom);

	cout<<"all nodes----------xx"<<endl;
	//SLPA::show_cpm(cpm);
	//2.find all components(sorted)
	vector<vector<int> > allcomponents_cpm=pre_findAllConnectedComponents_InOneCluster_CPMpointer_UnorderSet(cpm);

	cout<<"all connected----------xx"<<endl;
	//SLPA::show_cpm(allcomponents_cpm);

	//***Key:sorting and get largest
	SLPA::sort_cpm(allcomponents_cpm); //Collections.sort(allcomponents_cpm,ComCPMsizedec);
	largestCom=allcomponents_cpm[0];

	//Trick
	set<int> largestCom_Set(largestCom.begin(),largestCom.end());

	//3.reset the network as only the largest largestCom
	//change NODES,NODESTABLE and N
	vector<NODE *> newNODES;   //ArrayList<NODE> newNODES=new ArrayList<NODE>();
	//map<int,NODE *> NODESTABLE;

	for(int i=0;i<NODES.size();i++){
		NODE* v=NODES[i];
		int vid=v->ID;

		if(isSetMember_PRIMITIVE<int>(largestCom_Set,vid)){
			newNODES.push_back(v);
		}else{
			NODESTABLE.erase(vid);
			delete v;                  //**
		}
	}

	NODES.clear();

	NODES=newNODES;
	N=NODES.size();
	M=getNumberofEdges();

	//4.**assuming that we do not need to handle the nbList and nbSet

	cout<<"Finding largest component in the network takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;
}
*/

void Net::pre_findGiantComponents(){
	time_t st=time(NULL);
	//------------------------------------------
	//		find gaint component
	//------------------------------------------
	vector<vector<NODE *> > coms; //all components

	UOrderedSet_NODEptr UnExpSet(NODES.begin(),NODES.end());   // unexported set
	UOrderedSet_NODEptr WorkingSet; 							// unexported set

	NODE *v;
	NODE *nbv;

	while(!UnExpSet.empty()){
		//new com
		vector<NODE *> com;

		v=*UnExpSet.begin(); // take the first node

		//mark this node
		WorkingSet.insert(v);
		UnExpSet.erase(v);
		v->status=1;
		com.push_back(v);

		//find one com
		while(!WorkingSet.empty()){
			v=*WorkingSet.begin(); // take the first nb node

			//explore the nbs
			for(int i=0;i<v->nbList_P.size();i++){
				nbv=v->nbList_P[i];

				if(nbv->status==0){
					//mark this node
					WorkingSet.insert(nbv);
					UnExpSet.erase(nbv);
					nbv->status=1;
					com.push_back(nbv);
				}
			}

			//remove this v
			WorkingSet.erase(v);
		}


		// add to the list
		coms.push_back(com);
	}

	//----------------------------
	//	et the giant one and  update the network
	//----------------------------
	//***Key:sorting and get largest (decreasing size)
	sortVecVec_bySize<NODE* >(coms);

	cout<<"check the sorting of sizes:"<<endl;
	int sum=0;
	for(int i=0;i<coms.size();i++){
		cout<<coms[i].size()<<endl;
		sum+=coms[i].size();
	}
	cout<<" sum size="<<sum<<" N="<<N<<endl;

	//------------------------------------------
	//3.reset the network as only the largest largestCom
	//change NODES,NODESTABLE and N
	NODES.clear();
	NODES=coms[0];  //copy pointer

	NODESTABLE.clear();
	for(int i=0;i<NODES.size();i++)
		NODESTABLE.insert(pair<int,NODE *>(v->ID,NODES[i]));

	N=NODES.size();
	M=getNumberofEdges();

	//------------------------------------------
	//		remove other NODES(***DELETE***)
	//------------------------------------------
	//start *1*
	for(int i=1;i<coms.size();i++){
		for(int j=0;j<coms[i].size();j++)
			delete coms[i][j];  //one node
	}


	//4.**assuming that we do not need to handle the nbList and nbSet
	cout<<"Finding largest component in the network takes :" <<difftime(time(NULL),st)<< " seconds."<<endl;
}


