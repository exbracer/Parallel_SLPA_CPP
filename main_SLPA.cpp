//============================================================================
// Author      : Jierui Xie (xiej2@rpi.edu)
// Date        : Nov. 5th, 2011
// Version     : v1.3
// Copyright   : All rights reserved.
// Description : SLPA algorithm for community detection.
// Web Site    : https://sites.google.com/site/communitydetectionslpa/
//Publication:
//             J. Xie, B. K. Szymanski and X. Liu, "SLPA: Uncovering Overlapping Communities in Social Networks via A Speaker-listener Interaction Dynamic Process", IEEE ICDM workshop on DMCCI 2011, Vancouver, CA.
//============================================================================
//Tested on 4 networks up to million nodes

#include <iostream>
#include <string>
#include "fileOpts.h"
#include "Net.h"
#include "NODE.h"
#include "SLPA.h"
#include <time.h>
#include <vector>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

using namespace std;

map<string,string> readinParameter(int argc,char* argv[]){
	map<string,string> argTable;

	string usage;
	usage+="Usage: ./SLPA -i networkfile\n";
	usage+= "Options:\n";
	usage+= "  -d output director (default: output)\n";
	usage+= "  -L set to 1 to use only the largest connected component\n";
	usage+= "  -t maximum iteration (default: 100)\n";
	usage+= "  -run number of repetitions\n";
	usage+= "  -r a specific threshold in [0,0.5)\n";
	usage+= "  -ov set to 0 to perform disjoint detection\n";
	usage+= "  -M the number of threads(for multi-threading)\n";
	usage+= "more parameters refer to Readme.txt\n";
	//cout<<usage<<endl;

	for (int i=1; i<argc-1; i++)    { //ignore the first one
		string arg(argv[i]);
		string val(argv[i+1]); //**

		if(arg.compare("-t")==0 || arg.compare("-ov")==0 || arg.compare("-r")==0 || arg.compare("-run")==0 || arg.compare("-i")==0 || arg.compare("-d")==0  || arg.compare("-L")==0  || arg.compare("-M")==0){
			if(val.compare(0,1,"-")!=0){
				argTable.insert(pair<string,string>(arg,val));
				i++;continue;
			}
		}
		else{
			cout<<"No such a parameter:"<<arg<<endl;
			cout<<usage<<endl;

			exit(1);
		}
	}

	//--------------------
	if(argTable.size()==0){
		cout<<usage<<endl;
		exit(1);
	}
	else{
		cout<<"INPUT param:"<<endl;
		map<string,string>::iterator mit;
		for(mit=argTable.begin();mit!=argTable.end();mit++){
			cout<<"\t"<<mit->first<<":"<<mit->second<<endl;
		}
	}

	return argTable;
}

void assignParameter(map<string,string>& argTable,int& maxT,bool& isUseLargestComp,bool& isOverlapping,int& maxRun,vector<double>& THRS,string& inputFileName,string& outputDir,int& numThreads){
	string key,v;

	map<string,string>::iterator mit;
	//----------------------------------------
	//			GLPA parameters
	//----------------------------------------
	key="-t";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		maxT=atoi(v.c_str());
	}

	key="-ov";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		if(v.compare("0")==0)
			isOverlapping=false;
	}

	key="-run";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		maxRun=atoi(v.c_str());
	}

	key="-r";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;

		THRS.clear();
		THRS.push_back(atof(v.c_str()));
	}

	//------------------
	if(!isOverlapping){
		THRS.clear();
		THRS.push_back(0.5);
	}

	key="-M"; //multi-thread
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		numThreads=atoi(v.c_str());
	}
	//----------------------------------------
	//			network parameters (default)
	//----------------------------------------
	key="-i";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		inputFileName=v;
	}

	key="-d";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		outputDir=v;
	}

	key="-L";
	if(argTable.count(key)>0){
		v=argTable.find(key)->second;
		if(v.compare("1")==0)
			isUseLargestComp=true;
	}

}

void pre_load_THRS(vector<double> & THRS){
	THRS.clear();

	THRS.push_back(0.01);
	THRS.push_back(0.05);
	THRS.push_back(0.1);
	THRS.push_back(0.15);
	THRS.push_back(0.2);
	THRS.push_back(0.25);
	THRS.push_back(0.3);
	THRS.push_back(0.35);
	THRS.push_back(0.4);
	THRS.push_back(0.45);
}

int main(int argc, char* argv[]) {
	cout<<"Process id="<<getpid()<<endl;

	//----------------------------------------
	//			Network parameters (default)
	//----------------------------------------
	bool isUseLargestComp=false;
	bool isSymmetrize=true;         //not pass to slpa

	string inputFileName="";
	string outputDir="output//";

	//----------------------------------------
	//			SLPA parameters (default)
	//----------------------------------------
	int maxRun=1;
	int maxT=100;
	bool isOverlapping=true;

	vector<double> THRS;
	pre_load_THRS(THRS);


	int numThreads=0;   //0-single thread;

	//----------------------------------------
	//			Read in parameter
	//----------------------------------------
	map<string,string> argTable=readinParameter(argc,argv);
	assignParameter(argTable,maxT,isUseLargestComp,isOverlapping,maxRun,THRS,inputFileName,outputDir,numThreads);

	//----------------------------------------
	//			SLPA
	//----------------------------------------
	if(inputFileName.size()==0 || !isFileExist(inputFileName.c_str())){
		cout<<"ERROR: input network "+inputFileName+" not found!"<<endl;
		exit(1);
	}else{
		time_t st=time(NULL);

		SLPA slpa(inputFileName,THRS,maxRun,maxT,outputDir,isUseLargestComp,numThreads);

		cout<<"Running Time is :" <<difftime(time(NULL),st)<< " seconds."<<endl;
	}

	cout<<"done!"<<endl;
	// Last thing that main() should do
	//****WHY it makes different!!!!!***
	pthread_exit(NULL);
	//return 0;
}
